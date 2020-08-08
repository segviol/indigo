#include "../../include/aixlog.hpp"
#include "bmir_optimization.hpp"

namespace front::optimization::bmir_optimization {
bool BmirOptimization::should_run_pass(std::string&& pass_name) {
  auto pass = pass_name;
  return should_run_pass(pass);
}

bool BmirOptimization::should_run_pass(std::string& pass_name) {
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

void BmirOptimization::do_bmir_optimization() {
  for (auto& pass : bmir_passes) {
    if (!should_run_pass(pass->pass_name())) {
      LOG(INFO) << "Skipping BMIR pass: " << pass->pass_name() << "\n";
      continue;
    }

    LOG(INFO) << "Running BMIR pass: " << pass->pass_name() << "\n";
    pass->optimize_bmir(package, bmirVariableTable, funcInsts, messages);
  }
}
}  // namespace front::optimization::bmir_optimization