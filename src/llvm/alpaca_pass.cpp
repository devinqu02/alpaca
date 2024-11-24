#include <llvm/IR/Constants.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "llvm/find_war.h"

using namespace llvm;

namespace {

vector<Function*> get_tasks(Module& M);
struct alpaca_pass : public ModulePass {
    static char ID;
    alpaca_pass() : ModulePass(ID) {}

    bool runOnModule(Module& m) {
        // All task-shared variables
        vector<GlobalVariable*> ts_all = find_all_ts(m);

        // Task-shared variables that have a WAR dependency in at least one task
        unordered_set<GlobalVariable*> ts_war;
        unordered_map<Function*, vector<GlobalVariable*>> war_in_task;
        for (Function* f : get_tasks(m)) {
            war_in_task[f] = find_war(*f, ts_all);
            ts_war.insert(begin(war_in_task[f]), end(war_in_task[f]));
        }

        for (GlobalVariable* gv : ts_war) {
            errs() << gv->getName() << '\n';
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
