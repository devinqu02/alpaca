#include <llvm/ADT/BitVector.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>

#include <unordered_map>
#include <unordered_set>

using namespace llvm;
using namespace std;

void privatize_scalar(Function*, unordered_set<GlobalVariable*>&, unordered_map<GlobalVariable*, GlobalVariable*>&, unordered_map<Function*, unordered_set<GlobalVariable*>>&, unordered_set<Function*>&, unordered_map<GlobalVariable*, unordered_map<Instruction*, BitVector>>&, Function*);
