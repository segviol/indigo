#pragma once
#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "../../include/aixlog.hpp"
#include "../../mir/mir.hpp"
#include "../backend.hpp"
#include "./var_replace.hpp"
#include "livevar_analyse.hpp"

namespace optimization::global_expr_move {

typedef std::pair<std::pair<mir::inst::VarId, mir::inst::Value>,
                  mir::inst::VarId>
    LoadOp;
class Op {
 public:
  mir::inst::Op op;
  mir::inst::Value lhs;
  mir::inst::Value rhs;
  Op(mir::inst::Op op, mir::inst::Value lhs, mir::inst::Value rhs)
      : op(op), lhs(lhs), rhs(rhs) {}

  bool eqaul(mir::inst::Value val1, mir::inst::Value val2) const {
    if (val1.index() != val2.index()) {
      return false;
    }
    if (val1.index() == 0) {
      return std::get<int>(val1) == std::get<int>(val2);
    }
    return std::get<mir::inst::VarId>(val1).id ==
           std::get<mir::inst::VarId>(val2).id;
  }
  bool operator==(const Op& other) const {
    return op == other.op && eqaul(lhs, other.lhs) && eqaul(rhs, other.rhs);
  }
  Op& operator=(const Op& other) = default;
};
struct hasher {
  size_t operator()(const Op& p) const {
    int lhs = p.lhs.index() == 0 ? std::get<int>(p.lhs)
                                 : std::get<mir::inst::VarId>(p.lhs).id;
    int rhs = p.rhs.index() == 0 ? std::get<int>(p.rhs)
                                 : std::get<mir::inst::VarId>(p.rhs).id;

    return std::hash<int>()(lhs) ^ std::hash<int>()(rhs);
  }
};

class BlockOps {
 public:
  std::unordered_map<Op, mir::inst::VarId, hasher> ops;
  std::map<std::pair<mir::inst::VarId, mir::inst::Value>, mir::inst::VarId>
      loads;
  std::set<mir::inst::VarId> stores;
  BlockOps(mir::inst::BasicBlk& blk, bool offset = false) {
    for (auto& inst : blk.inst) {
      auto& i = *inst;
      if (inst->inst_kind() == mir::inst::InstKind::Op) {
        auto opInst = dynamic_cast<mir::inst::OpInst*>(&i);
        ops.insert({Op(opInst->op, opInst->lhs, opInst->rhs), opInst->dest});
      } else if (offset) {
        if (inst->inst_kind() == mir::inst::InstKind::Load) {
          auto loadInst = dynamic_cast<mir::inst::LoadOffsetInst*>(&i);
          auto addr = std::get<mir::inst::VarId>(loadInst->src);
          if (stores.count(addr)) {
            continue;
          }
          loads.insert({{addr, loadInst->offset}, loadInst->dest});
        } else if (inst->inst_kind() == mir::inst::InstKind::Store) {
          auto storeInst = dynamic_cast<mir::inst::StoreOffsetInst*>(&i);
          stores.insert(storeInst->dest);
        }
      }
    }
  }

  bool has_op(std::variant<Op, LoadOp> op) {
    // for (auto& op : ops) {
    //   auto opInst =
    //       mir::inst::OpInst(mir::inst::VarId(0), op.lhs, op.rhs, op.op);
    //   std::cout << opInst << std::endl;
    // }
    if (disable_op(op)) {
      return false;
    }
    if (op.index() == 0) {
      return ops.find(std::get<Op>(op)) != ops.end();
    } else {
      auto load = std::get<LoadOp>(op);
      return loads.count(load.first);
    }
  }
  bool disable_op(std::variant<Op, LoadOp> op) {
    if (op.index() == 0) {
      return false;
    } else {
      auto load = std::get<LoadOp>(op);
      return stores.count(load.first.first);
    }
  }
};  // namespace optimization::global_expr_move

class Env {
 public:
  mir::inst::MirFunction& func;
  livevar_analyse::Livevar_Analyse& lva;
  std::map<mir::types::LabelId, BlockOps> blk_op_map;
  var_replace::Var_Replace& vp;
  Env(mir::inst::MirFunction& func, livevar_analyse::Livevar_Analyse& lva,
      var_replace::Var_Replace& vp, bool offset)
      : func(func), lva(lva), vp(vp) {
    for (auto& blkpair : func.basic_blks) {
      blk_op_map.insert({blkpair.first, BlockOps(blkpair.second, offset)});
    }
  }
};
std::shared_ptr<Env> env;

class Global_Expr_Mov : public backend::MirOptimizePass {
 public:
  bool offset;
  const std::string name = "Global expression mov";
  std::string pass_name() const { return name; }
  Global_Expr_Mov(bool offset = false) : offset(offset){};
  mir::types::LabelId find_common_live_blk(mir::inst::VarId lhs,
                                           mir::inst::VarId rhs) {
    auto lstart = env->vp.defpoint.at(lhs).first;
    auto rstart = env->vp.defpoint.at(lhs).first;
    auto lblv = env->lva.livevars.at(lstart);
    if (env->lva.livevars.at(lstart)->live_vars_out->count(rhs)) {
      return rstart;
    }
    return lstart;
  }

  std::unique_ptr<mir::inst::OpInst> copy_opinst(
      std::unique_ptr<mir::inst::OpInst>& inst) {
    return std::make_unique<mir::inst::OpInst>(inst->dest, inst->lhs, inst->rhs,
                                               inst->op);
  }

  bool prec_meaningful(int start, int prec) {
    auto& func = env->func;
    auto& precblk = func.basic_blks.at(prec);
    return !(precblk.jump.jump_kind == mir::inst::JumpKind::Loop &&
             precblk.jump.bb_true == start);
  }

  std::set<int> get_precedings(int start) {
    auto prec = env->func.basic_blks.at(start).preceding;
    std::set<int> res;
    std::list<int> queue;
    for (auto p : prec) {
      if (prec_meaningful(start, p)) {
        queue.push_back(p);
      }
    }
    while (!queue.empty()) {
      auto f = queue.front();
      queue.pop_front();
      if (res.count(f)) {
        continue;
      }
      res.insert(f);
      auto prec = env->func.basic_blks.at(f).preceding;
      for (auto p : prec) {
        if (prec_meaningful(f, p)) {
          queue.push_back(p);
        }
      }
    }
    return res;
  }

  mir::types::LabelId bfs(int start, std::variant<Op, LoadOp> op) {
    auto prec = env->func.basic_blks.at(start).preceding;
    if (prec.size() == 0) {
      return start;
    }
    std::set<mir::types::LabelId> res;
    if (prec.size() == 1) {
      auto p = bfs(*prec.begin(), op);
      if (env->blk_op_map.at(p).has_op(op)) {
        res.insert(p);
      }
    } else {
      auto f = *prec.begin();
      res = get_precedings(f);
      if (op.index() != 0) {
        std::set<mir::types::LabelId> disabled_pre;
        for (auto p : res) {
          if (env->blk_op_map.at(p).disable_op(op)) {
            if (disabled_pre.count(p)) {
              continue;
            }
            disabled_pre.insert(p);
            for (auto x : get_precedings(p)) {
              disabled_pre.insert(x);
            }
          }
        }
        for (auto p : disabled_pre) {
          if (res.count(p)) {
            res.erase(p);
          }
        }
      }

      prec.erase(prec.begin());
      for (auto p : prec) {
        auto precs = get_precedings(p);
        std::set<int> tmp;
        std::set_intersection(res.begin(), res.end(), precs.begin(),
                              precs.end(), std::inserter(tmp, tmp.begin()));
        res = std::set(tmp);
      }
    }

    for (auto p : res) {
      if (env->blk_op_map.at(p).has_op(op)) {
        return p;
      }
    }
    return start;
  }

  void optimize_func(mir::inst::MirFunction& func) {
    if (func.type->is_extern) {
      return;
    }

    auto& startblk = func.basic_blks.begin()->second;
    for (auto& blkpair : func.basic_blks) {
      auto& blk = blkpair.second;
      for (auto iter = blkpair.second.inst.begin();
           iter != blkpair.second.inst.end();) {
        auto& inst = *iter;
        if (inst->inst_kind() == mir::inst::InstKind::Ref) {
          auto& i = *inst;
          auto refinst = dynamic_cast<mir::inst::RefInst*>(&i);
          if (blk.id == startblk.id) {
            break;
          }
          func.variables.at(inst->dest.id).is_temp_var = false;
          startblk.inst.push_back(std::move(inst));
          iter = blk.inst.erase(iter);
        } else {
          iter++;
        }
      }
    }
    int cnt = 0;
    while (true) {
      bool modify = false;
      livevar_analyse::Livevar_Analyse lva(func);
      lva.build();
      var_replace::Var_Replace vp(func);
      env = std::make_shared<Env>(func, lva, vp, offset);
      std::list<mir::types::LabelId> queue;
      std::set<mir::types::LabelId> visited;
      auto iter = func.basic_blks.end();
      iter--;
      queue.push_back(iter->first);
      while (!queue.empty()) {
        auto start = queue.front();
        queue.pop_front();
        if (visited.count(start)) {
          continue;
        }

        auto& blk = func.basic_blks.at(start);
        {  // visit logic
          auto& block = blk;
          auto& blv = lva.livevars[start];
          auto& variables = func.variables;
          for (auto iter = block.inst.begin(); iter != block.inst.end();) {
            auto& inst = *iter;
            if (func.variables.at(inst->dest.id).is_phi_var) {
              iter++;
              continue;
            }
            auto kind = inst->inst_kind();
            auto& i = *inst;
            switch (kind) {
              case mir::inst::InstKind::Op: {
                auto opInst = dynamic_cast<mir::inst::OpInst*>(&i);
                if (opInst->lhs.is_immediate() && opInst->rhs.is_immediate()) {
                  break;
                }
                int id = -1;
                Op op(opInst->op, opInst->lhs, opInst->rhs);
                if (!opInst->lhs.is_immediate() &&
                    !opInst->rhs.is_immediate()) {
                  auto lhs = std::get<mir::inst::VarId>(opInst->lhs);
                  auto rhs = std::get<mir::inst::VarId>(opInst->rhs);
                  if (!blv->live_vars_in->count(lhs) ||
                      !blv->live_vars_in->count(rhs)) {
                    break;
                  }
                  id = bfs(start, op);
                } else if (!opInst->lhs.is_immediate()) {
                  auto lhs = std::get<mir::inst::VarId>(opInst->lhs);
                  if (!blv->live_vars_in->count(lhs)) {
                    break;
                  }
                  id = bfs(start, op);

                } else if (!opInst->rhs.is_immediate()) {
                  auto rhs = std::get<mir::inst::VarId>(opInst->rhs);
                  if (!blv->live_vars_in->count(rhs)) {
                    break;
                  }
                  id = bfs(start, op);
                }
                if (id == start) {
                  break;
                }
                auto var = env->blk_op_map.at(id).ops.at(op);
                if (func.variables.at(var.id).is_phi_var) {
                  vp.setdefpoint(inst->dest, id,
                                 func.basic_blks.at(id).inst.size());
                  for (auto var : inst->useVars()) {
                    auto& usepoints = vp.usepoints.at(var);
                    std::pair<mir::types::LabelId, int> point = {
                        blk.id, iter - block.inst.begin()};
                    auto find_iter =
                        std::find(usepoints.begin(), usepoints.end(), point);
                    if (find_iter != usepoints.end()) {
                      usepoints.erase(find_iter);
                      usepoints.push_back(
                          {id, func.basic_blks.at(id).inst.size()});
                    }
                  }
                  func.basic_blks.at(id).inst.push_back(std::move(inst));
                  inst = std::make_unique<mir::inst::AssignInst>(
                      mir::inst::VarId(-1), -1);
                } else {
                  LOG(TRACE)
                      << inst->dest << " is replaced to " << var << std::endl;
                  vp.replace(inst->dest, var);
                  // useless inst
                  inst =
                      std::make_unique<mir::inst::AssignInst>(inst->dest, -999);
                }
                func.variables.at(var).is_temp_var = false;
                modify = true;
                continue;
              }
              case mir::inst::InstKind::Load: {
                if (!offset) {
                  break;
                }
                auto loadInst = dynamic_cast<mir::inst::LoadOffsetInst*>(&i);
                LoadOp op = {{std::get<mir::inst::VarId>(loadInst->src),
                              loadInst->offset},
                             loadInst->dest};
                auto id = bfs(start, op);
                if (id == start) {
                  break;
                }
                auto var = env->blk_op_map.at(id).loads.at(op.first);
                if (!func.variables.at(var.id).is_phi_var) {
                  vp.replace(inst->dest, var);
                  // useless inst
                  inst =
                      std::make_unique<mir::inst::AssignInst>(inst->dest, -999);
                  func.variables.at(var).is_temp_var = false;
                  modify = true;
                  continue;
                } else {
                  break;
                }
              }
              // case mir::inst::InstKind::Store: {
              //   auto storeInst = dynamic_cast<mir::inst::StoreInst*>(&i);

              //   break;
              // }
              default:;
            }
            iter++;
          }
        }
        visited.insert(start);
        for (auto pre : blk.preceding) {
          queue.push_back(pre);
        }
      }
      if (!modify) {
        break;
      }
    }
  }
  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         ++iter) {
      optimize_func(iter->second);
    }
  }
};

}  // namespace optimization::global_expr_move
