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
#include <utility>
#include <variant>
#include <vector>

#include "../../include/aixlog.hpp"
#include "../../mir/mir.hpp"
#include "../backend.hpp"
#include "./var_replace.hpp"
#include "livevar_analyse.hpp"

namespace optimization::global_expr_move {

class Env {
 public:
  mir::inst::MirFunction& func;
  livevar_analyse::Livevar_Analyse& lva;
  var_replace::Var_Replace& vp;
  Env(mir::inst::MirFunction& func, livevar_analyse::Livevar_Analyse& lva,
      var_replace::Var_Replace& vp)
      : func(func), lva(lva), vp(vp) {}
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
  void optimize_func(mir::inst::MirFunction& func) {
    if (func.type->is_extern) {
      return;
    }
    livevar_analyse::Livevar_Analyse lva(func);
    lva.build();
    var_replace::Var_Replace vp(func);
    env = std::make_shared<Env>(func, lva, vp);
    for (auto& blkpair : func.basic_blks) {
      if (blkpair.first == 20) {
        std::cout << 'q';
      }
      std::cout << func.name << " - " << blkpair.first << std::endl;
      auto& block = blkpair.second;
      auto& blv = lva.livevars[blkpair.first];
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
            if (!opInst->lhs.is_immediate() && !opInst->rhs.is_immediate()) {
              auto lhs = std::get<mir::inst::VarId>(opInst->lhs);
              auto rhs = std::get<mir::inst::VarId>(opInst->rhs);
              if (!blv->live_vars_in->count(lhs) ||
                  !blv->live_vars_in->count(rhs)) {
                break;
              }
              id = find_common_live_blk(lhs, rhs);
            } else if (!opInst->lhs.is_immediate()) {
              auto lhs = std::get<mir::inst::VarId>(opInst->lhs);
              if (!blv->live_vars_in->count(lhs)) {
                break;
              }
              id = env->vp.defpoint.at(lhs).first;

            } else if (!opInst->rhs.is_immediate()) {
              auto rhs = std::get<mir::inst::VarId>(opInst->rhs);
              if (!blv->live_vars_in->count(rhs)) {
                break;
              }
              id = env->vp.defpoint.at(rhs).first;
            }
            if (id == blkpair.first) {
              break;
            }
            vp.setdefpoint(inst->dest, blkpair.first,
                           func.basic_blks.at(id).inst.size());
            func.variables.at(inst->dest.id).is_temp_var = false;
            func.basic_blks.at(id).inst.push_back(std::move(inst));
            iter = block.inst.erase(iter);
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
