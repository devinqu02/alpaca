#include "llvm/find_war.h"

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <unordered_set>
#include <unordered_map>

using namespace llvm;

namespace {
struct alpaca_pass : public ModulePass {
    static char ID;
    alpaca_pass() : ModulePass(ID) {}

    bool runOnModule(Module& m) {
        // All task-shared variables
        vector<GlobalVariable*> ts_all = find_all_ts(m);

        // Task-shared variables that have a WAR dependency in at least one task
        unordered_set<GlobalVariable*> ts_war;
        unordered_map<Function*, vector<GlobalVariable*>> war_in_task;
        for (Function& f : m) {
            war_in_task[&f] = find_war(f, ts_all);
            ts_war.insert(begin(war_in_task[&f]), end(war_in_task[&f]));
        }

        for (GlobalVariable* gv : ts_war) {
            errs() << gv->getName() << '\n';
        }

        return false;
    }
};
}

char alpaca_pass::ID = 0;
static RegisterPass<alpaca_pass> alpaca_pass("alpaca-pass", "Main Alpaca pass");
