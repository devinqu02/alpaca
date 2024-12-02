#include "llvm/find_war.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>

#include <unordered_map>
#include <unordered_set>

unordered_set<GlobalVariable*> find_all_scalars(Module& m) {
    unordered_set<GlobalVariable*> scalars;
    for (GlobalVariable& gv : m.globals()) {
        // TODO: check if it's a scalar
        if (gv.getSection() == "nv_data") {
            scalars.insert(&gv);
        }
    }

    return scalars;
}

bool is_war_scalar(Function& task, GlobalVariable* scalar, unordered_map<Function*, unordered_set<Function*>>& adj) {
    unordered_map<Function*, BitVector> in_f, out_f;
    unordered_map<BasicBlock*, BitVector> in_bb, out_bb;
    unordered_map<Instruction*, BitVector> in_i, out_i;

    for (auto& pr : adj) {
        Function* f = pr.first;
        in_f[f] = out_f[f] = BitVector(2);
        for (BasicBlock& bb : *f) {
            in_bb[&bb] = out_bb[&bb] = BitVector(2);
            for (Instruction& i : bb) {
                in_i[&i] = out_i[&i] = BitVector(2);
            }
        }
    }

    unordered_set<Function*> queue_f;
    in_f[&task][0] = 1;
    queue_f.insert(&task);
    bool found = false;
    while (!empty(queue_f)) {
        Function* f = *begin(queue_f);
        queue_f.erase(begin(queue_f));

        BitVector out_f_old = out_f[f];
        if (empty(*f)) {
            out_f[f] = in_f[f];
        } else {
            in_bb[&f->front()] = in_f[f];
            // Initialize queue with all blocks because this function may call another function whose output changed
            unordered_set<BasicBlock*> queue_bb;
            for (BasicBlock& bb : *f) {
                queue_bb.insert(&bb);
            }

            while (!empty(queue_bb)) {
                BasicBlock* bb = *begin(queue_bb);
                queue_bb.erase(begin(queue_bb));

                BitVector out_bb_old = out_bb[bb];
                if (empty(*bb)) {
                    out_bb[bb] = in_bb[bb];
                } else {
                    in_i[&bb->front()] = in_bb[bb];
                    for (auto ii = begin(*bb); ii != end(*bb); ++ii) {
                        Instruction* i = &(*ii);
                        if (CallInst* ci = dyn_cast<CallInst>(i)) {
                            Function* to = ci->getCalledFunction();
                            BitVector in_f_to_old = in_f[to];
                            in_f[to] |= in_i[i];
                            if (in_f[to] != in_f_to_old) {
                                queue_f.insert(to);
                            }
                            out_i[i] = out_f[to];
                        } else {
                            out_i[i] = in_i[i];
                            if (LoadInst* li = dyn_cast<LoadInst>(i)) {
                                if (GlobalVariable* gv = dyn_cast<GlobalVariable>(li->getOperand(0))) {
                                    if (gv == scalar) {
                                        if (out_i[i][0]) {
                                            out_i[i][1] = 1;
                                            out_i[i][0] = 0;
                                        }
                                    }
                                }
                            } else if (StoreInst* si = dyn_cast<StoreInst>(i)) {
                                if (GlobalVariable* gv = dyn_cast<GlobalVariable>(si->getOperand(1))) {
                                    if (gv == scalar) {
                                        if (out_i[i][1]) {
                                            return true;
                                        }
                                        out_i[i][0] = out_i[i][1] = 0;
                                    }
                                }
                            }
                        }

                        if (next(ii) != end(*bb)) {
                            Instruction* nxt = &(*next(ii));
                            in_i[nxt] = out_i[i];
                        }
                    }

                    out_bb[bb] = out_i[&bb->back()];
                }

                if (out_bb[bb] != out_bb_old) {
                    for (BasicBlock* to : successors(bb)) {
                        BitVector in_to_old = in_bb[to];
                        in_bb[to] |= out_bb[bb];
                        if (in_bb[to] != in_to_old) {
                            queue_bb.insert(to);
                        }
                    }
                }
            }

            out_f[f] = out_bb[&f->back()];
        }

        if (out_f[f] != out_f_old) {
            for (Function* to : adj[f]) {
                queue_f.insert(to);
            }
        }
    }

    return false;
}

unordered_set<GlobalVariable*> find_war_scalars(Function& task, unordered_set<GlobalVariable*>& scalars, unordered_map<Function*, unordered_set<Function*>>& reachable_functions) {
    unordered_map<Function*, unordered_set<Function*>> adj;
    unordered_set<Function*> visited;
    auto dfs = [&](auto& self, Function* f) -> void {
        visited.insert(f);
        for (BasicBlock& bb : *f) {
            for (Instruction& i : bb) {
                if (CallInst* ci = dyn_cast<CallInst>(&i)) {
                    Function* to = ci->getCalledFunction();
                    if (reachable_functions[&task].count(to)) {
                        adj[f].insert(to);
                        adj[to].insert(f);
                        if (!visited.count(to)) {
                            self(self, to);
                        }
                    }
                }
            }
        }
    };
    adj[&task] = {};
    dfs(dfs, &task);

    unordered_set<GlobalVariable*> war_scalars;
    for (GlobalVariable* gv : scalars) {
        if (is_war_scalar(task, gv, adj)) {
            war_scalars.insert(gv);
        }
    }

    return war_scalars;
}
