#include "llvm/pass_helper.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <vector>

using namespace llvm;
using namespace std;

namespace {

struct stats_pass : public ModulePass {
    static char ID;
    stats_pass() : ModulePass(ID) {}

    bool runOnModule(Module& m) {
#ifndef STATS
        return false;
#endif
        LLVMContext& context = m.getContext();
        const DataLayout& dl = m.getDataLayout();

        FunctionType* track_access_type = FunctionType::get(Type::getVoidTy(context), {PointerType::getUnqual(context), Type::getInt32Ty(context)}, false);
        Function* track_load = m.getFunction("track_load");
        if (!track_load) {
            track_load = Function::Create(track_access_type, Function::ExternalLinkage, "track_load", &m);
        }
        Function* track_store = m.getFunction("track_store");
        if (!track_store) {
            track_store = Function::Create(track_access_type, Function::ExternalLinkage, "track_store", &m);
        }
        for (Function& f : m) {
            if (&f == track_load || &f == track_store) {
                continue;
            }

            for (BasicBlock& bb : f) {
                for (auto ii = bb.begin(); ii != bb.end(); ++ii) {
                    Instruction& i = *ii;
                    if (LoadInst* li = dyn_cast<LoadInst>(&i)) {
                        ConstantInt* size = ConstantInt::get(Type::getInt32Ty(context), dl.getTypeAllocSize(li->getType()));
                        CallInst::Create(track_load, makeArrayRef(vector<Value*>{li->getOperand(0), size}), "", li);
                    } else if (StoreInst* si = dyn_cast<StoreInst>(&i)) {
                        ConstantInt* size = ConstantInt::get(Type::getInt32Ty(context), dl.getTypeAllocSize(si->getOperand(0)->getType()));
                        CallInst::Create(track_store, makeArrayRef(vector<Value*>{si->getOperand(1), size}), "", si);
                    } else if (MemCpyInst* mci = dyn_cast<MemCpyInst>(&i)) {
                        CastInst* size = CastInst::CreateIntegerCast(mci->getOperand(2), Type::getInt32Ty(context), false, "", mci);
                        CallInst::Create(track_load, makeArrayRef(vector<Value*>{mci->getOperand(1), size}), "", mci);
                        CallInst::Create(track_store, makeArrayRef(vector<Value*>{mci->getOperand(0), size}), "", mci);
                    }
                }
            }
        }

        return true;
    }
};

}

char stats_pass::ID = 0;
static RegisterPass<stats_pass> stats_pass("stats-pass", "NV memory usage and access counter");
