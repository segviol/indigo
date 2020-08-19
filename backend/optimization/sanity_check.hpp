#pragma once

#include <stdexcept>

#include "../backend.hpp"
#include "aixlog.hpp"

namespace optimization::sanity_check {

class SanityCheck final : public backend::MirOptimizePass {
 public:
  std::string pass_name() const override { return "const_loop_expand"; }
  void optimize_mir(mir::inst::MirPackage &mir,
                    std::map<std::string, std::any> &extra_data_repo) override {
    for (auto &f : mir.functions) {
      if (!f.second.type->is_extern) {
        optimize_func(f.second);
      }
    }
    if (failed) throw std::logic_error("Sanity check failed! please see log");
  }

  bool failed = false;

 private:
  void optimize_func(mir::inst::MirFunction &func) {
    auto check = [&](mir::inst::VarId id, int bb) {
      if (func.variables.find(id) == func.variables.end()) {
        LOG(FATAL) << "No such variable: " << id << " (in: " << func.name
                   << ", bb " << bb << ")" << std::endl;
        failed = true;
      }
    };
    auto check_val = [&](mir::inst::Value val, int bb) {
      if (auto id = val.get_if<mir::inst::VarId>()) {
        check(*id, bb);
      }
    };
    for (auto &bb : func.basic_blks) {
      for (auto &inst : bb.second.inst) {
        auto &i = *inst;
        check(i.dest, bb.first);
        if (auto x = dynamic_cast<mir::inst::OpInst *>(&i)) {
          check_val(x->lhs, bb.first);
          check_val(x->rhs, bb.first);
        } else if (auto x = dynamic_cast<mir::inst::CallInst *>(&i)) {
          for (auto v : x->params) check_val(v, bb.first);

        } else if (auto x = dynamic_cast<mir::inst::AssignInst *>(&i)) {
          check_val(x->src, bb.first);
        } else if (auto x = dynamic_cast<mir::inst::LoadInst *>(&i)) {
          check_val(x->src, bb.first);
        } else if (auto x = dynamic_cast<mir::inst::StoreInst *>(&i)) {
          check_val(x->val, bb.first);

        } else if (auto x = dynamic_cast<mir::inst::LoadOffsetInst *>(&i)) {
          check_val(x->src, bb.first);
          check_val(x->offset, bb.first);

        } else if (auto x = dynamic_cast<mir::inst::StoreOffsetInst *>(&i)) {
          check_val(x->val, bb.first);
          check_val(x->offset, bb.first);
        } else if (auto x = dynamic_cast<mir::inst::RefInst *>(&i)) {
          if (x->val.index() == 0)
            check_val(std::get<0>(x->val), bb.first);
          else {
          }
        } else if (auto x = dynamic_cast<mir::inst::PhiInst *>(&i)) {
          for (auto v : x->vars) check(v, bb.first);
        } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst *>(&i)) {
          check_val(x->ptr, bb.first);
          check_val(x->offset, bb.first);
        } else {
          throw new std::bad_cast();
        }
      }
    }
  }
};

}  // namespace optimization::sanity_check
