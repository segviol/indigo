#pragma once

#include "../../include/aixlog.hpp"
#include "../backend.hpp"

namespace optimization::global_var_to_local {

class Global_Var_to_Local final : public backend::MirOptimizePass {
 public:
  std::string pass_name() const override { return "Global var to local"; }
  void optimize_mir(mir::inst::MirPackage &mir,
                    std::map<std::string, std::any> &extra_data_repo) override;

 private:
  void optimize_func(mir::inst::MirFunction &func,
                     std::set<std::string> &global_vars);
};

}  // namespace optimization::global_var_to_local
