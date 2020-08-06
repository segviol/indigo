#pragma once
#include "../backend.hpp"

namespace backend::optimization {

class ComplexDeadCodeElimination final : public MirOptimizePass {
 public:
  std::string pass_name() const override { return "complex_dce"; }
  void optimize_mir(mir::inst::MirPackage &,
                    std::map<std::string, std::any> &) override;
};

}  // namespace backend::optimization
