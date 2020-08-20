#include "mla.hpp"

#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "../../include/aixlog.hpp"

namespace optimization::mla {
using namespace mir::inst;
void MlaPass::optimize_mir(MirPackage &package,
                           std::map<std::string, std::any> &extra_data_repo) {
  for (auto &func : package.functions) {
    optimize_function(func.second);
  }
}

void MlaPass::optimize_function(MirFunction &func) {
  for (auto &bb : func.basic_blks) {
    auto mla_map = std::unordered_map<VarId, std::pair<OpInst, int>>();
    for (auto &inst_ : bb.second.inst) {
      if (auto inst = dynamic_cast<OpInst *>(&*inst_)) {
        if (inst->op == mir::inst::Op::Mul ||
            inst->op == mir::inst::Op::MulSh) {
          mla_map.insert({inst->dest, {*inst, 0}});
        }
      }
      auto use_var = inst_->useVars();
      for (auto x : use_var) {
        if (auto found = mla_map.find(x); found != mla_map.end()) {
          found->second.second++;
        }
      }
    }

    for (auto &inst : bb.second.inst) {
      auto &i = *inst;
      if (auto x = dynamic_cast<OpInst *>(&i)) {
        if (x->op == mir::inst::Op::Add && !x->lhs.is_immediate() &&
            !x->lhs.has_shift() && !x->rhs.is_immediate() &&
            !x->rhs.has_shift()) {
          // swap sides
          if (mla_map.find(std::get<1>(x->lhs)) != mla_map.end() &&
              mla_map.find(std::get<1>(x->rhs)) == mla_map.end()) {
            std::swap(x->lhs, x->rhs);
          }
          if (auto found = mla_map.find(std::get<1>(x->rhs));
              found != mla_map.end() && found->second.second == 1) {
            auto copy_of_add = *x;
            auto copy_of_mul = found->second.first;
            OpAcc op = mir::inst::OpAcc::MulAdd;
            if (copy_of_mul.op == mir::inst::Op::Mul) {
              op = mir::inst::OpAcc::MulAdd;
            } else if (copy_of_mul.op == mir::inst::Op::MulSh) {
              op = mir::inst::OpAcc::MulShAdd;
            }
            inst = std::make_unique<OpAccInst>(
                copy_of_add.dest, copy_of_mul.lhs, copy_of_mul.rhs,
                std::get<1>(copy_of_add.lhs), op);
          }
        }
      }
    }
  }
}
}  // namespace optimization::mla
