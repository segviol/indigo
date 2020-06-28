#pragma once

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

std::map<mir::inst::VarId, std::shared_ptr<Define_Use_Chain> > ud_map;
std::map<mir::types::LabelId, Block_Live_Var&> livevars;

class Block_Live_Var {
 public:
  // instLiveVars[idx] means livevars before inst at idx
  std::vector<sharedPtrVariableSet> instLiveVars;
  sharedPtrVariableSet live_vars_out;
  Block_Live_Var(int size = 0) {
    live_vars_out = std::make_shared<VariableSet>();
    while (instLiveVars.size() < size) {
      instLiveVars.emplace_back(std::make_shared<VariableSet>());
    }
  }

  bool build(sharedPtrVariableSet live_vars_out, mir::inst::BasicBlk& block) {
    bool modify = false;
    // std::set_union(
    //     live_vars_out->begin(), live_vars_out->begin(),
    //     this->live_vars_out->begin(), this->live_vars_out->end(),
    //     std::inserter(*(this->live_vars_out), this->live_vars_out->begin()));
    auto tmp = this->live_vars_out;
    for (int i = instLiveVars.size() - 1; i >= 0; --i) {
      int size_before = instLiveVars[i]->size();
      auto useVars = block.inst[i]->useVars();
      auto defvar = block.inst[i]->dest;
      VariableSet defvars;
      if (defvar.ty->kind() != mir::types::TyKind::Void) {
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

    return modify;
  }
  bool remove_dead_code(mir::inst::BasicBlk& block) {
    bool modify = false;
    auto tmp = live_vars_out;
    for (auto iter = block.inst.begin(); iter != block.inst.end(); iter++) {
      auto defvar = iter->get()->dest;
      int idx = iter - block.inst.begin();
      if (defvar.ty->kind() == mir::types::TyKind::Void) {
        continue;
      }
      if (!live_vars_out->count(defvar.id)) {
        modify = true;
        iter = block.inst.erase(iter);
        instLiveVars.erase(instLiveVars.begin() + idx);
      }
    }
    // if (modify) {
    //   build(live_vars_out, block);
    // }
    return modify;
  }
  sharedPtrVariableSet get_live_vars_in() { return instLiveVars.front(); }
  ~Block_Live_Var();
};
class Remove_Dead_Code : public backend::MirOptimizePass {
  std::string name = "RemoveDeadCode";
  std::string pass_name() { return name; }

  bool dfs_build(
      mir::inst::BasicBlk& start, sharedPtrVariableSet live_vars_out,
      std::map<mir::types::LabelId, mir::inst::BasicBlk>& basic_blks) {
    bool modify = false;
    livevars[start.id].build(live_vars_out, basic_blks[start.id]);
    for (auto pre : start.preceding) {
      modify |= livevars[start.id].build(livevars[start.id].get_live_vars_in(),
                                         basic_blks[pre]);
    }
    return modify;
  }

  void optimize_func(mir::inst::MirFunction& func) {
    for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
         ++iter) {
      auto blv = Block_Live_Var(iter->second.inst.size());
      livevars[iter->second.id] = blv;
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
      optimize_func(iter->second);
    }
  }
};
}  // namespace optimization::ud_chain
