#include "backend.hpp"

#include <climits>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

namespace backend {

void Backend::do_mir_optimization() {
  for (auto& pass : mir_passes) {
    pass->optimize_mir(package, extra_data);
  }
}

void Backend::do_arm_optimization() {
  for (auto& pass : arm_passes) {
    pass->optimize_arm(arm_code.value(), extra_data);
  }
}

void Backend::do_mir_to_arm_transform() {}

arm::ArmCode Backend::generate_code() {
  do_mir_optimization();
  do_mir_to_arm_transform();
  do_arm_optimization();
  return std::move(arm_code.value());
}

}  // namespace backend
