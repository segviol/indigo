#pragma once

#include "../../include/aixlog.hpp"
#include "../backend.hpp"

namespace optimization::func_array_global {

class Func_Array_Global final : public backend::MirOptimizePass {
 public:
  std::string pass_name() const override { return "Func array to global"; }
  void optimize_mir(mir::inst::MirPackage &mir,
                    std::map<std::string, std::any> &extra_data_repo) override;

 private:
  void optimize_func(mir::inst::MirFunction &func, mir::inst::MirPackage &mir);
};

}  // namespace optimization::func_array_global
