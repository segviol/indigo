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

#include "../../mir/mir.hpp"
#include "../backend.hpp"

namespace inlineFunc {

class Rewriter {
 public:
  mir::inst::MirFunction& func;
  mir::inst::MirFunction& subfunc;
  // cast from subfunc to func
  std::map<int, int> var_cast_map;
  std::map<int, int> label_cast_map;
  std::vector<std::unique_ptr<mir::inst::Inst>> init_inst_before;
  std::vector<std::unique_ptr<mir::inst::Inst>> init_inst_after;
  int varId;
  int labelId;

  Rewriter(mir::inst::MirFunction& func, mir::inst::MirFunction& subfunc,
           mir::inst::CallInst& callInst)
      : func(func), subfunc(subfunc) {
    auto var_end_iter = func.variables.end();
    var_end_iter--;
    varId = var_end_iter->first;
    auto label_end_iter = func.basic_blks.end();
    label_end_iter--;
    label_end_iter--;
    labelId = label_end_iter->first;
    for (int i = 0; i < callInst.params.size(); i++) {
      int para = i + 1;
      if (callInst.params[i].index() == 0) {
        auto imm = std::get<int>(callInst.params[i]);
        auto destId = get_new_varId();
        init_inst_before.push_back(
            std::make_unique<mir::inst::AssignInst>(destId, imm));
        var_cast_map[para] = destId.id;
      } else {
        auto para_varId = std::get<mir::inst::VarId>(callInst.params[i]);
        var_cast_map[para] = para_varId.id;
      }
    }
    int ret = 0;
    auto destId = get_new_varId();
    var_cast_map[ret] = destId.id;
    init_inst_after.push_back(
        std::make_unique<mir::inst::AssignInst>(callInst.dest, destId));
    for (auto iter = subfunc.variables.begin(); iter != subfunc.variables.end();
         ++iter) {
      if (!var_cast_map.count(iter->first)) {
        auto new_id = get_new_varId().id;
        var_cast_map[iter->first] = new_id;
        func.variables.insert(
            std::make_pair(new_id, subfunc.variables.at(iter->first)));
      }
    }
    for (auto iter = subfunc.basic_blks.begin();
         iter != subfunc.basic_blks.end(); ++iter) {
      if (!label_cast_map.count(iter->first)) {
        label_cast_map[iter->first] = get_new_labelId();
      }
    }
  }

  mir::inst::Value cast_value(mir::inst::Value val) {
    if (val.index() == 0) {
      return std::get<int>(val);
    }
    return var_cast_map.at(std::get<mir::inst::VarId>(val).id);
  }

  mir::inst::VarId cast_varId(mir::inst::VarId var) {
    return mir::inst::VarId(var_cast_map.at(var.id));
  }

  void cast_block(mir::inst::BasicBlk& blk) {
    mir::inst::BasicBlk new_blk(label_cast_map[blk.id]);
    for (auto pre : blk.preceding) {
      new_blk.preceding.insert(label_cast_map.at(pre));
    }

    for (auto& inst : blk.inst) {
      auto kind = inst->inst_kind();
      auto& i = *inst;
      switch (kind) {
        case mir::inst::InstKind::Ref: {
          auto refInst = dynamic_cast<mir::inst::RefInst*>(&i);
          auto val = refInst->val;
          std::variant<mir::inst::VarId, std::string> _val;
          if (val.index() == 0) {
            _val = mir::inst::VarId(
                var_cast_map.at(std::get<mir::inst::VarId>(val).id));
          } else {
            _val = std::get<std::string>(val);
          }
          new_blk.inst.push_back(std::make_unique<mir::inst::RefInst>(
              cast_varId(refInst->dest), _val));
          break;
        }
        case mir::inst::InstKind::Op: {
          auto opInst = dynamic_cast<mir::inst::OpInst*>(&i);
          new_blk.inst.push_back(std::make_unique<mir::inst::OpInst>(
              cast_varId(opInst->dest), cast_value(opInst->lhs),
              cast_value(opInst->lhs), opInst->op));
          break;
        }
        case mir::inst::InstKind::Assign: {
          auto assignInst = dynamic_cast<mir::inst::AssignInst*>(&i);
          new_blk.inst.push_back(std::make_unique<mir::inst::AssignInst>(
              cast_varId(assignInst->dest), cast_value(assignInst->src)));
          break;
        }
        case mir::inst::InstKind::Load: {
          auto loadInst = dynamic_cast<mir::inst::LoadInst*>(&i);
          new_blk.inst.push_back(std::make_unique<mir::inst::LoadInst>(
              cast_value(loadInst->src), cast_varId(loadInst->dest)));
          break;
        }
        case mir::inst::InstKind::Store: {
          auto storeInst = dynamic_cast<mir::inst::StoreInst*>(&i);
          new_blk.inst.push_back(std::make_unique<mir::inst::LoadInst>(
              cast_value(storeInst->val), cast_varId(storeInst->dest)));
          break;
        }
        case mir::inst::InstKind::Call: {
          auto callInst = dynamic_cast<mir::inst::CallInst*>(&i);
          std::vector<mir::inst::Value> paras;
          for (auto val : callInst->params) {
            paras.push_back(cast_value(val));
          }
          new_blk.inst.push_back(std::make_unique<mir::inst::CallInst>(
              cast_varId(callInst->dest), callInst->func, paras));
          break;
        }
        case mir::inst::InstKind::PtrOffset: {
          auto ptrInst = dynamic_cast<mir::inst::PtrOffsetInst*>(&i);
          new_blk.inst.push_back(std::make_unique<mir::inst::PtrOffsetInst>(
              cast_varId(ptrInst->dest), cast_varId(ptrInst->ptr),
              cast_value(ptrInst->offset)));
          break;
        }
        case mir::inst::InstKind::Phi: {
          auto phiInst = dynamic_cast<mir::inst::PhiInst*>(&i);
          std::vector<mir::inst::VarId> vars;
          for (auto vaR : phiInst->vars) {
            vars.push_back(cast_varId(vaR));
          }
          new_blk.inst.push_back(std::make_unique<mir::inst::CallInst>(
              cast_varId(phiInst->dest), vars));
          break;
        }

        default:
          std::cout << "error!" << std::endl;
      }
    }

    void excute_jump() {}

    std::optional<mir::inst::VarId> cond_or_ret = {};
    if (blk.jump.cond_or_ret.has_value()) {
      cond_or_ret = blk.jump.cond_or_ret.value();
    }
    new_blk.jump = mir::inst::JumpInstruction(blk.jump.kind, blk.jump.bb_true,
                                              blk.jump.bb_false, cond_or_ret);
    if (func.basic_blks.count(blk.id)) {
      func.basic_blks.insert(std::make_pair(new_blk.id, new_blk));
    }
  }

  mir::inst::VarId get_new_varId() { return mir::inst::VarId(++varId); }

  int get_new_labelId() { return ++labelId; }
};  // namespace inlineFunc

class Inline_Func : public backend::MirOptimizePass {
 public:
  std::set<std::string> uninlineable_funcs;

  // void init_map(mir::inst::MirPackage& package) {
  //   for (auto iter = package.functions.begin(); iter !=
  //   package.functions.end();
  //        iter++) {
  //     for (auto blkiter = iter->second.basic_blks.begin();
  //          blkiter != iter->second.basic_blks.end(); ++blkiter) {
  //       auto& insts = blkiter->second.inst;
  //       for(auto )
  //     }
  //   }
  // }

  void optimize_func(std::string funcId, mir::inst::MirFunction& func,
                     std::map<std::string, mir::inst::MirFunction>& funcTable) {
    int cur_blkId = func.basic_blks.begin()->first;
    for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
         ++iter) {
      auto& insts = iter->second.inst;
      for (int i = 0; i < insts.size(); i++) {
        auto& inst = insts[i];
        if (inst->inst_kind() != mir::inst::InstKind::Call) {
          continue;
        }
        auto& iptr = *inst;
        auto callInst = dynamic_cast<mir::inst::CallInst*>(&iptr);
        auto& subfunc = funcTable.at(callInst->func);
        if (uninlineable_funcs.count(func.name)) {
          continue;
        }
        if (subfunc.type->is_extern) {
          continue;
        }
        if (subfunc.variables.size() + func.variables.size() >= 1024) {
          continue;
        }
        if (callInst->func == func.name) {
          uninlineable_funcs.insert(func.name);
          continue;
        }
        //  Then this func is inlineable
        Rewriter rwt(func, subfunc, *callInst);
        for (auto iter = subfunc.basic_blks.begin();
             iter != subfunc.basic_blks.end(); iter++) {
          rwt.cast_block(iter->second);
        }
        auto sub_start_id =
            rwt.label_cast_map.at(subfunc.basic_blks.begin()->first);
        auto end_iter = subfunc.basic_blks.end();
        end_iter--;
        auto sub_end_id = rwt.label_cast_map.at(end_iter->first);
        int new_id;  // the preceding half of the block (when the block is the
                     // starting block, the new_id should smaller than the
                     // original one)
        if (iter == func.basic_blks.begin()) {  // the first
        }
      }
    }
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         iter++) {
      optimize_func(iter->first, iter->second, package.functions);
    }
  }
};

}  // namespace inlineFunc