#include "llvm/pass_helper.h"

bool is_load(Instruction* i, GlobalVariable* nv = nullptr) {
    if (LoadInst* li = dyn_cast<LoadInst>(i)) {
        if (GlobalVariable* gv = dyn_cast<GlobalVariable>(li->getOperand(0))) {
            if (gv == nv || (!nv && gv->getSection() == "nv_data")) {
                return true;
            }
        } else if (ConstantExpr* ce = dyn_cast<ConstantExpr>(li->getOperand(0))) {
            if (ce->getOpcode() == Instruction::GetElementPtr) {
                if (GlobalVariable* gv = dyn_cast<GlobalVariable>(ce->getOperand(0))) {
                    if (gv == nv || (!nv && gv->getSection() == "nv_data")) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool is_store(Instruction* i, GlobalVariable* nv = nullptr) {
    if (StoreInst* si = dyn_cast<StoreInst>(i)) {
        if (GlobalVariable* gv = dyn_cast<GlobalVariable>(si->getOperand(1))) {
            if (gv == nv || (!nv && gv->getSection() == "nv_data")) {
                return true;
            }
        } else if (ConstantExpr* ce = dyn_cast<ConstantExpr>(si->getOperand(1))) {
            if (ce->getOpcode() == Instruction::GetElementPtr) {
                if (GlobalVariable* gv = dyn_cast<GlobalVariable>(ce->getOperand(0))) {
                    if (gv == nv || (!nv && gv->getSection() == "nv_data")) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}
