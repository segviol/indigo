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

namespace optimization::var_replace {

class Var_Replace {
 public:
  mir::inst::MirFunction& func;
  std::map<mir::inst::VarId, std::vector<std::pair<mir::types::LabelId, int>>>
      usepoints;
  std::map<mir::inst::VarId, std::pair<mir::types::LabelId, int>> defpoint;
  Var_Replace(mir::inst::MirFunction& func) : func(func) {
    for (auto& blkpair : func.basic_blks) {
      for (int i = 0; i < blkpair.second.inst.size(); i++) {
        auto& inst = blkpair.second.inst[i];
        auto& is = *inst;
        for (auto var : inst->useVars()) {
          add_usepoint(var, blkpair.first, i);
        }
        if (inst->inst_kind() == mir::inst::InstKind::Phi) {
          auto phiInst = dynamic_cast<mir::inst::PhiInst*>(&is);
          for (auto var : phiInst->vars) {
            func.variables.at(var.id).is_phi_var = true;
          }
          func.variables.at(phiInst->dest.id).is_phi_var = true;
        }
        setdefpoint(inst->dest, blkpair.first, i);
      }

      if (blkpair.second.jump.cond_or_ret.has_value()) {
        add_usepoint(blkpair.second.jump.cond_or_ret.value(), blkpair.first,
                     -1);
      }
    }
  }

  void add_usepoint(mir::inst::VarId var, mir::types::LabelId labelId,
                    int idx) {
    if (!usepoints.count(var)) {
      usepoints.insert(
          {var, std::vector<std::pair<mir::types::LabelId, int>>()});
    }
    usepoints.at(var).push_back({labelId, idx});
  }

  void replace(mir::inst::VarId from, mir::inst::VarId to) {
    if (from == to) {
      return;
    }
    if (func.variables.at(from).is_phi_var ||
        func.variables.at(to).is_phi_var) {
      return;
    }
    if (usepoints.count(from)) {
      for (auto pair : usepoints.at(from)) {
        if (pair.second == -1) {
          func.basic_blks.at(pair.first).jump.replace(from, to);
        } else {
          auto& inst = func.basic_blks.at(pair.first).inst.at(pair.second);
          inst->replace(from, to);
        }
      }
    }
  }

  void setdefpoint(mir::inst::VarId var, mir::types::LabelId labelId, int i) {
    defpoint.insert({var, {labelId, i}});
  }
};
}  // namespace optimization::var_replace
