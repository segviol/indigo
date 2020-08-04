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

namespace optimization::inlineFunc {

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
  int cur_blkId;
  int sub_endId;
  int sub_starttId;
  const int Inline_Var_Priority = 0;
  Rewriter(mir::inst::MirFunction& func, mir::inst::MirFunction& subfunc,
           mir::inst::CallInst& callInst, int cur_blkId)
      : func(func), subfunc(subfunc), cur_blkId(cur_blkId) {
    auto var_end_iter = func.variables.end();
    var_end_iter--;
    while (var_end_iter->first > 1000000) {
      var_end_iter--;
    }
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
        func.variables[destId] = subfunc.variables.at(para);
        func.variables[destId].priority = Inline_Var_Priority;
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
    if (subfunc.variables.count(ret)) {
      func.variables[destId.id] = subfunc.variables.at(ret);
    } else {
      func.variables[destId.id] = func.variables.at(callInst.dest.id);
    }
    init_inst_after.push_back(
        std::make_unique<mir::inst::AssignInst>(callInst.dest, destId));
    for (auto iter = subfunc.variables.begin(); iter != subfunc.variables.end();
         ++iter) {
      if (!var_cast_map.count(iter->first)) {
        auto new_id = get_new_varId().id;
        var_cast_map[iter->first] = new_id;
        func.variables.insert(
            std::make_pair(new_id, subfunc.variables.at(iter->first)));
        func.variables[new_id].priority = Inline_Var_Priority;
      }
    }
    for (auto iter = subfunc.basic_blks.begin();
         iter != subfunc.basic_blks.end(); ++iter) {
      int new_id;

      new_id = get_new_labelId();
      if (!label_cast_map.count(iter->first)) {
        label_cast_map[iter->first] = new_id;
      }
    }
    auto iter = subfunc.basic_blks.end();
    iter--;
    sub_starttId = label_cast_map.at(subfunc.basic_blks.begin()->first);
    sub_endId = label_cast_map.at(iter->first);
  }

  mir::inst::Value cast_value(mir::inst::Value val) {
    if (val.index() == 0) {
      return std::get<int>(val);
    }
    return mir::inst::VarId(
        var_cast_map.at(std::get<mir::inst::VarId>(val).id));
  }

  mir::inst::VarId cast_varId(mir::inst::VarId var) {
    return mir::inst::VarId(var_cast_map.at(var.id));
  }

  void cast_block(mir::inst::BasicBlk& blk) {
    auto new_id = label_cast_map.at(blk.id);
    func.basic_blks.insert(std::make_pair(new_id, mir::inst::BasicBlk(new_id)));
    auto& new_blk = func.basic_blks.at(new_id);
    for (auto pre : blk.preceding) {
      auto new_pre = label_cast_map.at(pre);
      if (new_pre == sub_starttId) {
        new_pre = cur_blkId;
      }
      new_blk.preceding.insert(new_pre);
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
              cast_value(opInst->rhs), opInst->op));
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
          new_blk.inst.push_back(std::make_unique<mir::inst::StoreInst>(
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
          new_blk.inst.push_back(std::make_unique<mir::inst::PhiInst>(
              cast_varId(phiInst->dest), vars));
          break;
        }

        default:
          LOG(TRACE) << "error!" << std::endl;
      }
    }

    std::optional<mir::inst::VarId> cond_or_ret = {};
    auto bb_true = blk.jump.bb_true;
    auto bb_false = blk.jump.bb_false;
    if (label_cast_map.count(bb_true)) {
      bb_true = label_cast_map.at(bb_true);
      // if (bb_true == sub_endId) {
      //   bb_true = cur_blkId;
      // }
    }

    if (label_cast_map.count(bb_false)) {
      bb_false = label_cast_map.at(bb_false);
      // if (bb_false == sub_endId) {
      //   bb_false = cur_blkId;
      // }
    }

    if (blk.jump.cond_or_ret.has_value()) {
      cond_or_ret = blk.jump.cond_or_ret.value();
      cond_or_ret = mir::inst::VarId(var_cast_map.at(cond_or_ret.value().id));
    }
    new_blk.jump = mir::inst::JumpInstruction(blk.jump.kind, bb_true, bb_false,
                                              cond_or_ret, blk.jump.jump_kind);
  }

  mir::inst::VarId get_new_varId() { return mir::inst::VarId(++varId); }

  int get_new_labelId() { return ++labelId; }
};  // namespace inlineFunc

class Inline_Func : public backend::MirOptimizePass {
 public:
  std::set<std::string> uninlineable_funcs;
  std::string name = "InlineFunction";
  bool recursively;
  Inline_Func(bool recursively = true) : recursively(recursively) {}
  std::string pass_name() const { return name; }

  void init(mir::inst::MirPackage& package) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         iter++) {
      bool flag = false;
      for (auto& blkpair : iter->second.basic_blks) {
        if (blkpair.second.jump.jump_kind == mir::inst::JumpKind::Loop) {
          flag = true;
          uninlineable_funcs.insert(iter->first);
          break;
        }
        auto& insts = blkpair.second.inst;
        for (auto& inst : insts) {
          if (inst->inst_kind() != mir::inst::InstKind::Call) {
            continue;
          }
          auto& iptr = *inst;
          auto callInst = dynamic_cast<mir::inst::CallInst*>(&iptr);
          if (callInst->func == iter->first) {
            uninlineable_funcs.insert(iter->first);
            flag = true;
            break;
          }
        }
        if (flag) {
          break;
        }
      }
    }
  }

  // bool is_inlineable(mir::inst::MirFunction& func) {
  //   if (uninlineable_funcs.count(func.name)) {
  //     return false;
  //   }
  // }

  void optimize_func(std::string funcId, mir::inst::MirFunction& func,
                     std::map<std::string, mir::inst::MirFunction>& funcTable) {
    if (func.name == "main" || func.type->is_extern) {
      return;
    }
    int cur_blkId = func.basic_blks.begin()->first;
    std::set<mir::types::LabelId> base_labels;
    for (auto& iter : func.basic_blks) {
      base_labels.insert(iter.first);
    }
    while (true) {
      auto iter = func.basic_blks.begin();
      for (; iter != func.basic_blks.end(); ++iter) {
        bool flag = false;
        if (iter->first < cur_blkId || !base_labels.count(iter->first)) {
          // if (iter->first < cur_blkId || !base_labels.count(iter->first)) {

          continue;
        }
        auto& insts = iter->second.inst;
        auto& cur_blk = iter->second;
        for (int i = 0; i < insts.size(); i++) {
          auto& inst = insts[i];
          if (inst->inst_kind() != mir::inst::InstKind::Call) {
            continue;
          }
          auto& iptr = *inst;
          auto callInst = dynamic_cast<mir::inst::CallInst*>(&iptr);
          auto& subfunc = funcTable.at(callInst->func);
          if (uninlineable_funcs.count(subfunc.name)) {
            continue;
          }
          if (subfunc.type->is_extern) {
            continue;
          }
          if (subfunc.variables.size() + func.variables.size() >= 256) {
            continue;
          }
          //  Then this func is inlineable
          LOG(TRACE) << "( inline_func ) " << func.name << " inlines "
                     << subfunc.name << std::endl;
          Rewriter rwt(func, subfunc, *callInst, cur_blk.id);
          for (auto& pair : subfunc.basic_blks) {
            rwt.cast_block(pair.second);
          }
          auto sub_start_id =
              rwt.label_cast_map.at(subfunc.basic_blks.begin()->first);
          auto end_iter = subfunc.basic_blks.end();
          end_iter--;
          auto sub_end_id = rwt.label_cast_map.at(end_iter->first);
          auto& start_blk = func.basic_blks.at(sub_start_id);

          std::vector<std::unique_ptr<mir::inst::Inst>> end_blk_insts;
          auto& end_blk = func.basic_blks.at(sub_end_id);
          for (auto& j : end_blk.inst) {
            end_blk_insts.push_back(std::move(j));
          }

          for (auto& j : rwt.init_inst_after) {
            end_blk_insts.push_back(std::move(j));
          }
          for (int j = i + 1; j < insts.size(); j++) {
            end_blk_insts.push_back(std::move(insts[j]));
          }
          end_blk.inst.clear();
          for (auto& j : end_blk_insts) {
            end_blk.inst.push_back(std::move(j));
          }
          end_blk.jump = mir::inst::JumpInstruction(
              cur_blk.jump.kind, cur_blk.jump.bb_true, cur_blk.jump.bb_false,
              cur_blk.jump.cond_or_ret, cur_blk.jump.jump_kind);
          int bb_true = cur_blk.jump.bb_true;
          if (bb_true != -1) {
            auto& blk = func.basic_blks.at(bb_true);
            blk.preceding.erase(cur_blk.id);
            blk.preceding.insert(end_blk.id);
          }
          int bb_false = cur_blk.jump.bb_false;
          if (bb_false != -1) {
            auto& blk = func.basic_blks.at(bb_false);
            blk.preceding.erase(cur_blk.id);
            blk.preceding.insert(end_blk.id);
          }

          std::vector<std::unique_ptr<mir::inst::Inst>> start_blk_insts;
          for (int j = 0; j < i; j++) {
            start_blk_insts.push_back(std::move(insts[j]));
          }
          for (auto& j : rwt.init_inst_before) {
            start_blk_insts.push_back(std::move(j));
          }
          for (auto& j : start_blk.inst) {
            start_blk_insts.push_back(std::move(j));
          }
          cur_blk.inst.clear();
          for (auto& j : start_blk_insts) {
            cur_blk.inst.push_back(std::move(j));
          }
          cur_blk.jump = mir::inst::JumpInstruction(
              start_blk.jump.kind, start_blk.jump.bb_true,
              start_blk.jump.bb_false, start_blk.jump.cond_or_ret,
              start_blk.jump.jump_kind);

          flag = true;
          func.basic_blks.erase(sub_start_id);
          break;
        }
        if (flag) {
          cur_blkId = iter->first;
          break;
        }
      }
      if (iter == func.basic_blks.end()) {
        break;
      }
    }
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    init(package);
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         iter++) {
      optimize_func(iter->first, iter->second, package.functions);
    }
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         iter++) {
      LOG(TRACE) << iter->second.name << " variables size is "
                 << iter->second.variables.size() << std::endl;
    }
  }
};

}  // namespace optimization::inlineFunc
