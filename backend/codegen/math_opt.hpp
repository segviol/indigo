#pragma once

#include <iterator>

#include "../backend.hpp"
namespace backend::codegen {

using InstList = std::vector<std::unique_ptr<arm::Inst>>;

class MathOptimization final : public backend::ArmOptimizePass {
 public:
  std::string pass_name() const override { return "math_optimization"; }
  void optimize_arm(arm::ArmCode &arm_code,
                    std::map<std::string, std::any> &extra_data_repo) override;

 private:
  void optimize_func(arm::Function &f);
  void make_mul(arm::Reg rd, arm::Reg lhs, arm::Operand2 rhs,
                std::insert_iterator<InstList> inserter);
  void make_div(arm::Reg rd, arm::Reg lhs, arm::Operand2 rhs,
                std::insert_iterator<InstList> inserter);
  void make_rem(arm::Reg rd, arm::Reg lhs, arm::Operand2 rhs,
                std::insert_iterator<InstList> inserter);
};
}  // namespace backend::codegen
