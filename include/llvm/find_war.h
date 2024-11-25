#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace llvm;

vector<GlobalVariable*> find_all_ts(Module&);
vector<GlobalVariable*> find_war(Function&, vector<GlobalVariable*>&, unordered_map<Function*, unordered_set<Function*>>&);
