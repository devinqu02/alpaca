#include "llvm/privatize.h"

#include <llvm/IR/Instructions.h>

void privatize(Function* task, unordered_set<GlobalVariable*>& need_to_privatize, unordered_map<GlobalVariable*, GlobalVariable*>& private_copy, unordered_map<Function*, unordered_set<GlobalVariable*>>& privatized, unordered_set<Function*>& used_functions) {
    for (Function* f : used_functions) {
        for (GlobalVariable* ts : need_to_privatize) {
            if (privatized[f].count(ts)) {
                continue;
            }

            for (BasicBlock& bb : *f) {
                for (Instruction& i : bb) {
                    for (int j = 0; j < i.getNumOperands(); ++j) {
                        Value* op = i.getOperand(j);
                        if (GlobalVariable* gv = dyn_cast<GlobalVariable>(op)) {
                            if (need_to_privatize.count(gv)) {
                                i.setOperand(j, private_copy[gv]);
                            }
                        }
                    }
                }
            }

            privatized[f].insert(ts);
        }
    }

    if (!empty(*task)) {
        BasicBlock& bb = task->front();
        Instruction* first = &bb.front();
        for (GlobalVariable* gv : need_to_privatize) {
            LoadInst* old_value = new LoadInst(gv->getValueType(), gv, gv->getName() + "_old", first);
            new StoreInst(old_value, private_copy[gv], first);
        }

        errs() << *task << '\n';
    }
}
