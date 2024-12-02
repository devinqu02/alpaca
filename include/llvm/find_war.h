#include <llvm/ADT/BitVector.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

#include <unordered_map>
#include <unordered_set>
#include <utility>

using namespace std;
using namespace llvm;

unordered_set<GlobalVariable*> find_all_nv(Module&);
pair<unordered_set<GlobalVariable*>, unordered_map<GlobalVariable*, unordered_map<Instruction*, BitVector>>> find_war(Function&, unordered_set<GlobalVariable*>&, unordered_map<Function*, unordered_set<Function*>>&);
