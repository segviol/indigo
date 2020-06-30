#pragma once

#include <assert.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

#include "../backend.hpp"

namespace optimization::ud_chain {

class Var_Point;

class Define_Use_Chain;
class Remove_Dead_Code;
class Block_Live_Var;

typedef uint32_t InstIdx;
typedef std::set<mir::inst::VarId> VariableSet;
typedef std::shared_ptr<VariableSet> sharedPtrVariableSet;
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

class Block_Live_Var {
 public:
  // instLiveVars[idx] means livevars before inst at idx
  std::vector<sharedPtrVariableSet> instLiveVars;
  sharedPtrVariableSet live_vars_out;
  sharedPtrVariableSet live_vars_in;
  std::set<sharedPtrVariableSet> subsequents;

  mir::inst::MirPackage& package_ref;
  mir::inst::MirFunction& function_ref;

  Block_Live_Var(Block_Live_Var&&) = default;

  Block_Live_Var(mir::inst::MirPackage& package_ref,
                 mir::inst::MirFunction& function_ref, int size = 0)
      : package_ref(package_ref), function_ref(function_ref) {
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

  mir::types::SharedTyPtr get_ty(mir::inst::VarId id) {
    // This function assumes variable is valid at all times.
    return function_ref.variables.find(id)->second.ty;
  }

  bool build(mir::inst::BasicBlk& block) {
    assert(subsequents.size() <= 2 && subsequents.size() >= 0);
    bool modify = false;
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
    auto tmp = live_vars_out;
    for (int i = instLiveVars.size() - 1; i >= 0; --i) {
      int size_before = instLiveVars[i]->size();
      auto useVars = block.inst[i]->useVars();
      auto defvar = block.inst[i]->dest;
      VariableSet defvars;
      if (get_ty(defvar)->kind() != mir::types::TyKind::Void) {
        defvars.insert(defvar.id);
      }
      VariableSet diff_result;
      std::set_difference(tmp->begin(), tmp->end(), defvars.begin(),
                          defvars.end(),
                          std::inserter(diff_result, diff_result.begin()));
      std::set_union(useVars.begin(), useVars.end(), diff_result.begin(),
                     diff_result.end(),
                     std::inserter(*instLiveVars[i], instLiveVars[i]->begin()));
      modify |= size_before == instLiveVars[i]->size() ? false : true;
    }
    live_vars_in->clear();
    if (instLiveVars.size()) {
      live_vars_in->insert(instLiveVars[0]->begin(), instLiveVars[0]->end());
    } else {  // Maybe the block's codes are all dead
      live_vars_in->insert(live_vars_out->begin(), live_vars_out->end());
    }
    return modify;
  }
  bool remove_dead_code(mir::inst::BasicBlk& block) {
    bool modify = false;
    auto tmp = live_vars_out;
    for (auto iter = block.inst.begin(); iter != block.inst.end();) {
      auto defvar = iter->get()->dest;
      int idx = iter - block.inst.begin();
      if (idx >= instLiveVars.size()) {  // remove the last code
        tmp = live_vars_out;
      } else {
        tmp = *(instLiveVars.begin() + idx);
      }
      if (get_ty(defvar)->kind() == mir::types::TyKind::Void) {
        continue;
      }
    }
    // if (modify) {
    //   build(live_vars_out, block);
    // }
    live_vars_in->clear();
    if (instLiveVars.size()) {
      live_vars_in->insert(instLiveVars[0]->begin(), instLiveVars[0]->end());
    } else {  // Maybe the block's codes are all dead
      live_vars_in->insert(live_vars_out->begin(), live_vars_out->end());
    }
    return modify;
  }
  ~Block_Live_Var();
};
class Remove_Dead_Code : public backend::MirOptimizePass {
  std::string name = "RemoveDeadCode";
  std::string pass_name() { return name; }

  std::map<mir::inst::VarId, std::shared_ptr<Define_Use_Chain> > ud_map;
  std::map<mir::types::LabelId, Block_Live_Var> livevars;

  bool dfs_build(
      mir::inst::BasicBlk& start, sharedPtrVariableSet live_vars_out,
      std::map<mir::types::LabelId, mir::inst::BasicBlk>& basic_blks) {
    bool modify = false;
    livevars[start.id].build(basic_blks[start.id]);
    for (auto pre : start.preceding) {
      modify |= livevars[pre].build(basic_blks[pre]);
    }
    return modify;
  }

  void optimize_func(mir::inst::MirFunction& func,
                     mir::inst::MirPackage& package_ref) {
    for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
         ++iter) {
      auto blv = Block_Live_Var(package_ref, func, iter->second.inst.size());
      livevars.insert({iter->second.id, std::move(blv)});
    }
    for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
         ++iter) {  // init subsequnt relation
      auto& blv = livevars[iter->second.id];
      for (auto prec : iter->second.preceding) {
        livevars[prec].add_subsequent(blv.live_vars_in);
      }
    }
    auto end = func.basic_blks.end();
    end--;
    sharedPtrVariableSet empty = std::make_shared<VariableSet>();
    while (dfs_build(end->second, empty, func.basic_blks))
      ;
    while (true) {  // delete death code for each block and build again util no
                    // death code to remove
      bool modify = false;
      for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
           iter++) {
        modify |= livevars[iter->first].remove_dead_code(iter->second);
      }
      if (!modify) {
        break;
      }
      while (dfs_build(end->second, empty, func.basic_blks))
        ;
    }
  }
  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         ++iter) {
      optimize_func(iter->second, package);
    }
  }
};
}  // namespace optimization::ud_chain
