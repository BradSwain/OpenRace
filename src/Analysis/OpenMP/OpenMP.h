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

#pragma once

#include <set>
#include <vector>

#include "Trace/ThreadTrace.h"
#include "llvm/IR/Instructions.h"

namespace race {

// Get any icmp_eq insts that use this value and compare against a constant integer
// return list of pairs (cmp, c) where cmp is the cmpInst and c is the constant value compared against
std::vector<std::pair<const llvm::CmpInst *, ThreadID>> getConstCmpEqInsts(const llvm::Value *value);

// Get list of blocks guarded by one case of this branch.
// branch arg decides if checking for blocks guarded by true or false branch
// Start by assuming the target block is guarded
// Iterate from the target block until we find a block that has an unguarded predecessor
// Cannot handle loops
std::set<llvm::BasicBlock *> getGuardedBlocks(const llvm::BranchInst *branchInst, bool branch = true);

}  // namespace race