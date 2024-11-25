#include "llvm/privatize.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

void privatize(Function* task, unordered_set<GlobalVariable*>& need_to_privatize, unordered_map<GlobalVariable*, GlobalVariable*>& private_copy, unordered_map<Function*, unordered_set<GlobalVariable*>>& privatized, unordered_set<Function*>& used_functions, Function* pre_commit) {
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

    BasicBlock& bb_first = task->front();
    Instruction* first = &bb_first.front();
    for (GlobalVariable* gv : need_to_privatize) {
        LoadInst* old_value = new LoadInst(gv->getValueType(), gv, gv->getName() + "_old", first);
        new StoreInst(old_value, private_copy[gv], first);
    }

    Instruction* store_transition_to;
    for (BasicBlock& bb : *task) {
        for (Instruction& i : bb) {
            if (StoreInst* si = dyn_cast<StoreInst>(&i)) {
                if (si->getOperand(1)->getName() == "transition_to_arg") {
                    store_transition_to = &i;
                }
            }
        }
    }

    const DataLayout& dl = task->getParent()->getDataLayout();
    LLVMContext& context = task->getContext();
    for (GlobalVariable* ts : need_to_privatize) {
        ConstantInt* size = ConstantInt::get(Type::getInt32Ty(context), dl.getTypeAllocSize(ts->getValueType()));
        CallInst::Create(pre_commit, makeArrayRef(vector<Value*>{ts, private_copy[ts], size}), "", store_transition_to);
    }
}
