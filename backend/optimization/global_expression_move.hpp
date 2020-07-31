#pragma once
#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <iostream>
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

class BlockOps {
 public:
  std::set<
      std::pair<mir::inst::Op, std::pair<mir::inst::Value, mir::inst::Value>>>
      ops;
  BlockOps(mir::inst::BasicBlk& blk) {
    for (auto& inst : blk.inst) {
      auto& i = *inst;
      if (inst->inst_kind() == mir::inst::InstKind::Op) {
        auto opInst = dynamic_cast<mir::inst::OpInst*>(&i);
        ops.insert({opInst->op, {opInst->lhs, opInst->rhs}});
      }
    }
  }

  bool has_op(
      std::pair<mir::inst::Op, std::pair<mir::inst::Value, mir::inst::Value>>
          op) {
    return ops.count(op);
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

  mir::types::LabelId bfs(
      int start,
      std::pair<mir::inst::Op, std::pair<mir::inst::Value, mir::inst::Value>>
          op) {
    std::list<mir::types::LabelId> queue;
    std::set<mir::types::LabelId> visited;
    visited.insert(start);
    for (auto pre : env->func.basic_blks.at(start).preceding) {
      if (pre != start) {
        queue.push_back(pre);
      }
    }
    while (!queue.empty()) {
      bool all_has_op = true;
      for (auto pre : queue) {
        if (!env->blk_op_map.at(pre).has_op(op)) {
          all_has_op = false;
          break;
        }
      }
      if (all_has_op) {
        if (queue.size() == 1) {
          return queue.front();
        }
      }
      auto front = queue.front();
      queue.pop_front();
      if (visited.count(front)) {
        continue;
      }
      visited.insert(front);
      for (auto pre : env->func.basic_blks.at(front).preceding) {
        if (visited.count(pre)) {
          queue.push_back(pre);
        }
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
        if (start == 6) {
          std::cout << "as";
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
                  id = bfs(start, {opInst->op, {opInst->lhs, opInst->rhs}});
                } else if (!opInst->lhs.is_immediate()) {
                  auto lhs = std::get<mir::inst::VarId>(opInst->lhs);
                  if (!blv->live_vars_in->count(lhs)) {
                    break;
                  }
                  id = bfs(start, {opInst->op, {opInst->lhs, opInst->rhs}});

                } else if (!opInst->rhs.is_immediate()) {
                  auto rhs = std::get<mir::inst::VarId>(opInst->rhs);
                  if (!blv->live_vars_in->count(rhs)) {
                    break;
                  }
                  id = bfs(start, {opInst->op, {opInst->lhs, opInst->rhs}});
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
