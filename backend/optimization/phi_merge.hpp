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
#include <utility>
#include <vector>

#include "../../include/aixlog.hpp"
#include "../backend.hpp"
#include "livevar_analyse.hpp"

namespace optimization::phi_merge {
typedef int color;  //
typedef std::map<mir::inst::VarId, color> Color_Map;
class Phi_Merge {
 public:
  std::map<mir::inst::VarId, mir::inst::VarId> merged_var_Map;
  std::map<mir::inst::VarId, std::set<mir::inst::VarId>> merged_Map;
  mir::inst::MirFunction& func;
  int varId;
  u_int color_num = 8;
  Phi_Merge(mir::inst::MirFunction& func) : func(func) {
    if (func.variables.size()) {
      auto end_iter = func.variables.end();
      end_iter--;
      varId = end_iter->first;
    } else {
      varId = mir::inst::VarId(65535);
    }
    for (auto& blkiter : func.basic_blks) {
      for (auto& inst : blkiter.second.inst) {
        if (inst->inst_kind() == mir::inst::InstKind::Phi) {
          mir::inst::VarId new_VarId(0);
          for (auto var : inst->useVars()) {
            if (merged_var_Map.count(var)) {
              new_VarId = merged_var_Map.at(var);
              break;
            }
          }
          if (new_VarId.id == 0) {
            if (merged_var_Map.count(inst->dest)) {
              new_VarId = merged_var_Map.at(inst->dest);
            } else {
              new_VarId = get_new_VarId();
            }
          }
          merge(inst->dest, new_VarId);
          for (auto var : inst->useVars()) {
            merge(var, new_VarId);
          }
        }
      }
    }
  }
  mir::inst::VarId get_new_VarId() { return mir::inst::VarId(++varId); }
  void merge(mir::inst::VarId var1, mir::inst::VarId var2) {
    if (!merged_Map.count(var2)) {
      merged_Map[var2] = std::set<mir::inst::VarId>();
    }
    if (merged_var_Map.count(var1)) {
      auto old_merged_var = merged_var_Map.at(var1);
      if (old_merged_var == var2) {
        return;
      }
      for (auto& var : merged_Map.at(old_merged_var)) {
        merged_var_Map[var] = var2;
        merged_Map[var2].insert(var);
      }
      merged_Map.erase(old_merged_var);
    } else {
      merged_var_Map[var1] = var2;
      merged_Map[var2].insert(var1);
    }
  }

  mir::inst::VarId get_actual_var(mir::inst::VarId var) {
    if (merged_var_Map.count(var)) {
      return merged_var_Map.at(var);
    }
    return var;
  }
};
}  // namespace optimization::phi_merge