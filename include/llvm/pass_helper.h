#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/GlobalVariable.h>

using namespace llvm;

bool is_load(Instruction*, GlobalVariable*);

bool is_store(Instruction*, GlobalVariable*);
