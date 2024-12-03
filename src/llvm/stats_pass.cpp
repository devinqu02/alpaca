#include "llvm/pass_helper.h"

#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

using namespace llvm;

namespace {

struct stats_pass : public ModulePass {
    static char ID;
    stats_pass() : ModulePass(ID) {}

    bool runOnModule(Module& m) {
        LLVMContext& context = m.getContext();
        const DataLayout& dl = m.getDataLayout();

        int memory_usage = 0;
        for (GlobalVariable& gv : m.globals()) {
            if (gv.getSection() == "nv_data" && gv.getName() != "load_count" && gv.getName() != "store_count") {
                if (ArrayType* at = dyn_cast<ArrayType>(gv.getValueType())) {
                    memory_usage += dl.getTypeAllocSize(at->getArrayElementType()) * at->getNumElements();
                } else {
                    memory_usage += dl.getTypeAllocSize(gv.getValueType());
                }
            }
        }
        errs() << "Memory Usage: " << memory_usage << " bytes\n";

        GlobalVariable* load_count = m.getGlobalVariable("load_count");
        GlobalVariable* store_count = m.getGlobalVariable("store_count");
        for (Function& f : m) {
            for (BasicBlock& bb : f) {
                for (auto ii = bb.begin(); ii != bb.end(); ++ii) {
                    Instruction& i = *ii;
                    if (is_load(&i, nullptr) && load_count) {
                        LoadInst* load_count_old = new LoadInst(load_count->getValueType(), load_count, "load_count_old", &i);
                        BinaryOperator* load_count_new = BinaryOperator::CreateAdd(load_count_old, ConstantInt::get(load_count_old->getType(), 1), "load_count_new", &i);
                        new StoreInst(load_count_new, load_count, &i);
                    } else if (is_store(&i, nullptr) && store_count) {
                        LoadInst* store_count_old = new LoadInst(store_count->getValueType(), store_count, "store_count_old", &i);
                        BinaryOperator* store_count_new = BinaryOperator::CreateAdd(store_count_old, ConstantInt::get(store_count_old->getType(), 1), "store_count_new", &i);
                        new StoreInst(store_count_new, store_count, &i);
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
