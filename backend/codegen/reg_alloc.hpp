#pragma once

#include "../backend.hpp"
namespace backend::codegen {
class RegAllocatePass final : public backend::ArmOptimizePass {
 public:
  std::string pass_name() const override { return "reg_alloc"; }
  void optimize_arm(arm::ArmCode &arm_code,
                    std::map<std::string, std::any> &extra_data_repo) override;

 private:
  void optimize_func(arm::Function &f);
};
}  // namespace backend::codegen
