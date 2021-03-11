#include "Trace/ThreadTrace.h"

#include "EventImpl.h"
#include "IR/Builder.h"
#include "Trace/CallStack.h"
#include "Trace/ProgramTrace.h"

using namespace race;

namespace {

void traverseCallNode(const pta::CallGraphNodeTy *node, const ThreadTrace &thread, CallStack &callstack,
                      const pta::PTA &pta, std::vector<std::unique_ptr<const Event>> &events) {
    auto func = node->getTargetFun()->getFunction();
    if (callstack.contains(func)) {
        // prevent recursion
        return;
    }
    callstack.push(func);

    auto irFunc = generateRaceFunction(func);
    auto const context = node->getContext();
    auto einfo = std::make_shared<EventInfo>(thread, context);

    for (auto const &ir : irFunc) {
        switch (ir->type) {
            case StmtInfo::Type::Read: {
                std::shared_ptr<const ReadInfo> read(ir, llvm::cast<ReadInfo>(ir.get()));
                events.push_back(std::make_unique<const ReadEventImpl>(read, einfo, events.size()));
                break;
            }
            case StmtInfo::Type::Write: {
                std::shared_ptr<const WriteInfo> write(ir, llvm::cast<WriteInfo>(ir.get()));
                events.push_back(std::make_unique<const WriteEventImpl>(write, einfo, events.size()));
                break;
            }
            case StmtInfo::Type::Fork: {
                std::shared_ptr<const ForkInfo> fork(ir, llvm::cast<ForkInfo>(ir.get()));
                events.push_back(std::make_unique<const ForkEventImpl>(fork, einfo, events.size()));
                break;
            }
            case StmtInfo::Type::Join: {
                std::shared_ptr<const JoinInfo> join(ir, llvm::cast<JoinInfo>(ir.get()));
                events.push_back(std::make_unique<const JoinEventImpl>(join, einfo, events.size()));
                break;
            }
            case StmtInfo::Type::Lock: {
                std::shared_ptr<const LockInfo> lock(ir, llvm::cast<LockInfo>(ir.get()));
                events.push_back(std::make_unique<const LockEventImpl>(lock, einfo, events.size()));
                break;
            }
            case StmtInfo::Type::Unlock: {
                std::shared_ptr<const UnlockInfo> unlock(ir, llvm::cast<UnlockInfo>(ir.get()));
                events.push_back(std::make_unique<const UnlockEventImpl>(unlock, einfo, events.size()));
                break;
            }
            case StmtInfo::Type::Call: {
                std::shared_ptr<const CallInfo> call(ir, llvm::cast<CallInfo>(ir.get()));

                auto directContext = pta::CT::contextEvolve(context, ir->getInst());
                auto const directNode = pta.getDirectNodeOrNull(directContext, call->getInst()->getCalledFunction());

                if (directNode == nullptr) {
                    // TODO: LOG unable to get child node
                    llvm::errs() << "Unable to get child node: " << call->getInst()->getCalledFunction()->getName()
                                 << "\n";
                    continue;
                }

                if (directNode->getTargetFun()->isExtFunction()) {
                    // TODO: LOG skipping external function
                    llvm::errs() << "Skipping external function: " << directNode->getTargetFun()->getName() << "\n";
                    continue;
                }

                events.push_back(std::make_unique<const EnterCallEventImpl>(call, einfo, events.size()));
                traverseCallNode(directNode, thread, callstack, pta, events);
                events.push_back(std::make_unique<const LeaveCallEventImpl>(call, einfo, events.size()));
                break;
            }
        }
    }

    callstack.pop();
}

std::vector<std::unique_ptr<const Event>> buildEventTrace(const ThreadTrace &thread, const pta::CallGraphNodeTy *entry,
                                                          const pta::PTA &pta) {
    std::vector<std::unique_ptr<const Event>> events;
    CallStack callstack;
    traverseCallNode(entry, thread, callstack, pta, events);
    return events;
}
}  // namespace

ThreadTrace::ThreadTrace(const race::ProgramTrace &program, const pta::CallGraphNodeTy *entry)
    : id(0), program(program), spawnEvent(std::nullopt), events(buildEventTrace(*this, entry, program.pta)) {}

ThreadTrace::ThreadTrace(ThreadID id, const ForkEvent *spawningEvent, const pta::CallGraphNodeTy *entry)
    : id(id),
      program(spawningEvent->getThread().program),
      spawnEvent(spawningEvent),
      events(buildEventTrace(*this, entry, program.pta)) {
    auto const entries = spawningEvent->getThreadEntry();
    auto it = std::find(entries.begin(), entries.end(), entry);
    // entry mut be one of the entries from the spawning event
    assert(it != entries.end());
}

std::vector<const ForkEvent *> ThreadTrace::getForkEvents() const {
    std::vector<const ForkEvent *> forks;
    for (auto const &event : events) {
        if (auto fork = llvm::dyn_cast<ForkEvent>(event.get())) {
            forks.push_back(fork);
        }
    }
    return forks;
}

void ThreadTrace::print(llvm::raw_ostream &os) const {
    os << "Thread " << id << "\n";
    for (auto const &event : events) {
        event->print(os);
    }
}
