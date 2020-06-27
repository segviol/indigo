#pragma once
#include "../../arm_code/arm.hpp"
#include "../../prelude/prelude.hpp"
#include "../backend.hpp"

namespace backend::codegen {

class Codegen final {
 public:
  Codegen(mir::inst::MirPackage& pkg) : package(pkg) {}

  /// Translate MirPackage into ArmCode
  arm::ArmCode translate();

 private:
  mir::inst::MirPackage& package;

  arm::Function translate_function(mir::inst::MirFunction& f);
};
}  // namespace backend::codegen
