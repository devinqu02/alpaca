#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>

#include <vector>

using namespace std;
using namespace llvm;

vector<GlobalVariable*> find_all_ts(Module&);
vector<GlobalVariable*> find_war(Function&, vector<GlobalVariable*>&);
