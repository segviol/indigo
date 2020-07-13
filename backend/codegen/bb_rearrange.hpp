#pragma once

#include "../backend.hpp"
namespace backend::codegen {
class BasicBlkRearrange final : public backend::MirOptimizePass {
 public:
  std::string pass_name() const override { return "bb_rearrange"; }
  void optimize_mir(mir::inst::MirPackage &mir,
                    std::map<std::string, std::any> &extra_data_repo) override;

 private:
  std::vector<uint32_t> optimize_func(mir::inst::MirFunction &f);
};
}  // namespace backend::codegen
