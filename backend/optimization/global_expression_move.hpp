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
  std::unordered_set<Op, hasher> ops;
  BlockOps(mir::inst::BasicBlk& blk) {
    for (auto& inst : blk.inst) {
      auto& i = *inst;
      if (inst->inst_kind() == mir::inst::InstKind::Op) {
        auto opInst = dynamic_cast<mir::inst::OpInst*>(&i);
        ops.insert(Op(opInst->op, opInst->lhs, opInst->rhs));
      }
    }
  }

  bool has_op(Op op) {
    // for (auto& op : ops) {
    //   auto opInst =
    //       mir::inst::OpInst(mir::inst::VarId(0), op.lhs, op.rhs, op.op);
    //   std::cout << opInst << std::endl;
    // }

    return ops.find(op) != ops.end();
  }
};

class Env {
 public:
  mir::inst::MirFunction& func;
  livevar_analyse::Livevar_Analyse& lva;
  std::map<mir::types::LabelId, BlockOps> blk_op_map;
  var_replace::Var_Replace& vp;
  Env(mir::inst::MirFunction& func, livevar_analyse::Livevar_Analyse& lva,
      var_replace::Var_Replace& vp)
      : func(func), lva(lva), vp(vp) {
    for (auto& blkpair : func.basic_blks) {
      blk_op_map.insert({blkpair.first, BlockOps(blkpair.second)});
    }
  }
};
std::shared_ptr<Env> env;

class Global_Expr_Mov : public backend::MirOptimizePass {
 public:
  const std::string name = "Global expression mov";
  std::string pass_name() const { return name; }

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

  mir::types::LabelId bfs(int start, Op op) {
    auto prec = env->func.basic_blks.at(start).preceding;
    if (prec.size() == 0) {
      return start;
    }
    std::set<int> res;
    if (prec.size() == 1) {
      auto p = bfs(*prec.begin(), op);
      if (env->blk_op_map.at(p).has_op(op)) {
        res.insert(p);
      }
    } else {
      auto f = *prec.begin();
      res = get_precedings(f);
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
    livevar_analyse::Livevar_Analyse lva(func);
    lva.build();
    var_replace::Var_Replace vp(func);
    env = std::make_shared<Env>(func, lva, vp);
    auto& startblk = func.basic_blks.begin()->second;
    while (true) {
      bool modify = false;
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
                if (!opInst->lhs.is_immediate() &&
                    !opInst->rhs.is_immediate()) {
                  auto lhs = std::get<mir::inst::VarId>(opInst->lhs);
                  auto rhs = std::get<mir::inst::VarId>(opInst->rhs);
                  if (!blv->live_vars_in->count(lhs) ||
                      !blv->live_vars_in->count(rhs)) {
                    break;
                  }
                  id = bfs(start, Op(opInst->op, opInst->lhs, opInst->rhs));
                } else if (!opInst->lhs.is_immediate()) {
                  auto lhs = std::get<mir::inst::VarId>(opInst->lhs);
                  if (!blv->live_vars_in->count(lhs)) {
                    break;
                  }
                  id = bfs(start, Op(opInst->op, opInst->lhs, opInst->rhs));

                } else if (!opInst->rhs.is_immediate()) {
                  auto rhs = std::get<mir::inst::VarId>(opInst->rhs);
                  if (!blv->live_vars_in->count(rhs)) {
                    break;
                  }
                  id = bfs(start, Op(opInst->op, opInst->lhs, opInst->rhs));
                }
                if (id == start) {
                  break;
                }
                vp.setdefpoint(inst->dest, start,
                               func.basic_blks.at(id).inst.size());
                func.variables.at(inst->dest.id).is_temp_var = false;
                func.basic_blks.at(id).inst.push_back(std::move(inst));
                iter = block.inst.erase(iter);
                modify = true;
                continue;
              }
              case mir::inst::InstKind::Ref: {
                auto refinst = dynamic_cast<mir::inst::RefInst*>(&i);
                if (refinst->val.index() == 0 || blk.id == startblk.id) {
                  break;
                }
                vp.setdefpoint(inst->dest, start, startblk.inst.size());
                func.variables.at(inst->dest.id).is_temp_var = false;
                startblk.inst.push_back(std::move(inst));
                iter = block.inst.erase(iter);
                modify = true;
                continue;
              }
              // case mir::inst::InstKind::Load: {
              //   auto loadInst = dynamic_cast<mir::inst::LoadInst*>(&i);

              //   break;
              // }
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
