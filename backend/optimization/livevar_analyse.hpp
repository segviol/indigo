#include <assert.h>

#include <algorithm>
#include <map>
#include <memory>

#include "../backend.hpp"

namespace optimization::livevar_analyse {
class Block_Live_Var;
typedef uint32_t InstIdx;
typedef std::set<mir::inst::VarId> VariableSet;
typedef std::shared_ptr<VariableSet> sharedPtrVariableSet;
typedef std::shared_ptr<Block_Live_Var> sharedPtrBlkLivevar;
class Var_Point;

class Define_Use_Chain;
class Remove_Dead_Code;

typedef uint32_t InstIdx;
class Var_Point {
  mir::types::LabelId blk_id;
  mir::inst::Variable var;
  InstIdx instidx;
};
class Define_Use_Chain {
 public:
  Var_Point define;
  std::vector<Var_Point> uses;
};

// The class will ignore global variables and array variables
class Block_Live_Var {
 public:
  // instLiveVars[idx] means livevars before inst at idx
  std::vector<sharedPtrVariableSet> instLiveVars;
  sharedPtrVariableSet live_vars_out;
  sharedPtrVariableSet live_vars_in;
  std::set<sharedPtrVariableSet> subsequents;
  bool ignore_global_and_array = true;
  Block_Live_Var(int size = 0, bool ignore_global_and_array = true) {
    live_vars_out = std::make_shared<VariableSet>();
    live_vars_in = std::make_shared<VariableSet>();
    while (instLiveVars.size() < size) {
      instLiveVars.emplace_back(std::make_shared<VariableSet>());
    }
  }

  /*
  The function should be only called once to build the prceding relation.
  After that, the pointer will update automatically when it's sub
  */
  void add_subsequent(sharedPtrVariableSet subsequent) {
    subsequents.insert(subsequent);
  }

  // TODO: remove global and ptr
  std::set<mir::inst::VarId> filter_vars(
      std::set<mir::inst::VarId> vars,
      std::map<uint32_t, mir::inst::Variable> vartable) {  // remove global
    auto res = std::set(vars);
    for (auto iter = res.begin(); iter != res.end();) {
      if (vartable[iter->id].is_memory_var) {
        iter = res.erase(iter);
      } else {
        ++iter;
      }
    }
    return vars;
  }

  bool build(mir::inst::BasicBlk& block,
             std::map<uint32_t, mir::inst::Variable>& vartable) {
    assert(subsequents.size() <= 2 && subsequents.size() >= 0);
    auto live_vars_out_before = *live_vars_out;
    live_vars_out->clear();
    auto jump_use_vars = block.jump.useVars();
    live_vars_out->insert(jump_use_vars.begin(), jump_use_vars.end());
    if (subsequents.size() == 2) {
      auto first = subsequents.begin();
      auto second = subsequents.end();
      second--;
      std::set_union(
          first->get()->begin(), first->get()->end(), second->get()->begin(),
          second->get()->end(),
          std::inserter(*(live_vars_out), this->live_vars_out->begin()));
    } else if (subsequents.size() == 1) {
      auto iter = subsequents.begin();
      live_vars_out->insert(iter->get()->begin(), iter->get()->end());
    }
    if (std::equal(
            live_vars_out_before.begin(), live_vars_out_before.end(),
            live_vars_out->begin(),
            live_vars_out->end())) {  // if live_vars out dont't change, then
                                      // the block will not be modified
      return false;
    }
    auto tmp = live_vars_out;
    for (int i = instLiveVars.size() - 1; i >= 0; --i) {
      int size_before = instLiveVars[i]->size();
      auto useVars = block.inst[i]->useVars();
      auto defvar = block.inst[i]->dest;
      if (ignore_global_and_array) {
        useVars = filter_vars(useVars, vartable);
      }
      VariableSet defvars;
      if (vartable[defvar].ty->kind() != mir::types::TyKind::Void) {
        if (ignore_global_and_array && !vartable[defvar.id].is_memory_var ||
            !ignore_global_and_array)
          defvars.insert(defvar.id);
      }
      VariableSet diff_result;
      std::set_difference(tmp->begin(), tmp->end(), defvars.begin(),
                          defvars.end(),
                          std::inserter(diff_result, diff_result.begin()));
      std::set_union(useVars.begin(), useVars.end(), diff_result.begin(),
                     diff_result.end(),
                     std::inserter(*instLiveVars[i], instLiveVars[i]->begin()));
    }
    live_vars_in->clear();
    if (instLiveVars.size()) {
      live_vars_in->insert(instLiveVars[0]->begin(), instLiveVars[0]->end());
    } else {  // Maybe the block's codes are all dead
      live_vars_in->insert(live_vars_out->begin(), live_vars_out->end());
    }
    return true;
  }

  ~Block_Live_Var();
};

class Livevar_Analyse {
 public:
  std::map<mir::types::LabelId, sharedPtrBlkLivevar> livevars;
  std::map<mir::inst::VarId, std::set<mir::types::LabelId> > var_blks;
  mir::inst::MirFunction& func;
  Livevar_Analyse(mir::inst::MirFunction& func) : func(func) {
    for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
         ++iter) {
      auto blv = std::make_shared<Block_Live_Var>(iter->second.inst.size());
      livevars[iter->second.id] = blv;
    }
    for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
         ++iter) {  // init subsequnt relation
      auto blv = livevars[iter->second.id];
      for (auto prec : iter->second.preceding) {
        livevars[prec]->add_subsequent(blv->live_vars_in);
      }
    }
  }
  bool dfs_build(mir::inst::BasicBlk& start, sharedPtrVariableSet live_vars_out,
                 std::map<mir::types::LabelId, mir::inst::BasicBlk>& basic_blks,
                 std::map<uint32_t, mir::inst::Variable>& vartable) {
    bool modify = false;
    livevars[start.id]->build(basic_blks[start.id], vartable);
    for (auto pre : start.preceding) {
      modify |= livevars[pre]->build(basic_blks[pre], vartable);
    }
    return modify;
  }

  void build() {
    var_blks.clear();
    auto end = func.basic_blks.end();
    end--;
    sharedPtrVariableSet empty = std::make_shared<VariableSet>();
    while (dfs_build(end->second, empty, func.basic_blks, func.variables))
      ;
  }
};
}  // namespace optimization::livevar_analyse