#include "llvm/pass_helper.h"

bool is_load(Instruction* i, GlobalVariable* nv) {
    if (LoadInst* li = dyn_cast<LoadInst>(i)) {
        if (GlobalVariable* gv = dyn_cast<GlobalVariable>(li->getOperand(0))) {
            if (gv == nv) {
                return true;
            }
        } else if (ConstantExpr* ce = dyn_cast<ConstantExpr>(li->getOperand(0))) {
            if (ce->getOpcode() == Instruction::GetElementPtr) {
                if (ce->getOperand(0) == nv) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool is_store(Instruction* i, GlobalVariable* nv) {
    if (StoreInst* si = dyn_cast<StoreInst>(i)) {
        if (GlobalVariable* gv = dyn_cast<GlobalVariable>(si->getOperand(1))) {
            if (gv == nv) {
                return true;
            }
        } else if (ConstantExpr* ce = dyn_cast<ConstantExpr>(si->getOperand(1))) {
            if (ce->getOpcode() == Instruction::GetElementPtr) {
                if (ce->getOperand(0) == nv) {
                    return true;
                }
            }
        }
    }

    return false;
}
