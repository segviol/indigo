#pragma once

#include "../backend.hpp"

namespace optimization::init_end_block {
class InitEndBlock final : public backend::MirOptimizePass {
 public:
  std::string pass_name() const override { return "Init End Block"; }
  void optimize_mir(mir::inst::MirPackage &mir,
                    std::map<std::string, std::any> &extra_data_repo) override;

 private:
  void optimize_func(mir::inst::MirFunction &f);
};
}  // namespace optimization::init_end_block
