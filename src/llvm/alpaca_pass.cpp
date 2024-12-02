#include "llvm/find_war.h"
#include "llvm/privatize.h"

#include <llvm/IR/Attributes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>

using namespace llvm;

constexpr bool use_war_scalar_analysis = true;

unordered_set<Function*> get_reachable_functions(Function* task) {
    unordered_set<Function*> reachable_functions;
    auto dfs = [&](auto& self, Function* f) -> void {
        reachable_functions.insert(f);
        for (BasicBlock& bb : *f) {
            for (Instruction& i : bb) {
                if (CallInst* ci = dyn_cast<CallInst>(&i)) {
                    Function* to = ci->getCalledFunction();
                    if (to->getName() != "transition_to" && !reachable_functions.count(to)) {
                        self(self, to);
                    }
                }
            }
        }
    };
    dfs(dfs, task);

    return reachable_functions;
}

namespace {

vector<Function*> get_tasks(Module& M);
struct alpaca_pass : public ModulePass {
    static char ID;
    alpaca_pass() : ModulePass(ID) {}

    bool runOnModule(Module& m) {
        // Get all non volatile scalars
        unordered_set<GlobalVariable*> scalars = find_all_scalars(m);

        vector<Function*> tasks = get_tasks(m);

        // For each task, find all reachable functions
        unordered_map<Function*, unordered_set<Function*>> reachable_functions;
        for (Function* task : tasks) {
            reachable_functions[task] = get_reachable_functions(task);
        }

        // For each function, find all tasks that reach it
        unordered_map<Function*, vector<Function*>> calling_tasks;
        for (auto& task_map : reachable_functions) {
            for (Function* f : task_map.second) {
                calling_tasks[f].push_back(task_map.first);
            }
        }

        // For each function, find all scalars it uses
        unordered_map<Function*, unordered_set<GlobalVariable*>> used_scalars;
        for (Function& f : m) {
            for (BasicBlock& bb : f) {
                for (Instruction& i : bb) {
                    for (Value* v : i.operands()) {
                        if (GlobalVariable* gv = dyn_cast<GlobalVariable>(v)) {
                            if (scalars.count(gv)) {
                                used_scalars[&f].insert(gv);
                            }
                        }
                    }
                }
            }
        }

        unordered_map<Function*, unordered_set<GlobalVariable*>> need_to_privatize;
        // Get all non volatile scalars that have a WAR dependency in at least one task
        unordered_set<GlobalVariable*> war_scalars;

        if (use_war_scalar_analysis) {
            // For each task, find all non volatile scalars that have a WAR dependency
            for (Function* task : tasks) {
                need_to_privatize[task] = find_war_scalars(*task, scalars, reachable_functions);
            }

            for (Function* task : tasks) {
                war_scalars.insert(need_to_privatize[task].begin(), need_to_privatize[task].end());
            }

            for (GlobalVariable* gv : war_scalars) {
                queue<Function*> q;
                for (Function* task : tasks) {
                    if (need_to_privatize[task].count(gv)) {
                        q.push(task);
                    }
                }

                while (!q.empty()) {
                    Function* task = q.front();
                    q.pop();
                    for (Function* f : reachable_functions[task]) {
                        if (!used_scalars[f].count(gv)) {
                            continue;
                        }

                        for (Function* task2 : calling_tasks[f]) {
                            if (!need_to_privatize[task2].count(gv)) {
                                need_to_privatize[task2].insert(gv);
                                q.push(task2);
                            }
                        }
                    }
                }
            }
        } else {
            for (Function* task : tasks) {
                for (Function* f : reachable_functions[task]) {
                    for (GlobalVariable* gv : used_scalars[f]) {
                        need_to_privatize[task].insert(gv);
                        war_scalars.insert(gv);
                    }
                }
            }
        }

        // Create private copies
        unordered_map<GlobalVariable*, GlobalVariable*> private_copy;
        for (GlobalVariable* gv : war_scalars) {
            private_copy[gv] = new GlobalVariable(m, gv->getValueType(), gv->isConstant(), gv->getLinkage(), gv->getInitializer(), gv->getName() + "_priv", gv, gv->getThreadLocalMode(), gv->getAddressSpace(), gv->isExternallyInitialized());
        }

        LLVMContext& context = m.getContext();
        FunctionType* pre_commit_type = FunctionType::get(Type::getVoidTy(context), {PointerType::getUnqual(context), PointerType::getUnqual(context), Type::getInt32Ty(context)}, false);
        Function* pre_commit = Function::Create(pre_commit_type, Function::ExternalLinkage, "pre_commit", &m);
        pre_commit->addAttributeAtIndex(1, Attribute::get(context, Attribute::NoUndef));
        pre_commit->addAttributeAtIndex(2, Attribute::get(context, Attribute::NoUndef));
        pre_commit->addAttributeAtIndex(3, Attribute::get(context, Attribute::NoUndef));

        unordered_map<Function*, unordered_set<GlobalVariable*>> privatized;
        for (Function* task : tasks) {
            privatize(task, need_to_privatize[task], private_copy, privatized, reachable_functions[task]);

            insert_precommit(task, need_to_privatize[task], private_copy, pre_commit);
        }

        errs() << m << '\n';

        return false;
    }
};

vector<Function*> get_tasks(Module& m) {
    vector<Function*> tasks;
    GlobalVariable* annotations = m.getNamedGlobal("llvm.global.annotations");
    if (!annotations || !annotations->hasInitializer()) return tasks;

    // Get initializer (array of annotations)
    if (ConstantArray* CA =
            dyn_cast<ConstantArray>(annotations->getInitializer())) {
        for (unsigned i = 0; i < CA->getNumOperands(); ++i) {
            ConstantStruct* CS = dyn_cast<ConstantStruct>(CA->getOperand(i));
            if (!CS) continue;
            if (CS->getNumOperands() < 2) continue;

            Function* F =
                dyn_cast<Function>(CS->getOperand(0)->stripPointerCasts());
            GlobalVariable* C = dyn_cast<GlobalVariable>(
                CS->getOperand(1)->stripPointerCasts());

            if (!F || !C || !C->hasInitializer()) continue;

            if (ConstantDataArray* D =
                    dyn_cast<ConstantDataArray>(C->getInitializer())) {
                tasks.push_back(F);
            }
        }
    }
    return tasks;
}

}  // namespace

char alpaca_pass::ID = 0;
static RegisterPass<alpaca_pass> alpaca_pass("alpaca-pass", "Main Alpaca pass");
