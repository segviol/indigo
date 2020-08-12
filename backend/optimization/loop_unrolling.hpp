#pragma once

#include "../backend.hpp"

namespace optimization::loop_unrolling {

class Loop_Unrolling final : public backend::MirOptimizePass {
 public:
  std::string pass_name() const override { return "Loop unrolling"; }
  void optimize_mir(mir::inst::MirPackage &mir,
                    std::map<std::string, std::any> &extra_data_repo) override;

 private:
  void optimize_func(mir::inst::MirFunction &func);
};

}  // namespace optimization::loop_unrolling
