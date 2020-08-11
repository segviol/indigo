#include "./func_array_global.hpp"

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "../../include/aixlog.hpp"
#include "../../mir/mir.hpp"
#include "../optimization/optimization.hpp"
#include "./var_replace.hpp"

namespace optimization::func_array_global {

struct GlobalVarManager {
  mir::inst::MirPackage& mir;
  GlobalVarManager(mir::inst::MirPackage& mir) : mir(mir) {}
  std::string add_global_var(std::string func, mir::inst::VarId var,
                             arm::ConstValue val) {
    std::string s = "$$6_" + func + "_" + std::to_string(var.id);
    mir.global_values.insert({s, val});
    return s;
  }

  mir::inst::MirFunction& get_init_func() { return mir.functions.at("main"); }
  mir::inst::VarId get_new_init_id(mir::inst::Variable variable) {
    auto& f = get_init_func();
    auto end_iter = f.variables.end();
    end_iter--;
    auto id = end_iter->first;
    id++;
    f.variables.insert({id, variable});
    return mir::inst::VarId(id);
  }
};

void Func_Array_Global::optimize_mir(
    mir::inst::MirPackage& mir,
    std::map<std::string, std::any>& extra_data_repo) {
  for (auto& f : mir.functions) {
    if (f.second.type->is_extern) continue;
    optimize_func(f.second, mir);
  }
}

void Func_Array_Global::optimize_func(mir::inst::MirFunction& func,
                                      mir::inst::MirPackage& mir) {
  if (!func.basic_blks.size() || func.name == "$$5_main") {
    return;
  }
  auto& startBlk = func.basic_blks.begin()->second;
  var_replace::Var_Replace vp(func);
  std::set<mir::inst::VarId> ref_results;
  for (auto& inst : startBlk.inst) {
    if (inst->inst_kind() == mir::inst::InstKind::Ref &&
        inst->useVars().size()) {
      ref_results.insert(inst->dest);
    }
  }
  for (auto& inst : startBlk.inst) {
    if (inst->inst_kind() == mir::inst::InstKind::Store) {
      auto vars = inst->useVars();
      if (vars.size() > 1) {  // store must be all const(except addr)
        for (auto var : vars) {
          if (ref_results.count(var)) {
            ref_results.erase(var);
          }
        }
      }
    }
  }

  for (auto& blk : func.basic_blks) {
    if (blk.first == startBlk.id) {
      continue;
    }
    for (auto& inst : blk.second.inst) {
      if (inst->inst_kind() == mir::inst::InstKind::Store ||
          inst->inst_kind() == mir::inst::InstKind::Call) {
        for (auto var : ref_results) {
          if (inst->useVars().count(var)) {
            ref_results.erase(var);
            break;
          }
        }
      }
    }
  }

  if (ref_results.size()) {
    GlobalVarManager gvm(mir);
    std::vector<std::unique_ptr<mir::inst::Inst>> init_insts;
    std::vector<std::unique_ptr<mir::inst::Inst>> call_insts;
    bool after_call = false;
    auto& init_func = gvm.get_init_func();
    auto& init_blk = init_func.basic_blks.begin()->second;
    for (auto& inst : init_blk.inst) {
      if (inst->inst_kind() == mir::inst::InstKind::Call) {
        after_call = true;
      }
      if (after_call) {
        call_insts.push_back(std::move(inst));
      } else {
        init_insts.push_back(std::move(inst));
      }
    }
    init_blk.inst.clear();
    for (auto var : ref_results) {
      auto def_point = vp.defpoint.at(var);
      auto& inst =
          func.basic_blks.at(def_point.first).inst.at(def_point.second);
      auto ref_var = *inst->useVars().begin();
      std::vector<uint32_t> val(func.variables.at(ref_var.id).size() / 4, 0);
      auto name = gvm.add_global_var(func.name, ref_var, val);
      LOG(TRACE) << "convert array " << ref_var << "in " << func.name
                 << " to global array " << name << "" << std::endl;
      inst = std::make_unique<mir::inst::RefInst>(var, name);
      auto init_var = gvm.get_new_init_id(func.variables.at(var.id));
      init_insts.push_back(
          std::make_unique<mir::inst::RefInst>(init_var, name));
      func.variables.erase(ref_var);

      for (auto iter = startBlk.inst.begin(); iter != startBlk.inst.end();) {
        auto& inst = *iter;
        if (inst->useVars().count(var) &&
            inst->inst_kind() == mir::inst::InstKind::Store) {
          auto ptr = std::unique_ptr<mir::inst::Inst>(inst->deep_copy());
          ptr->replace(var, init_var);
          init_insts.push_back(std::move(ptr));
          iter = startBlk.inst.erase(iter);
        } else {
          iter++;
        }
      }
    }

    for (auto& inst : init_insts) {
      init_blk.inst.push_back(std::move(inst));
    }

    for (auto& inst : call_insts) {
      init_blk.inst.push_back(std::move(inst));
    }
  }
}

}  // namespace optimization::func_array_global
