#include <llvm/ADT/BitVector.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

#include <unordered_map>
#include <unordered_set>

using namespace std;
using namespace llvm;

unordered_set<GlobalVariable*> find_all_scalars(Module&);
unordered_set<GlobalVariable*> find_war_scalars(Function&, unordered_set<GlobalVariable*>&, unordered_map<Function*, unordered_set<Function*>>&);
