#pragma once
// #ifndef Livevar_Analyse
// #define Livevar_Analyse
#include <assert.h>
#include <stdint.h>

#include <algorithm>
#include <list>
#include <map>
#include <memory>

#include "../../mir/mir.hpp"
#include "../backend.hpp"

namespace optimization::livevar_analyse {
class Block_Live_Var;
typedef uint32_t InstIdx;
typedef std::set<mir::inst::VarId> VariableSet;
typedef std::shared_ptr<VariableSet> sharedPtrVariableSet;
typedef std::shared_ptr<Block_Live_Var> sharedPtrBlkLivevar;
// class Var_Point;

// class Define_Use_Chain;
// class Remove_Dead_Code;

// class Var_Point {
//   mir::types::LabelId blk_id;
//   mir::inst::Variable var;
//   InstIdx instidx;
// };
// class Define_Use_Chain {
//  public:
//   Var_Point define;
//   std::vector<Var_Point> uses;
// };

// The class will ignore global variables and array variables
class Block_Live_Var {
 public:
  // instLiveVars[idx] means livevars before inst at idx
  std::vector<sharedPtrVariableSet> instLiveVars;
  sharedPtrVariableSet live_vars_out_ignoring_jump;
  sharedPtrVariableSet live_vars_out;
  sharedPtrVariableSet live_vars_in;
  std::set<sharedPtrVariableSet> subsequents;
  sharedPtrVariableSet defvars;
  bool first_build = true;
  mir::inst::BasicBlk& block;
  bool strict;
  std::map<uint32_t, mir::inst::Variable>& vartable;
  Block_Live_Var(mir::inst::BasicBlk& block,
                 std::map<uint32_t, mir::inst::Variable>& vartable,
                 bool strict = false)
      : block(block), vartable(vartable), strict(strict) {
    auto size = block.inst.size();
    live_vars_out = std::make_shared<VariableSet>();
    live_vars_in = std::make_shared<VariableSet>();
    defvars = std::make_shared<VariableSet>();
    live_vars_out_ignoring_jump = std::make_shared<VariableSet>();
    while (instLiveVars.size() < size) {
      instLiveVars.emplace_back(std::make_shared<VariableSet>());
    }
  }

  /*
  The function is called to build the prceding relation.
  After that, the pointer will update automatically when it's sub
  */
  void add_subsequent(sharedPtrVariableSet subsequent) {
    subsequents.insert(subsequent);
  }

  mir::types::TyKind queryTy(mir::inst::VarId var) {
    return vartable.at(var).ty->kind();
  }

  // TODO: remove global and ptr
  // std::set<mir::inst::VarId> filter_vars(
  //     std::set<mir::inst::VarId> vars,
  //     std::map<uint32_t, mir::inst::Variable> vartable) {  // remove global
  //   auto res = std::set(vars);
  //   for (auto iter = res.begin(); iter != res.end();) {
  //     if (vartable[iter->id].is_memory_var) {
  //       iter = res.erase(iter);
  //     } else {
  //       ++iter;
  //     }
  //   }
  //   return vars;
  // }

  void update(int idx) {
    if (idx < 0) {
      return;
    }
    auto tmp = live_vars_out;
    if (idx + 1 < instLiveVars.size()) {
      tmp = instLiveVars[idx + 1];
    }
    int size_before = instLiveVars[idx]->size();
    auto useVars = block.inst[idx]->useVars();
    auto defvar = block.inst[idx]->dest;
    // if (ignore_global_and_array) {
    //   useVars = filter_vars(useVars, vartable);
    // }
    VariableSet defvars;
    if (vartable[defvar.id].ty->kind() != mir::types::TyKind::Void &&
        block.inst[idx]->inst_kind() != mir::inst::InstKind::Store) {
      defvars.insert(defvar);
      this->defvars->insert(defvar);
    }
    if (strict) {
      if (block.inst.at(idx)->inst_kind() == mir::inst::InstKind::Phi) {
        for (auto iter = useVars.begin(); iter != useVars.end(); iter++) {
          if (this->defvars->count(*iter)) {
            useVars.erase(iter);
            break;
          }
        }
      }
    }

    VariableSet diff_result;
    instLiveVars[idx]->clear();
    std::set_difference(tmp->begin(), tmp->end(), defvars.begin(),
                        defvars.end(),
                        std::inserter(diff_result, diff_result.begin()));
    std::set_union(
        useVars.begin(), useVars.end(), diff_result.begin(), diff_result.end(),
        std::inserter(*instLiveVars[idx], instLiveVars[idx]->begin()));
  }

  bool build() {
    assert(subsequents.size() <= 2 && subsequents.size() >= 0);
    auto live_vars_out_before = *live_vars_out_ignoring_jump;
    live_vars_out_ignoring_jump->clear();

    if (subsequents.size() == 2) {
      auto first = subsequents.begin();
      auto second = subsequents.end();
      second--;
      std::set_union(first->get()->begin(), first->get()->end(),
                     second->get()->begin(), second->get()->end(),
                     std::inserter(*(live_vars_out_ignoring_jump),
                                   this->live_vars_out_ignoring_jump->begin()));
    } else if (subsequents.size() == 1) {
      auto iter = subsequents.begin();
      live_vars_out_ignoring_jump->insert(iter->get()->begin(),
                                          iter->get()->end());
    }
    // LOG(TRACE) << live_vars_out_before.size() << "  " <<
    // live_vars_out->size()
    //           << std::endl;
    if (!first_build &&
        *live_vars_out_ignoring_jump ==
            live_vars_out_before) {  // if live_vars out dont't change, then
                                     // the block will not be modified
      return false;
    }
    auto jump_use_vars = block.jump.useVars();
    live_vars_out->clear();
    live_vars_out->insert(live_vars_out_ignoring_jump->begin(),
                          live_vars_out_ignoring_jump->end());
    live_vars_out->insert(jump_use_vars.begin(), jump_use_vars.end());
    auto tmp = live_vars_out;
    for (int i = instLiveVars.size() - 1; i >= 0; --i) {
      update(i);
    }
    live_vars_in->clear();
    if (instLiveVars.size()) {
      live_vars_in->insert(instLiveVars[0]->begin(), instLiveVars[0]->end());
    } else {  // Maybe the block's codes are all dead
      live_vars_in->insert(live_vars_out->begin(), live_vars_out->end());
    }
    first_build = false;
    return true;
  }

  ~Block_Live_Var(){};
};

class Livevar_Analyse {
 public:
  std::map<mir::types::LabelId, sharedPtrBlkLivevar> livevars;
  bool strict;
  mir::inst::MirFunction& func;
  Livevar_Analyse(mir::inst::MirFunction& func, bool strcit = false)
      : func(func), strict(strcit) {
    auto& vartable = func.variables;
    for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
         ++iter) {
      auto& block = iter->second;
      auto blv = std::make_shared<Block_Live_Var>(block, vartable);
      livevars[iter->second.id] = blv;
    }
    for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
         ++iter) {  // init subsequnt relation
      auto blv = livevars[iter->second.id];
      for (auto prec : iter->second.preceding) {
        // if (!livevars.count(prec)) {
        //   continue;
        // }
        livevars[prec]->add_subsequent(blv->live_vars_in);
      }
    }
  }
  ~Livevar_Analyse(){};
  bool bfs_build(mir::inst::BasicBlk& start) {
    std::list<mir::types::LabelId> queue;
    std::set<mir::types::LabelId> visited;
    queue.push_back(start.id);
    bool modify = false;
    while (!queue.empty()) {
      auto id = queue.front();
      if (!visited.count(id)) {
        modify |= livevars.at(id)->build();
        visited.insert(id);
        for (auto pre : livevars.at(id)->block.preceding) {
          queue.push_back(pre);
        }
      }
      queue.pop_front();
    }

    return modify;
  }

  void build() {
    if (!func.basic_blks.size()) {
      return;
    }
    auto end = func.basic_blks.end();
    end--;
    sharedPtrVariableSet empty = std::make_shared<VariableSet>();
    while (bfs_build(end->second))
      ;
    // for (auto& pair : livevars) {
    //   LOG(TRACE) << pair.first << ": " << std::endl;
    //   for (auto var : *pair.second->live_vars_in) {
    //     LOG(TRACE) << var << ", ";
    //   }
    //   LOG(TRACE) << std::endl;
    //   for (auto ilv : pair.second->instLiveVars) {
    //     for (auto var : *ilv) {
    //       LOG(TRACE) << var << ", ";
    //     }
    //     LOG(TRACE) << std::endl;
    //   }
    //   LOG(TRACE) << "******************" << std::endl;
    // }
  }
};  // namespace optimization::livevar_analyse
}  // namespace optimization::livevar_analyse
// #endif
