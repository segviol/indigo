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
          del = true;
        } else {
        }
      } else {
      }
      if (!del) {
        new_inst.push_back(std::move(f->inst[i]));
      }
    }

    f->inst = std::move(new_inst);
  }
}

}  // namespace backend::optimization
