#pragma once
#include "../../prelude/prelude.hpp"
#include "../backend.hpp"

namespace backend::optimization {

class LoopUnroll final : public MirOptimizePass {
  std::string pass_name() const override { return "loop_unroll"; }
  void optimize_mir(mir::inst::MirPackage &package,
                    std::map<std::string, std::any> &extra_data_repo) override;
};

}  // namespace backend::optimization
