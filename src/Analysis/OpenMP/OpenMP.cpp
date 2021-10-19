/* Copyright 2021 Coderrect Inc. All Rights Reserved.
Licensed under the GNU Affero General Public License, version 3 or later (“AGPL”), as published by the Free Software
Foundation. You may not use this file except in compliance with the License. You may obtain a copy of the License at
https://www.gnu.org/licenses/agpl-3.0.en.html
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an “AS IS” BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "OpenMP.h"

#include "Trace/ProgramTrace.h"

using namespace race;

std::vector<std::pair<const llvm::CmpInst *, ThreadID>> race::getConstCmpEqInsts(const llvm::Value *value) {
  std::vector<std::pair<const llvm::CmpInst *, ThreadID>> result;

  std::vector<const llvm::User *> worklist;

  for (auto user : value->users()) {
    worklist.push_back(user);
  }

  while (!worklist.empty()) {
    auto const user = worklist.back();
    worklist.pop_back();

    // follow loads
    if (auto load = llvm::dyn_cast<llvm::LoadInst>(user)) {
      std::copy(load->users().begin(), load->users().end(), std::back_inserter(worklist));
      continue;
    }

    if (auto cmp = llvm::dyn_cast<llvm::CmpInst>(user)) {
      if (cmp->getPredicate() != llvm::CmpInst::Predicate::ICMP_EQ) continue;

      if (auto val = llvm::dyn_cast<llvm::ConstantInt>(cmp->getOperand(1))) {
        result.emplace_back(cmp, val->getZExtValue());
        continue;
      }

      if (auto val = llvm::dyn_cast<llvm::ConstantInt>(cmp->getOperand(0))) {
        result.emplace_back(cmp, val->getZExtValue());
        continue;
      }
    }
  }

  return result;
}

std::set<llvm::BasicBlock *> race::getGuardedBlocks(const llvm::BranchInst *branchInst, bool branch) {
  // This branch should use a cmp eq instruction
  // Otherwise the true/false blocks below may be wrong
  assert(llvm::isa<llvm::CmpInst>(branchInst->getOperand(0)));
  assert(llvm::cast<llvm::CmpInst>(branchInst->getOperand(0))->getPredicate() == llvm::CmpInst::Predicate::ICMP_EQ);

  auto trueBlock = llvm::cast<llvm::BasicBlock>(branchInst->getOperand(2));
  auto falseBlock = llvm::cast<llvm::BasicBlock>(branchInst->getOperand(1));

  auto targetBlock = (branch) ? trueBlock : falseBlock;

  // This will be the returned result
  std::set<llvm::BasicBlock *> guardedBlocks;
  guardedBlocks.insert(targetBlock);

  std::set<llvm::BasicBlock *> visited;
  std::vector<llvm::BasicBlock *> worklist;

  visited.insert(targetBlock);
  std::copy(succ_begin(targetBlock), succ_end(targetBlock), std::back_inserter(worklist));

  do {
    auto currentBlock = worklist.back();
    worklist.pop_back();

    auto hasUnguardedPred = std::any_of(
        pred_begin(currentBlock), pred_end(currentBlock),
        [&guardedBlocks](llvm::BasicBlock *pred) { return guardedBlocks.find(pred) == guardedBlocks.end(); });

    if (hasUnguardedPred) continue;
    visited.insert(currentBlock);

    guardedBlocks.insert(currentBlock);

    for (auto next : successors(currentBlock)) {
      if (visited.find(next) == visited.end()) {
        worklist.push_back(next);
      }
    }

  } while (!worklist.empty());

  return guardedBlocks;
}
