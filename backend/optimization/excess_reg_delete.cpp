#include "excess_reg_delete.hpp"

#include <memory>

#include "../../arm_code/arm.hpp"

namespace backend::optimization {
using namespace arm;
void ExcessRegDelete::optimize_arm(
    arm::ArmCode &arm_code, std::map<std::string, std::any> &extra_data_repo) {
  for (auto &f : arm_code.functions) {
    auto new_inst = std::vector<std::unique_ptr<arm::Inst>>();
    new_inst.reserve(f->inst.size());

    for (int i = 0; i < f->inst.size(); i++) {
      auto inst_ = &*f->inst[i];
      bool del = false;
      if (auto x = dynamic_cast<Arith2Inst *>(inst_)) {
        if (x->op == arm::OpCode::Mov && (x->r1 == x->r2)) {
          // Delete `mov rA, rA`
          del = true;
        }
      } else if (auto x = dynamic_cast<BrInst *>(inst_)) {
        if (i < f->inst.size() - 1) {
          if (auto x_ = dynamic_cast<LabelInst *>(&*f->inst[i + 1])) {
            if (x->cond == arm::ConditionCode::Always && x->l == x_->label) {
              // delete `b label1; label1:`
              del = true;
            }
          }
        }
        if (!del) {
          new_inst.push_back(std::move(f->inst[i]));
        }
      }
    }

    f->inst = std::move(new_inst);
  }
}

}  // namespace backend::optimization
