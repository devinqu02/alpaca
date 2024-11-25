#include "llvm/find_war.h"
#include "llvm/privatize.h"

#include <llvm/IR/Attributes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "llvm/find_war.h"

using namespace llvm;

unordered_set<Function*> get_used_functions(Function* task) {
    unordered_set<Function*> used_functions;
    auto dfs = [&](auto& self, Function* f) -> void {
        used_functions.insert(f);

        for (BasicBlock& bb : *f) {
            for (Instruction& i : bb) {
                if (CallInst* ci = dyn_cast<CallInst>(&i)) {
                    Function* to = ci->getCalledFunction();
                    if (!used_functions.count(to)) {
                        self(self, to);
                    }
                }
            }
        }
    };
    dfs(dfs, task);

    return used_functions;
}

namespace {

vector<Function*> get_tasks(Module& M);
struct alpaca_pass : public ModulePass {
    static char ID;
    alpaca_pass() : ModulePass(ID) {}

    bool runOnModule(Module& m) {
        // Get all task-shared variables
        vector<GlobalVariable*> ts_all = find_all_ts(m);

        vector<Function*> tasks = get_tasks(m);

        // For each task, find all reachable functions
        unordered_map<Function*, unordered_set<Function*>> used_functions;
        for (Function* task : tasks) {
            used_functions[task] = get_used_functions(task);
        }

        // For each task, find all task-shared variables that have a WAR dependency
        unordered_map<Function*, vector<GlobalVariable*>> war_in_task;
        for (Function* task : tasks) {
            war_in_task[task] = find_war(*task, ts_all, used_functions);
        }

        // Get all task-shared variables that have a WAR dependency in at least one task
        unordered_set<GlobalVariable*> ts_war;
        for (auto& pr : war_in_task) {
            ts_war.insert(begin(pr.second), end(pr.second));
        }

        // Create privatized copies
        unordered_map<GlobalVariable*, GlobalVariable*> private_copy;
        for (GlobalVariable* gv : ts_war) {
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
            unordered_set<GlobalVariable*> need_to_privatize(begin(war_in_task[task]), end(war_in_task[task]));
            privatize(task, need_to_privatize, private_copy, privatized, used_functions[task], pre_commit);
        }

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
