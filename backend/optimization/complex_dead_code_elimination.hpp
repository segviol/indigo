#pragma once
#include "../backend.hpp"

namespace optimization::complex_dce {

class ComplexDeadCodeElimination final : public backend::MirOptimizePass {
 public:
  std::string pass_name() const override { return "complex_dce"; }
  void optimize_mir(mir::inst::MirPackage &,
                    std::map<std::string, std::any> &) override;
};

}  // namespace optimization::complex_dce
