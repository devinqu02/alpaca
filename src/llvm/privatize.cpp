#include "llvm/privatize.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

void privatize_scalar(Function* task, GlobalVariable* nv, GlobalVariable* priv, unordered_map<Function*, unordered_set<GlobalVariable*>>& privatized, unordered_set<Function*>& used_functions, unordered_map<Instruction*, BitVector>& in, Function* pre_commit) {
    // Replace the non-volatile variables to privatize with their private copy
    for (Function* f : used_functions) {
        if (privatized[f].count(nv)) {
            continue;
        }

        for (BasicBlock& bb : *f) {
            for (Instruction& i : bb) {
                for (int j = 0; j < i.getNumOperands(); ++j) {
                    Value* op = i.getOperand(j);
                    if (GlobalVariable* gv = dyn_cast<GlobalVariable>(op)) {
                        if (gv == nv) {
                            i.setOperand(j, priv);
                        }
                    }
                }
            }
        }

        privatized[f].insert(nv);
    }

    // Copy non-volatile variables to private copies
    BasicBlock& bb_first = task->front();
    Instruction* first = &bb_first.front();
    LoadInst* old_value = new LoadInst(nv->getValueType(), nv, nv->getName() + "_old", first);
    new StoreInst(old_value, priv, first);

    // Need to insert pre_commit calls before all calls of transition_to
    vector<Instruction*> transition_insts;
    for (BasicBlock& bb : *task) {
        for (Instruction& i : bb) {
            if (StoreInst* si = dyn_cast<StoreInst>(&i)) {
                if (si->getOperand(1)->getName() == "transition_to_arg") {
                    transition_insts.push_back(si);
                }
            }
        }
    }

    const DataLayout& dl = task->getParent()->getDataLayout();
    LLVMContext& context = task->getContext();
    for (Instruction* transition_to : transition_insts) {
        if (in.count(transition_to) && !in[transition_to][2] && !in[transition_to][3]) {
            continue;
        }

        ConstantInt* size = ConstantInt::get(Type::getInt32Ty(context), dl.getTypeAllocSize(nv->getValueType()));
        CallInst::Create(pre_commit, makeArrayRef(vector<Value*>{nv, priv, size}), "", transition_to);
    }
}
