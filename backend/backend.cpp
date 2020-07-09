#include "backend.hpp"

#include <climits>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

#include "../include/spdlog/include/spdlog/spdlog.h"
#include "codegen/codegen.hpp"

namespace backend {

void Backend::do_mir_optimization() {
  for (auto& pass : mir_passes) {
    spdlog::info("Running MIR pass: {}", pass->pass_name());
    pass->optimize_mir(package, extra_data);
    if (options.show_code_after_each_pass) {
      spdlog::info("Code after pass: {}", pass->pass_name());
      std::cout << package << std::endl;
    }
  }
}

void Backend::do_arm_optimization() {
  for (auto& pass : arm_passes) {
    spdlog::info("Running ARM pass: {}", pass->pass_name());
    pass->optimize_arm(arm_code.value(), extra_data);
    if (options.show_code_after_each_pass) {
      spdlog::info("Code after pass: {}", pass->pass_name());
      std::cout << arm_code.value() << std::endl;
    }
  }
}

void Backend::do_mir_to_arm_transform() {
  spdlog::info("Doing  mir->arm  transform");
  auto code = arm::ArmCode();
  for (auto& f : package.functions) {
    if (f.second.type->is_extern) continue;
    auto cg = codegen::Codegen(f.second, package, extra_data);
    auto arm_f = cg.translate_function();
    code.functions.push_back(std::make_unique<arm::Function>(std::move(arm_f)));
  }
  for (auto& v : package.global_values) {
    code.consts.insert({v.first, v.second});
  }
  arm_code.emplace(std::move(code));
  if (options.show_code_after_each_pass) {
    spdlog::info("Code after transformation");
    std::cout << arm_code.value() << std::endl;
  }
}

arm::ArmCode Backend::generate_code() {
  do_mir_optimization();
  do_mir_to_arm_transform();
  do_arm_optimization();
  return std::move(arm_code.value());
}

}  // namespace backend
