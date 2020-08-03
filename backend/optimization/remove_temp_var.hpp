#pragma once

#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "../../include/aixlog.hpp"
#include "../backend.hpp"
#include "livevar_analyse.hpp"

namespace optimization::remove_temp_var {
typedef mir::inst::Op Op;

class Env {
 public:
  mir::inst::MirFunction& func;
  Env(mir::inst::MirFunction& func) : func(func) {}
  bool is_temp_var(mir::inst::VarId var) {
    return func.variables.at(var.id).is_temp_var;
  }
  void remove_var(mir::inst::VarId var) { func.variables.erase(var.id); }
};
std::shared_ptr<Env> env;
class Remove_Temp_Var : public backend::MirOptimizePass {
 public:
  std::map<mir::inst::VarId, int> const_var_map;
  std::string name = "RemoveTempVar";
  std::string pass_name() const { return name; }

  void optimize_blk(mir::inst::BasicBlk& blk) {
    if (blk.inst.size() <= 1) {
      return;
    }
    for (auto iter = blk.inst.begin() + 1; iter != blk.inst.end();) {
      auto& inst1 = *(iter - 1);
      auto& inst = *iter;
      auto& i = *inst;
      if (inst->inst_kind() == mir::inst::InstKind::Assign) {
        auto assignInst = dynamic_cast<mir::inst::AssignInst*>(&i);
        if (!assignInst->src.is_immediate()) {
          auto var = std::get<mir::inst::VarId>(assignInst->src);
          if (env->is_temp_var(var) && inst1->dest == var) {
            inst1->dest = inst->dest;
            env->remove_var(var);
            iter = blk.inst.erase(iter);
            continue;
          }
        }
      }
      iter++;
    }
  }
  void optimize_func(mir::inst::MirFunction& func) {
    for (auto& blkpair : func.basic_blks) {
      env = std::make_shared<Env>(func);
      optimize_blk(blkpair.second);
    }
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         iter++) {
      optimize_func(iter->second);
    }
  }
};

}  // namespace optimization::remove_temp_var
