#include "complex_dead_code_elimination.hpp"

#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace backend::optimization {
using namespace mir::inst;

class ComplexDceRunner {
 public:
  ComplexDceRunner(MirFunction& f) : f(f) {}
  MirFunction& f;

  std::unordered_set<VarId> root_vars;
  std::unordered_set<VarId> ref_vars;
  std::unordered_map<mir::types::LabelId, VarId> bb_var_dependance;

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
  void add_ref_var(VarId id) { ref_vars.insert(id); }

  void scan_dependant_vars();
  void scan_bb_dependance_vars();
};

void ComplexDceRunner::scan_dependant_vars() {
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
      } else if (auto x = dynamic_cast<mir::inst::LoadOffsetInst*>(&i)) {
        add_dependance(x->dest, x->src);
        add_dependance(x->dest, x->offset);
      } else if (auto x = dynamic_cast<mir::inst::StoreOffsetInst*>(&i)) {
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
  }
}

void ComplexDeadCodeElimination::optimize_mir(
    mir::inst::MirPackage& mir_pkg,
    std::map<std::string, std::any>& extra_data_repo) {}
}  // namespace backend::optimization
