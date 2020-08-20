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
        } else if (x->op == arm::OpCode::Mov && new_inst.size() > 0) {
          // Delete `mov rA, rB; mov rA, rB;`
          if (auto x1 = dynamic_cast<Arith2Inst *>(&*new_inst.back())) {
            if (x1->op == arm::OpCode::Mov && x->r1 == x1->r1 &&
                x->r2 == x1->r2 && x->cond == x1->cond) {
              del = true;
            }
          }
        }
      } else if (auto x = dynamic_cast<BrInst *>(inst_)) {
        // thether the branch is always or not, it has no effect
        if (i < f->inst.size() - 1) {
          if (auto x_ = dynamic_cast<LabelInst *>(&*f->inst[i + 1])) {
            if (x->l == x_->label) {
              // delete `b label1;` before `label1:`
              del = true;
            }
          }
        }
        if (x->cond != arm::ConditionCode::Always && i < f->inst.size() - 2) {
          // simplify `bA label1; b label2; label1:` to `b~A label2; label1:`
          auto x1 = dynamic_cast<BrInst *>(&*f->inst[i + 1]);
          auto x2 = dynamic_cast<LabelInst *>(&*f->inst[i + 2]);
          if (x1 && x2) {
            if (x->l == x2->label && (x1->cond == arm::ConditionCode::Always ||
                                      x1->cond == invert_cond(x->cond))) {
              x1->cond = invert_cond(x->cond);
              del = true;
            }
          }
        }
      } else if (auto x = dynamic_cast<LoadStoreInst *>(inst_);
                 x && new_inst.size() >= 1) {
        // Simplify `ldr xA, memA; str xA, memA`
        if (auto x1 = dynamic_cast<LoadStoreInst *>(&*new_inst.back())) {
          if (x1->op == arm::OpCode::LdR && x->op == arm::OpCode::StR &&
              x->mem == x1->mem && x->cond == x1->cond && x->rd == x1->rd) {
            del = true;
          }
        }
      }
      //   const std::set<OpCode> shifts = {OpCode::Lsl, OpCode::Lsr,
      //   OpCode::Asr};
      //   // Simplify `shift rA, #n; ldr/str rB, [rD, rA]`
      //   if (auto r2 = std::get_if<int32_t>(&x->r2);
      //       x->cond == arm::ConditionCode::Always && r2 &&
      //       shifts.find(x->op) != shifts.end() && i < f->inst.size() - 1)
      //       {
      //     if (auto x_ = dynamic_cast<LoadStoreInst *>(&*f->inst[i + 1]))
      //     {
      //       if (auto mem = std::get_if<MemoryOperand>(&x_->mem)) {
      //         if (auto off = std::get_if<RegisterOperand>(&mem->offset))
      //         {
      //           off->shift_amount += *r2;
      //           del = true;
      //         }
      //       }
      //     }
      //   }

      if (!del) {
        new_inst.push_back(std::move(f->inst[i]));
      }
    }

    f->inst = std::move(new_inst);
  }
}

}  // namespace backend::optimization
