#pragma once

#include "../backend.hpp"

namespace optimization::loop_expand {

class Const_Loop_Expand final : public backend::MirOptimizePass {
 public:
  std::string pass_name() const override { return "const_loop_expand"; }
  void optimize_mir(mir::inst::MirPackage &mir,
                    std::map<std::string, std::any> &extra_data_repo) override;

 private:
  void optimize_func(mir::inst::MirFunction &func);
};

}  // namespace optimization::loop_expand
