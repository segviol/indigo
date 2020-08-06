#include "complex_dead_code_elimination.hpp"

#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace backend::optimization {
using namespace mir::inst;

class ComplexDceRunner {
 public:
  ComplexDceRunner(MirFunction& f) : f(f) {}
  MirFunction& f;

  std::unordered_set<VarId> root_vars;
  std::unordered_set<VarId> ref_vars;
  std::unordered_multimap<mir::types::LabelId, VarId> bb_var_dependance;

  std::unordered_multimap<VarId, VarId> store_dependance;
  /// for a given VarId, all VarId that this value directly depends on
  std::unordered_multimap<VarId, VarId> var_dependance;
  /// for a given VarId, all VarId that depends on this value
  std::unordered_multimap<VarId, VarId> invert_dependance;

  void add_dependance(VarId dst, VarId src) {
    var_dependance.insert({dst, src});
    invert_dependance.insert({src, dst});
  }
  void add_dependance(VarId dst, Value& src) {
    if (auto var = std::get_if<VarId>(&src)) add_dependance(dst, *var);
  }
  void add_store_dependance(VarId dst, VarId src) {
    store_dependance.insert({dst, src});
  }
  void add_store_dependance(VarId dst, Value& src) {
    if (auto var = std::get_if<VarId>(&src)) add_store_dependance(dst, *var);
  }
  void add_store_dependance(Value& dst, Value& src) {
    if (auto dst_ = std::get_if<VarId>(&dst)) add_store_dependance(*dst_, src);
  }
  void add_ref_var(VarId id) { ref_vars.insert(id); }

  void scan_dependant_vars();
  void scan_bb_dependance_vars();
  void calc_remained_vars();
  void delete_excess_vars();
};

void ComplexDceRunner::scan_bb_dependance_vars() {
  for (auto& bb : f.basic_blks) {
  }
}

void ComplexDceRunner::scan_dependant_vars() {
  for (int i = 0; i < f.type->params.size(); i++) {
    if (f.type->params[i]->kind() == mir::types::TyKind::Ptr) {
      add_ref_var(i + 1);
    }
  }
  for (auto& bb : f.basic_blks) {
    for (auto& inst : bb.second.inst) {
      auto& i = *inst;
      if (auto x = dynamic_cast<mir::inst::OpInst*>(&i)) {
        add_dependance(x->dest, x->lhs);
        add_dependance(x->dest, x->rhs);
      } else if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
        for (auto v : x->params) add_dependance(x->dest, v);
      } else if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
        add_dependance(x->dest, x->src);
      } else if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
        add_dependance(x->dest, x->src);
      } else if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
        add_store_dependance(x->dest, x->val);
      } else if (auto x = dynamic_cast<mir::inst::LoadOffsetInst*>(&i)) {
        add_dependance(x->dest, x->src);
        add_dependance(x->dest, x->offset);
      } else if (auto x = dynamic_cast<mir::inst::StoreOffsetInst*>(&i)) {
        add_store_dependance(x->dest, x->val);
        add_store_dependance(x->offset, x->val);
      } else if (auto x = dynamic_cast<mir::inst::RefInst*>(&i)) {
        add_ref_var(i.dest);
      } else if (auto x = dynamic_cast<mir::inst::PhiInst*>(&i)) {
        for (auto v : x->vars) add_dependance(x->dest, v);
      } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst*>(&i)) {
        add_dependance(x->dest, x->ptr);
        add_dependance(x->dest, x->offset);
      } else {
        throw new std::bad_cast();
      }
    }
    if (bb.second.jump.kind == mir::inst::JumpInstructionKind::Return) {
      auto ret = bb.second.jump.cond_or_ret;
      if (ret) {
        root_vars.insert({ret.value()});
      }
    }
  }
}

void ComplexDceRunner::calc_remained_vars() {
  // Add all stored variables to root vars
  std::deque<VarId> remaining;
  std::unordered_set<VarId> visited;

  for (auto x : ref_vars) remaining.push_back(x);
  while (!remaining.empty()) {
    auto v = remaining.front();
    remaining.pop_front();
    for (auto [it, end] = invert_dependance.equal_range(v); it != end; it++) {
      if (visited.find(it->second) != visited.end()) {
        remaining.push_back(it->second);
        visited.insert(it->second);
      }
    }
    for (auto [it, end] = store_dependance.equal_range(v); it != end; it++) {
      root_vars.insert(it->second);
    }
  }

  // Add all dependant of root vars
  remaining.clear();
  for (auto x : root_vars) remaining.push_back(x);
  while (!remaining.empty()) {
    auto v = remaining.front();
    remaining.pop_front();
    for (auto [it, end] = var_dependance.equal_range(v); it != end; it++) {
      if (root_vars.find(it->second) != root_vars.end()) {
        remaining.push_back(it->second);
        root_vars.insert(it->second);
      }
    }
  }
}

void ComplexDceRunner::delete_excess_vars() {
  for (auto& bb : f.basic_blks) {
    auto new_inst = std::vector<std::unique_ptr<Inst>>();
    for (auto& inst : bb.second.inst) {
      if (root_vars.find(inst->dest) != root_vars.end()) {
        new_inst.push_back(std::move(inst));
      }
    }
    bb.second.inst = std::move(new_inst);
  }
}

void ComplexDeadCodeElimination::optimize_mir(
    mir::inst::MirPackage& mir_pkg,
    std::map<std::string, std::any>& extra_data_repo) {}
}  // namespace backend::optimization
