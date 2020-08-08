#pragma once
#include <any>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../mir/mir.hpp"
#include "../../opt.hpp"
#include "../ir_generator.hpp"
#include "bmir_variable_table.hpp"

namespace front::optimization::bmir_optimization {
class BmirOptimizationPass;

class BmirOptimization {
 public:
  BmirOptimization(
      mir::inst::MirPackage& package,
      bmir_variable_table::BmirVariableTable& bmirVariableTable,
      std::map<std::string, std::vector<irGenerator::Instruction>>& funcInsts,
      Options& options)
      : package(package),
        bmirVariableTable(bmirVariableTable),
        funcInsts(funcInsts),
        options(options),
        messages() {}
  void add_pass(std::unique_ptr<BmirOptimizationPass>&& pass) {
    bmir_passes.push_back(std::move(pass));
  }

  // Apply all bmir optimizations
  void do_bmir_optimization();

 private:
  mir::inst::MirPackage& package;
  bmir_variable_table::BmirVariableTable& bmirVariableTable;
  std::map<std::string, std::vector<irGenerator::Instruction>>& funcInsts;
  Options& options;

  std::vector<std::unique_ptr<BmirOptimizationPass>> bmir_passes;
  std::map<std::string, std::any> messages;

  bool should_run_pass(std::string& pass_name);
  bool should_run_pass(std::string&& pass_name);
};

class BmirOptimizationPass {
 public:
  virtual std::string pass_name() const = 0;
  virtual void optimize_bmir(
      mir::inst::MirPackage& package,
      bmir_variable_table::BmirVariableTable& bmirVariableTable,
      std::map<std::string, std::vector<irGenerator::Instruction>>& funcInsts,
      std::map<std::string, std::any>& messages) = 0;

  virtual ~BmirOptimizationPass() {}

 private:
};
}  // namespace front::optimization::bmir_optimization