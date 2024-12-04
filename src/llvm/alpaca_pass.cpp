#include "llvm/find_war.h"
#include "llvm/privatize.h"

#include <llvm/IR/Attributes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Pass.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>

using namespace llvm;

static cl::opt<bool> enable_alpaca(
    "enable-alpaca", cl::desc("Allow privatization"),
    cl::init(false));

static cl::opt<bool> use_war_analysis(
    "use-war-analysis", cl::desc("Privatize only WAR globals"),
    cl::init(false));

static cl::opt<bool> use_vbm_privatization(
    "use-vbm-privatization", cl::desc("Selectively privatize arrays"),
    cl::init(false));

unordered_set<Function*> get_reachable_functions(Function* task) {
    unordered_set<Function*> reachable_functions;
    auto dfs = [&](auto& self, Function* f) -> void {
        reachable_functions.insert(f);
        for (BasicBlock& bb : *f) {
            for (Instruction& i : bb) {
                if (CallInst* ci = dyn_cast<CallInst>(&i)) {
                    Function* to = ci->getCalledFunction();
                    if (to->getName() != "transition_to" && !reachable_functions.count(to)) {
                        self(self, to);
                    }
                }
            }
        }
    };
    dfs(dfs, task);

    return reachable_functions;
}

namespace {

vector<Function*> get_tasks(Module& M);
struct alpaca_pass : public ModulePass {
    static char ID;
    alpaca_pass() : ModulePass(ID) {}

    bool runOnModule(Module& m) {
        if (!enable_alpaca) {
            return false;
        }

        // Get all non-volatile variables
        unordered_set<GlobalVariable*> nv_set = find_all_nv(m);

        vector<Function*> tasks = get_tasks(m);

        // For each task, find all reachable functions
        unordered_map<Function*, unordered_set<Function*>> reachable_functions;
        for (Function* task : tasks) {
            reachable_functions[task] = get_reachable_functions(task);
        }

        // For each function, find all tasks that can reach it (the inverse of reachable_functions)
        unordered_map<Function*, vector<Function*>> calling_tasks;
        for (auto& task_map : reachable_functions) {
            for (Function* f : task_map.second) {
                calling_tasks[f].push_back(task_map.first);
            }
        }

        // For each function, find all non-volatile variables it uses
        unordered_map<Function*, unordered_set<GlobalVariable*>> used_nv;
        for (Function& f : m) {
            for (BasicBlock& bb : f) {
                for (Instruction& i : bb) {
                    for (Value* v : i.operands()) {
                        if (GlobalVariable* gv = dyn_cast<GlobalVariable>(v)) {
                            if (nv_set.count(gv)) {
                                used_nv[&f].insert(gv);
                            }
                        }
                        if (ConstantExpr* ce = dyn_cast<ConstantExpr>(v)) {
                            if (ce->getOpcode() == Instruction::GetElementPtr) {
                                if (GlobalVariable* gv = dyn_cast<GlobalVariable>(ce->getOperand(0))) {
                                    if (nv_set.count(gv)) {
                                        used_nv[&f].insert(gv);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        unordered_map<Function*, unordered_set<GlobalVariable*>> need_to_privatize;
        // All non-volatile variables that have a WAR dependency in at least one task
        unordered_set<GlobalVariable*> to_privatize;
        unordered_map<Function*, unordered_map<GlobalVariable*, unordered_map<Instruction*, BitVector>>> in;
        if (use_war_analysis) {
            // For each task, find all non-volatile variables that have a WAR dependency
            for (Function* task : tasks) {
                auto war_res = find_war(*task, nv_set, reachable_functions);
                need_to_privatize[task] = war_res.first;
                in[task] = war_res.second;

                to_privatize.insert(need_to_privatize[task].begin(), need_to_privatize[task].end());
            }

            for (GlobalVariable* gv : to_privatize) {
                queue<Function*> q;
                for (Function* task : tasks) {
                    if (need_to_privatize[task].count(gv)) {
                        q.push(task);
                    }
                }

                while (!q.empty()) {
                    Function* task = q.front();
                    q.pop();
                    for (Function* f : reachable_functions[task]) {
                        if (!used_nv[f].count(gv)) {
                            continue;
                        }

                        for (Function* task2 : calling_tasks[f]) {
                            if (!need_to_privatize[task2].count(gv)) {
                                need_to_privatize[task2].insert(gv);
                                q.push(task2);
                            }
                        }
                    }
                }
            }
        } else {
            for (Function* task : tasks) {
                for (Function* f : reachable_functions[task]) {
                    for (GlobalVariable* gv : used_nv[f]) {
                        need_to_privatize[task].insert(gv);
                        to_privatize.insert(gv);
                    }
                }
            }
        }

        // Create private copies
        unordered_map<GlobalVariable*, GlobalVariable*> priv, vbm;
        LLVMContext& context = m.getContext();
        for (GlobalVariable* gv : to_privatize) {
            priv[gv] = new GlobalVariable(m, gv->getValueType(), gv->isConstant(), gv->getLinkage(), gv->getInitializer(), gv->getName() + "_priv", gv, gv->getThreadLocalMode(), gv->getAddressSpace(), gv->isExternallyInitialized());
            priv[gv]->copyAttributesFrom(gv);

            if (ArrayType* at = dyn_cast<ArrayType>(gv->getValueType())) {
                if (use_vbm_privatization) {
                    ArrayType* vbm_type = ArrayType::get(Type::getInt16Ty(context), at->getNumElements());
                    vbm[gv] = new GlobalVariable(m, vbm_type, gv->isConstant(), gv->getLinkage(), ConstantAggregateZero::get(vbm_type), gv->getName() + "_vbm", gv, gv->getThreadLocalMode(), gv->getAddressSpace(), gv->isExternallyInitialized());
                    vbm[gv]->copyAttributesFrom(gv);
                    vbm[gv]->setAlignment(Align(2));
                }
            }
        }

        // Declaration for pre_commit()
        FunctionType* pre_commit_type = FunctionType::get(Type::getVoidTy(context), {PointerType::getUnqual(context), PointerType::getUnqual(context), Type::getInt32Ty(context)}, false);
        Function* pre_commit = Function::Create(pre_commit_type, Function::ExternalLinkage, "pre_commit", &m);
        for (int i = 1; i <= 3; ++i) {
            pre_commit->addAttributeAtIndex(i, Attribute::get(context, Attribute::NoUndef));
        }

        FunctionType* handle_array_type = FunctionType::get(Type::getVoidTy(context), {PointerType::getUnqual(context), PointerType::getUnqual(context), PointerType::getUnqual(Type::getInt16Ty(context)), Type::getInt32Ty(context), Type::getInt32Ty(context)}, false);
        Function* handle_load = Function::Create(handle_array_type, Function::ExternalLinkage, "handle_load", &m);
        Function* handle_store = Function::Create(handle_array_type, Function::ExternalLinkage, "handle_store", &m);
        for (int i = 1; i <= 5; ++i) {
            handle_load->addAttributeAtIndex(i, Attribute::get(context, Attribute::NoUndef));
            handle_store->addAttributeAtIndex(i, Attribute::get(context, Attribute::NoUndef));
        }

        FunctionType* naive_array_privatization_type = FunctionType::get(Type::getVoidTy(context), {PointerType::getUnqual(context), PointerType::getUnqual(context), Type::getInt32Ty(context), Type::getInt32Ty(context)}, false);
        Function* sync_priv = Function::Create(naive_array_privatization_type, Function::ExternalLinkage, "sync_priv", &m);
        Function* pre_commit_array = Function::Create(naive_array_privatization_type, Function::ExternalLinkage, "pre_commit_array", &m);
        for (int i = 1; i <= 4; ++i) {
            sync_priv->addAttributeAtIndex(i, Attribute::get(context, Attribute::NoUndef));
            pre_commit_array->addAttributeAtIndex(i, Attribute::get(context, Attribute::NoUndef));
        }

        unordered_map<Function*, unordered_set<GlobalVariable*>> privatized;
        unordered_set<Instruction*> handled_instructions;
        for (Function* task : tasks) {
            for (GlobalVariable* nv : need_to_privatize[task]) {
                if (isa<ArrayType>(nv->getValueType())) {
                    if (use_vbm_privatization) {
                        privatize_array_vbm(task, nv, priv[nv], vbm[nv], handled_instructions, reachable_functions[task], in[task][nv], handle_load, handle_store);
                    } else {
                        privatize_array(task, nv, priv[nv], privatized, reachable_functions[task], in[task][nv], sync_priv, pre_commit_array);
                    }
                } else {
                    privatize_scalar(task, nv, priv[nv], privatized, reachable_functions[task], in[task][nv], pre_commit);
                }
            }
        }

        return true;
    }
};

vector<Function*> get_tasks(Module& m) {
    vector<Function*> tasks;
    GlobalVariable* annotations = m.getNamedGlobal("llvm.global.annotations");
    if (!annotations || !annotations->hasInitializer()) return tasks;

    // Get initializer (array of annotations)
    if (ConstantArray* CA =
            dyn_cast<ConstantArray>(annotations->getInitializer())) {
        for (unsigned i = 0; i < CA->getNumOperands(); ++i) {
            ConstantStruct* CS = dyn_cast<ConstantStruct>(CA->getOperand(i));
            if (!CS) continue;
            if (CS->getNumOperands() < 2) continue;

            Function* F =
                dyn_cast<Function>(CS->getOperand(0)->stripPointerCasts());
            GlobalVariable* C = dyn_cast<GlobalVariable>(
                CS->getOperand(1)->stripPointerCasts());

            if (!F || !C || !C->hasInitializer()) continue;

            if (ConstantDataArray* D =
                    dyn_cast<ConstantDataArray>(C->getInitializer())) {
                tasks.push_back(F);
            }
        }
    }
    return tasks;
}

}  // namespace

char alpaca_pass::ID = 0;
static RegisterPass<alpaca_pass> alpaca_pass("alpaca-pass", "Main Alpaca pass");
