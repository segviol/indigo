#pragma once

#include <assert.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

#include "../backend.hpp"
#include "livevar_analyse.hpp"

namespace optimization::remove_dead_code {

class Var_Point;

class Define_Use_Chain;
class Remove_Dead_Code;

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

class Remove_Dead_Code : public backend::MirOptimizePass {
  std::string name = "RemoveDeadCode";
  std::string pass_name() { return name; }
  std::map<mir::types::LabelId,
           std::shared_ptr<livevar_analyse::Livevar_Analyse> >
      func_livevar_analyse;
  bool remove_dead_code(std::shared_ptr<livevar_analyse::Block_Live_Var> blv,
                        mir::inst::BasicBlk& block) {
    bool modify = false;
    auto tmp = blv->live_vars_out;
    for (auto iter = block.inst.begin(); iter != block.inst.end();) {
      auto defvar = iter->get()->dest;
      int idx = iter - block.inst.begin();
      if (idx + 1 >= blv->instLiveVars.size()) {  // remove the last code
        tmp = blv->live_vars_out;
      } else {
        tmp = *(blv->instLiveVars.begin() + idx + 1);
      }
      if (defvar.ty->kind() == mir::types::TyKind::Void) {
        continue;
      }
    }
    // if (modify) {
    //   build(live_vars_out, block);
    // }
    blv->live_vars_in->clear();
    if (blv->instLiveVars.size()) {
      blv->live_vars_in->insert(blv->instLiveVars[0]->begin(),
                                blv->instLiveVars[0]->end());
    } else {  // Maybe the block's codes are all dead
      blv->live_vars_in->insert(blv->live_vars_out->begin(),
                                blv->live_vars_out->end());
    }
    return modify;
  }

  void optimize_func(mir::types::LabelId funcId, mir::inst::MirFunction& func) {
    while (true) {  // delete death code for each block and build again util no
                    // death code to remove
      bool modify = false;
      for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
           iter++) {
        modify |= remove_dead_code(
            func_livevar_analyse[funcId]->livevars[iter->first], iter->second);
      }
      if (!modify) {
        break;
      }
      func_livevar_analyse[funcId]->build(func);
    }
  }
  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         ++iter) {
      optimize_func(iter->first, iter->second);
    }
  }
};
}  // namespace optimization::remove_dead_code
