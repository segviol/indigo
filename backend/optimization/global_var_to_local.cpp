#include "./global_var_to_local.hpp"

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "../../include/aixlog.hpp"
#include "../../mir/mir.hpp"
#include "../optimization/optimization.hpp"
#include "./var_replace.hpp"

namespace optimization::global_var_to_local {

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

mir::inst::VarId get_new_id(mir::inst::MirFunction& f, mir::inst::Variable v) {
  auto end = f.variables.end();
  end--;
  auto id = mir::inst::VarId(end->first + 1);
  f.variables.insert({id.id, v});
  return id;
}

void Global_Var_to_Local::optimize_mir(
    mir::inst::MirPackage& mir,
    std::map<std::string, std::any>& extra_data_repo) {
  std::set<std::string> global_vars;
  for (auto& p : mir.global_values) {
    if (p.second.index() == 0) {
      global_vars.insert(p.first);
    }
  }
  for (auto& f : mir.functions) {
    if (f.second.type->is_extern || f.first == "$$5_main" || f.first == "main")
      continue;
    optimize_func(f.second, global_vars);
  }
  if (!mir.functions.count("$$5_main")) {
    return;
  }
  auto& f = mir.functions.at("$$5_main");
  if (!f.basic_blks.size()) {
    return;
  }
  auto& startblk = f.basic_blks.begin()->second;
  mir::inst::VarId dest;
  for (auto s : global_vars) {
    int idx = 0;
    for (; idx < startblk.inst.size(); idx++) {
      auto& inst = startblk.inst.at(idx);
      auto& i = *inst;
      if (inst->inst_kind() == mir::inst::InstKind::Ref) {
        auto ref = dynamic_cast<mir::inst::RefInst*>(&i);
        if (ref->val.index() == 1 && std::get<std::string>(ref->val) == s) {
          dest = ref->dest;
          break;
        }
      }
    }
    if (idx == startblk.inst.size()) {
      continue;
    }
    auto var = get_new_id(f, mir::inst::Variable(mir::types::new_int_ty()));
    LOG(TRACE) << "convert global var  " << s << " to local var " << var
               << " in " << f.name << std::endl;
    startblk.inst.insert(startblk.inst.begin() + idx + 1,
                         std::make_unique<mir::inst::LoadInst>(dest, var));
    var_replace::Var_Replace vp(f);
    for (auto& usepoint : vp.usepoints.at(dest)) {
      if (usepoint.first == startblk.id && usepoint.second == idx + 1) {
        continue;
      }
      auto& inst = vp.get_usepoint(usepoint);
      if (inst->inst_kind() == mir::inst::InstKind::Store) {
        auto& i = *inst;
        auto ist = dynamic_cast<mir::inst::StoreOffsetInst*>(&i);
        if (!ist->val.is_immediate() &&
            !f.variables.at(std::get<mir::inst::VarId>(ist->val).id)
                 .is_phi_var) {
          vp.replace((std::get<mir::inst::VarId>(ist->val).id), var);
          inst = std::make_unique<mir::inst::AssignInst>(var, var);
        } else {
          inst = std::make_unique<mir::inst::AssignInst>(var, ist->val);
        }
      } else {
        auto& i = *inst;
        auto ist = dynamic_cast<mir::inst::LoadOffsetInst*>(&i);
        if (!f.variables.at(ist->dest.id).is_phi_var) {
          vp.replace(ist->dest, var);
          inst = std::make_unique<mir::inst::AssignInst>(var, var);
        } else {
          inst = std::make_unique<mir::inst::AssignInst>(ist->dest, var);
        }
      }
    }
  }
}

void Global_Var_to_Local::optimize_func(mir::inst::MirFunction& func,
                                        std::set<std::string>& global_vars) {
  if (!global_vars.size() || !func.basic_blks.size()) {
    return;
  }
  auto& insts = func.basic_blks.begin()->second.inst;
  for (auto& inst : insts) {
    if (inst->inst_kind() == mir::inst::InstKind::Ref) {
      auto& i = *inst;
      auto ref = dynamic_cast<mir::inst::RefInst*>(&i);
      if (ref->val.index() == 1) {
        auto s = std::get<std::string>(ref->val);
        if (global_vars.count(s)) {
          global_vars.erase(s);
        }
      }
    }
  }
}

}  // namespace optimization::global_var_to_local
