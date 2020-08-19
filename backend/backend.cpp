#include "backend.hpp"

#include <climits>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

#include "../include/aixlog.hpp"
#include "codegen/codegen.hpp"

namespace backend {

bool Backend::should_run_pass(std::string&& pass_name) {
  auto pass = pass_name;
  return should_run_pass(pass);
}

bool Backend::should_run_pass(std::string& pass_name) {
  if (options.run_pass &&
      (options.run_pass->find(pass_name) == options.run_pass->end())) {
    return false;
  } else {
    if (options.skip_pass.find(pass_name) != options.skip_pass.end()) {
      return false;
    } else {
      return true;
    }
  }
}

void Backend::do_mir_optimization() {
  for (auto& pass : mir_passes) {
    if (!should_run_pass(pass->pass_name())) {
      LOG(INFO) << "Skipping MIR pass: " << pass->pass_name() << "\n";
      continue;
    }

    LOG(INFO) << "Running MIR pass: " << pass->pass_name() << "\n";
    pass->optimize_mir(package, extra_data);
    if (options.show_code_after_each_pass) {
      LOG(INFO) << "Code after pass: " << pass->pass_name() << "\n";
      std::cout << package << std::endl;
    }
  }
}

void Backend::do_arm_optimization() {
  for (auto& pass : arm_passes) {
    if (!should_run_pass(pass->pass_name())) {
      LOG(INFO) << "Skipping ARM pass: " << pass->pass_name() << "\n";
      continue;
    }

    LOG(INFO) << "Running ARM pass: " << pass->pass_name() << "\n";
    pass->optimize_arm(arm_code.value(), extra_data);
    if (options.show_code_after_each_pass) {
      LOG(INFO) << "Code after pass: " << pass->pass_name() << "\n";
      std::cout << arm_code.value() << std::endl;
    }
  }
}

void Backend::do_mir_to_arm_transform() {
  LOG(INFO) << ("Doing  mir->arm  transform\n");
  auto code = arm::ArmCode();
  for (auto& f : package.functions) {
    if (f.second.type->is_extern) continue;
    auto cg = codegen::Codegen(f.second, package, extra_data,
                               options.allow_conditional_exec);
    auto arm_f = cg.translate_function();
    code.functions.push_back(std::make_unique<arm::Function>(std::move(arm_f)));
  }
  for (auto& v : package.global_values) {
    code.consts.insert({v.first, v.second});
  }
  arm_code.emplace(std::move(code));
  if (options.show_code_after_each_pass) {
    LOG(INFO) << ("Code after transformation\n");
    std::cout << arm_code.value() << std::endl;
  }
}

void Backend::show_passes(std::ostream& o) {
  for (auto& pass : mir_passes) {
    if (!should_run_pass(pass->pass_name())) {
      o << "Skip: " << pass->pass_name() << std::endl;
    } else {
      o << " Run: " << pass->pass_name() << std::endl;
    }
  }
  o << " Run: mir_to_arm" << std::endl;
  for (auto& pass : arm_passes) {
    if (!should_run_pass(pass->pass_name())) {
      o << "Skip: " << pass->pass_name() << std::endl;
    } else {
      o << " Run: " << pass->pass_name() << std::endl;
    }
  }
}

arm::ArmCode Backend::generate_code() {
  do_mir_optimization();
  do_mir_to_arm_transform();
  do_arm_optimization();
  return std::move(arm_code.value());
}

}  // namespace backend
