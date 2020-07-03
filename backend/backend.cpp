#include "backend.hpp"

#include <climits>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

#include "codegen/codegen.hpp"

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

void Backend::do_mir_to_arm_transform() {
  auto code = arm::ArmCode();
  for (auto& f : package.functions) {
    auto cg = codegen::Codegen(f.second, package, extra_data);
    auto arm_f = cg.translate_function();
    code.functions.push_back(std::make_unique<arm::Function>(std::move(arm_f)));
  }
  for (auto& v : package.global_values) {
    code.consts.insert({v.first, v.second});
  }
  arm_code.emplace(std::move(code));
}

arm::ArmCode Backend::generate_code() {
  do_mir_optimization();
  do_mir_to_arm_transform();
  do_arm_optimization();
  return std::move(arm_code.value());
}

}  // namespace backend
