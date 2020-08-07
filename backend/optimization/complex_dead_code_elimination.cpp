#include "complex_dead_code_elimination.hpp"

#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "aixlog.hpp"

namespace optimization::complex_dce {
using namespace mir::inst;
using namespace mir::types;

class ComplexDceRunner {
 public:
  ComplexDceRunner(MirFunction& f) : f(f) {}
  MirFunction& f;

  std::unordered_set<VarId> do_not_delete;
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
  void add_store_dependance(Value& dst, VarId src) {
    if (auto var = std::get_if<VarId>(&dst)) add_store_dependance(*var, src);
  }
  void add_store_dependance(Value& dst, Value& src) {
    if (auto dst_ = std::get_if<VarId>(&dst)) add_store_dependance(*dst_, src);
  }
  void add_ref_var(VarId id) { ref_vars.insert(id); }

  void scan_dependant_vars();
  void scan_bb_dependance_vars();
  void calc_remained_vars();
  void delete_excess_vars();

  void optimize_func() {
    LOG(TRACE) << "fn: " << f.name << std::endl;
    scan_bb_dependance_vars();
    scan_dependant_vars();
    display_dependance_maps();
    calc_remained_vars();
    LOG(TRACE) << "Result:" << std::endl;
    for (auto x : do_not_delete) {
      LOG(TRACE) << x << std::endl;
    }
    delete_excess_vars();
  }

  void display_dependance_maps() {
    LOG(TRACE) << "Dependance:" << std::endl;
    for (auto x : var_dependance) {
      LOG(TRACE) << x.first << " <- " << x.second << std::endl;
    }

    LOG(TRACE) << "Store dependance:" << std::endl;
    for (auto x : store_dependance) {
      LOG(TRACE) << x.first << " <- " << x.second << std::endl;
    }

    LOG(TRACE) << "Root:" << std::endl;
    for (auto x : do_not_delete) {
      LOG(TRACE) << x << std::endl;
    }
  }
};

void postorder_traverse_bb(std::vector<LabelId>& vec, std::set<LabelId>& vis,
                           MirFunction& f, LabelId id) {
  if (vis.find(id) != vis.end()) return;
  vis.insert(id);
  auto& jmp = f.basic_blks.at(id).jump;
  switch (jmp.kind) {
    case mir::inst::JumpInstructionKind::Br:
      postorder_traverse_bb(vec, vis, f, jmp.bb_true);
      break;
    case mir::inst::JumpInstructionKind::BrCond:
      postorder_traverse_bb(vec, vis, f, jmp.bb_true);
      postorder_traverse_bb(vec, vis, f, jmp.bb_false);
      break;
    default:
      break;
  }
  vec.push_back(id);
}

void ComplexDceRunner::scan_bb_dependance_vars() {
  // Calculate dominate nodes using algorithm specified in:
  // https://www.cs.rice.edu/~keith/EMBED/dom.pdf
  auto start_node = f.basic_blks.begin()->first;
  std::vector<LabelId> vec;
  {
    std::set<LabelId> vis;
    postorder_traverse_bb(vec, vis, f, start_node);
  }
  auto dom = std::map<mir::types::LabelId, std::set<mir::types::LabelId>>();
  auto all_nodes = std::set<LabelId>();
  for (auto& bb : f.basic_blks) {
    all_nodes.insert(bb.first);
  }
  for (auto& bb : f.basic_blks) {
    dom.insert({bb.first, std::set<mir::types::LabelId>(all_nodes)});
  }
  dom.at(start_node).insert(start_node);

  bool changed = true;
  while (changed) {
    changed = false;
    for (auto it = vec.crbegin(); it != vec.crbegin(); it++) {
      auto& pred = f.basic_blks.at(*it).preceding;
      std::set<LabelId> new_set;

      if (pred.size() == 0) {
      } else {
        auto pred_it = pred.begin();
        new_set = std::set<LabelId>(dom.at(*pred_it++));
        while (pred_it != pred.end()) {
          auto& dom_p = dom.at(*pred_it);
          auto it_set = new_set.begin();
          while (it_set != new_set.end()) {
            if (dom_p.find(*it_set) != dom_p.end()) {
              it_set++;
            } else {
              it_set = new_set.erase(it_set);
            }
          }
          pred_it++;
        }
      }
      new_set.insert(*it);
      auto& dom_set = dom.at(*it);
      if (dom_set != new_set) {
        changed = true;
        dom_set = std::move(new_set);
      }
    }
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
        do_not_delete.insert(x->dest);
      } else if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
        add_dependance(x->dest, x->src);
      } else if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
        add_dependance(x->dest, x->src);
      } else if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
        add_store_dependance(x->dest, x->val);
        // add_store_dependance(x->val, x->dest);
      } else if (auto x = dynamic_cast<mir::inst::LoadOffsetInst*>(&i)) {
        add_dependance(x->dest, x->src);
        add_dependance(x->dest, x->offset);
      } else if (auto x = dynamic_cast<mir::inst::StoreOffsetInst*>(&i)) {
        add_store_dependance(x->dest, x->val);
        add_store_dependance(x->dest, x->offset);
        add_store_dependance(x->offset, x->dest);
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
        do_not_delete.insert({ret.value()});
      }
    } else if (bb.second.jump.kind == mir::inst::JumpInstructionKind::BrCond) {
      auto ret = bb.second.jump.cond_or_ret;
      if (ret) {
        do_not_delete.insert({ret.value()});
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
    visited.insert(v);
    for (auto [it, end] = invert_dependance.equal_range(v); it != end; it++) {
      if (visited.find(it->second) == visited.end()) {
        remaining.push_back(it->second);
      }
    }
    for (auto [it, end] = store_dependance.equal_range(v); it != end; it++) {
      do_not_delete.insert(it->second);
      do_not_delete.insert(v);
    }
  }

  // Add all dependant of root vars
  remaining.clear();
  for (auto x : do_not_delete) remaining.push_back(x);
  while (!remaining.empty()) {
    auto v = remaining.front();
    remaining.pop_front();
    do_not_delete.insert(v);
    for (auto [it, end] = var_dependance.equal_range(v); it != end; it++) {
      if (do_not_delete.find(it->second) == do_not_delete.end()) {
        remaining.push_back(it->second);
      }
    }
  }
}

void ComplexDceRunner::delete_excess_vars() {
  for (auto& bb : f.basic_blks) {
    auto new_inst = std::vector<std::unique_ptr<Inst>>();
    for (auto& inst : bb.second.inst) {
      if (do_not_delete.find(inst->dest) != do_not_delete.end()) {
        new_inst.push_back(std::move(inst));
      }
    }
    bb.second.inst = std::move(new_inst);
  }
}

void ComplexDeadCodeElimination::optimize_mir(
    mir::inst::MirPackage& mir_pkg,
    std::map<std::string, std::any>& extra_data_repo) {
  for (auto& f : mir_pkg.functions) {
    if (f.second.type->is_extern) continue;
    auto dce = ComplexDceRunner(f.second);
    dce.optimize_func();
  }
}
}  // namespace optimization::complex_dce
