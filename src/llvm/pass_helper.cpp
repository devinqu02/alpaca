#include "llvm/pass_helper.h"

bool is_load(Instruction* i, GlobalVariable* nv = nullptr) {
    if (LoadInst* li = dyn_cast<LoadInst>(i)) {
        if (GlobalVariable* gv = dyn_cast<GlobalVariable>(li->getOperand(0))) {
            return (gv == nv || (!nv && gv->getSection() == "nv_data"));
        } else if (ConstantExpr* ce = dyn_cast<ConstantExpr>(li->getOperand(0))) {
            if (ce->getOpcode() == Instruction::GetElementPtr) {
                if (GlobalVariable* gv = dyn_cast<GlobalVariable>(ce->getOperand(0))) {
                    return (gv == nv || (!nv && gv->getSection() == "nv_data"));
                }
            }
        } else if (GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(li->getOperand(0))) {
            if (GlobalVariable* gv = dyn_cast<GlobalVariable>(gep->getOperand(0))) {
                return (gv == nv || (!nv && gv->getSection() == "nv_data"));
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
        } else if (GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(si->getOperand(1))) {
            if (GlobalVariable* gv = dyn_cast<GlobalVariable>(gep->getOperand(0))) {
                return (gv == nv || (!nv && gv->getSection() == "nv_data"));
            }
        }
    }

    return false;
}
