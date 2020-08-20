#pragma once

#include "../../mir/mir.hpp"
#include "../backend.hpp"

namespace optimization::mla {
class MlaPass final : public backend::MirOptimizePass {
 public:
  std::string pass_name() const override { return "mla"; }
  void optimize_mir(mir::inst::MirPackage &package,
                    std::map<std::string, std::any> &extra_data_repo) override;

 private:
  void optimize_function(mir::inst::MirFunction &func);
};
}  // namespace optimization::mla
