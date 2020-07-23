#pragma once
#include "../backend.hpp"

namespace backend::optimization {

class ExcessRegDelete final : public ArmOptimizePass {
 public:
  std::string pass_name() const override { return "ExcessRegDelete"; }
  void optimize_arm(arm::ArmCode &arm_code,
                    std::map<std::string, std::any> &extra_data_repo) override;
};

}  // namespace backend::optimization
