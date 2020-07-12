#include "math_opt.hpp"

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

using namespace arm;
using namespace backend::codegen;

void MathOptimization::optimize_arm(
    arm::ArmCode &arm_code, std::map<std::string, std::any> &extra_data_repo) {
  for (auto &f : arm_code.functions) {
    optimize_func(*f);
  }
}

void MathOptimization::optimize_func(arm::Function &f) {
  std::vector<std::unique_ptr<arm::Inst>> inst_new;

  for (auto &i : f.inst) {
    switch (i->op) {
        //   case arm::OpCode::Mul:
        // Optimize mul into shifts
      case arm::OpCode::SDiv: {
        auto i_ = dynamic_cast<arm::Arith3Inst *>(i.get());
        assert(i_ && "instruction must be Arith3Inst");
        // if (auto n = std::get_if<int32_t>(&i_->r2)) {
        //   if ((*n & (*n) - 1) == 0) {
        //     //
        //     inst_new.push_back(std::make_unique<Arith3Inst>(arm::OpCode::shr))
        // }
        // }
        inst_new.push_back(std::make_unique<arm::Arith2Inst>(
            arm::OpCode::Mov, arm::Reg(0), RegisterOperand(i_->r1)));
        inst_new.push_back(std::make_unique<arm::Arith2Inst>(
            arm::OpCode::Mov, arm::Reg(1), i_->r2));
        inst_new.push_back(
            std::make_unique<arm::BrInst>(arm::OpCode::Bl, "__aeabi_idiv"));
        inst_new.push_back(std::make_unique<arm::Arith2Inst>(
            arm::OpCode::Mov, i_->rd, RegisterOperand(0)));
        break;
      }
      case arm::OpCode::_Mod: {
        auto i_ = dynamic_cast<arm::Arith3Inst *>(i.get());
        assert(i_ && "instruction must be Arith3Inst");
        // if (auto n = std::get_if<int32_t>(&i_->r2)) {
        // } else {
        inst_new.push_back(std::make_unique<arm::Arith2Inst>(
            arm::OpCode::Mov, arm::Reg(0), RegisterOperand(i_->r1)));
        inst_new.push_back(std::make_unique<arm::Arith2Inst>(
            arm::OpCode::Mov, arm::Reg(1), i_->r2));
        inst_new.push_back(
            std::make_unique<arm::BrInst>(arm::OpCode::Bl, "__aeabi_idivmod"));
        inst_new.push_back(std::make_unique<arm::Arith2Inst>(
            arm::OpCode::Mov, i_->rd, RegisterOperand(1)));
        // }
        break;
      }
      default:
        inst_new.push_back(std::move(i));
    }
  }
  f.inst = std::move(inst_new);
}
