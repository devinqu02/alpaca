#include "llvm/find_war.h"

#include <llvm/ADT/BitVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Instructions.h>

#include <unordered_map>
#include <unordered_set>

vector<GlobalVariable*> find_all_ts(Module& m) {
    vector<GlobalVariable*> ts;
    for (GlobalVariable& gv : m.globals()) {
        if (gv.getSection() == "nv_data") {
            ts.push_back(&gv);
        }
    }

    return ts;
}

vector<BasicBlock*> get_reverse_postorder(Function& f) {
    vector<BasicBlock*> order;
    unordered_set<BasicBlock*> visited;
    auto dfs = [&](auto& self, BasicBlock* bb) -> void {
        visited.insert(bb);

        for (BasicBlock* to : successors(bb)) {
            if (!visited.count(to)) {
                self(self, to);
            }
        }

        order.push_back(bb);
    };
    dfs(dfs, &f.front());

    reverse(begin(order), end(order));

    return order;
}

bool is_ts_war(Function& sf, GlobalVariable* nv_v, unordered_map<Function*, unordered_set<Function*>>& adj_f) {
    unordered_map<Function*, BitVector> in_f, out_f;
    unordered_map<BasicBlock*, BitVector> in_bb, out_bb;
    unordered_map<Instruction*, BitVector> in_i, out_i;

    for (auto& pr : adj_f) {
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
    in_f[&sf][0] = 1;
    queue_f.insert(&sf);
    while (!empty(queue_f)) {
        Function* f = *begin(queue_f);
        queue_f.erase(begin(queue_f));

        BitVector out_f_old = out_f[f];
        if (empty(*f)) {
            out_f[f] = in_f[f];
        } else {
            in_bb[&f->front()] = in_f[f];
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
                                for (Value* v : i->operands()) {
                                    if (GlobalVariable* gv = dyn_cast<GlobalVariable>(v)) {
                                        if (gv == nv_v) {
                                            if (out_i[i][0]) {
                                                out_i[i][1] = 1;
                                                out_i[i][0] = 0;
                                            }
                                        }
                                    }
                                }
                            } else if (StoreInst* si = dyn_cast<StoreInst>(i)) {
                                for (Value* v : i->operands()) {
                                    if (GlobalVariable* gv = dyn_cast<GlobalVariable>(v)) {
                                        if (gv == nv_v) {
                                            if (out_i[i][0]) {
                                                out_i[i][0] = 0;
                                            }
                                            if (out_i[i][1]) {
                                                return true;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (next(ii) != end(*bb)) {
                            Instruction* nxt = &(*next(ii));
                            in_i[nxt] = out_i[i];
                        }

                        out_bb[bb] = out_i[&bb->back()];
                    }
                }

                if (out_bb[bb] != out_bb_old) {
                    for (BasicBlock* to : successors(bb)) {
                        in_bb[to] |= out_bb[bb];
                        queue_bb.insert(to);
                    }
                }
            }

            out_f[f] = out_bb[&f->back()];
        }

        if (out_f[f] != out_f_old) {
            for (Function* to : adj_f[f]) {
                queue_f.insert(to);
            }
        }
    }

    return false;
}

vector<GlobalVariable*> find_war(Function& f, vector<GlobalVariable*>& ts_all) {
    unordered_map<Function*, unordered_set<Function*>> adj_f;
    unordered_set<Function*> visited;
    auto dfs = [&](auto& self, Function* f) -> void {
        visited.insert(f);
        for (BasicBlock& bb : *f) {
            for (Instruction& i : bb) {
                if (CallInst* ci = dyn_cast<CallInst>(&i)) {
                    Function* to = ci->getCalledFunction();
                    adj_f[f].insert(to);
                    adj_f[to].insert(f);
                    if (visited.count(to)) {
                        self(self, to);
                    }
                }
            }
        }
    };
    adj_f[&f] = {};
    dfs(dfs, &f);

    vector<GlobalVariable*> war;
    for (GlobalVariable* v : ts_all) {
        if (is_ts_war(f, v, adj_f)) {
            war.push_back(v);
        }
    }

    return war;
}
