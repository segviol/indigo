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

namespace optimization::ref_count {
typedef mir::inst::Op Op;

class Env {
 public:
  mir::inst::MirFunction& func;
  Env(mir::inst::MirFunction& func) : func(func) {}
  void add_priority(mir::inst::VarId var, int pri) {
    func.variables[var.id].priority += pri;
  }
};
std::shared_ptr<Env> env;
class Ref_Count : public backend::MirOptimizePass {
 public:
  std::string name = "Ref count";
  const int loop_weight = 20;
  const int max_weight = 100000;
  const int min_weight = 1;
  std::string pass_name() const { return name; }

  void DFS(mir::inst::BasicBlk& startblk,
           std::set<mir::types::LabelId>& visited, int pri) {
    if (visited.count(startblk.id)) {
      return;
    }
    for (auto& inst : startblk.inst) {
      for (auto var : inst->useVars()) {
        env->add_priority(var, pri);
      }
      env->add_priority(inst->dest, pri);
    }
    auto& jump = startblk.jump;
    if (jump.cond_or_ret.has_value()) {
      env->add_priority(jump.cond_or_ret.value(), pri);
    }
    visited.insert(startblk.id);
    int bb_true_pri = pri;
    int bb_false_pri = pri;
    if (startblk.jump.jump_kind == mir::inst::JumpKind::Loop) {
      bb_true_pri = std::min(pri * loop_weight, max_weight);
      bb_false_pri = std::max(pri / loop_weight, max_weight);
    }
    if (jump.bb_true != -1) {
      DFS(env->func.basic_blks.at(jump.bb_true), visited, bb_true_pri);
    }
    if (jump.bb_false != -1) {
      DFS(env->func.basic_blks.at(jump.bb_false), visited, bb_false_pri);
    }
  }

  void optimize_func(mir::inst::MirFunction& func) {
    if (func.type->is_extern) {
      return;
    }
    env = std::make_shared<Env>(func);
    std::set<mir::types::LabelId> visited;
    DFS(func.basic_blks.begin()->second, visited, 1);
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         iter++) {
      optimize_func(iter->second);
    }
  }
};

}  // namespace optimization::ref_count
