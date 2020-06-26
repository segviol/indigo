#include "codegen.hpp"

#include <iterator>
#include <map>
#include <memory>
#include <vector>

namespace backend::codegen {

void Codegen::do_mir_optimization() {
  for (auto& pass : mir_passes) {
    pass->optimize_mir(package, extra_data);
  }
}

void Codegen::do_arm_optimization() {
  for (auto& pass : arm_passes) {
    pass->optimize_arm(arm_code.value(), extra_data);
  }
}

void Codegen::do_mir_to_arm_transform() {
  // TODO!
}

ArmCode Codegen::generate_code() {
  do_mir_optimization();
  do_mir_to_arm_transform();
  do_arm_optimization();
  return std::move(arm_code.value());
}

}  // namespace backend::codegen
