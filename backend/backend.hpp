#pragma once
#include <any>
#include <map>

#include "../arm_code/arm.hpp"
#include "../mir/mir.hpp"
#include "../opt.hpp"

namespace backend {

class MirOptimizePass;
class ArmOptimizePass;

class Backend {
 public:
  Backend(mir::inst::MirPackage& package, Options& options)
      : package(package),
        arm_code(),
        mir_passes(),
        arm_passes(),
        options(options),
        extra_data() {}

  void add_pass(std::unique_ptr<MirOptimizePass> pass) {
    mir_passes.push_back(std::move(pass));
  }

  void add_pass(std::unique_ptr<ArmOptimizePass> pass) {
    arm_passes.push_back(std::move(pass));
  }

  /// Apply all mir optimizations
  void do_mir_optimization();

  /// Apply all ARM optimizations
  void do_arm_optimization();

  /// Transform MIR to ARM
  void do_mir_to_arm_transform();

  /// Generate final code
  arm::ArmCode generate_code();

 private:
  mir::inst::MirPackage& package;
  std::optional<arm::ArmCode> arm_code;
  Options& options;

  std::vector<std::unique_ptr<MirOptimizePass>> mir_passes;
  std::vector<std::unique_ptr<ArmOptimizePass>> arm_passes;
  std::map<std::string, std::any> extra_data;
};

/// Base class for all optimize passes that work on MIR
class MirOptimizePass {
 public:
  virtual std::string pass_name() const = 0;
  virtual void optimize_mir(
      mir::inst::MirPackage& package,
      std::map<std::string, std::any>& extra_data_repo) = 0;

  virtual ~MirOptimizePass(){};
};

/// Base class for all optimize passes that work on ARM code
class ArmOptimizePass {
 public:
  virtual std::string pass_name() const = 0;
  virtual void optimize_arm(
      arm::ArmCode& arm_code,
      std::map<std::string, std::any>& extra_data_repo) = 0;

  virtual ~ArmOptimizePass(){};
};

}  // namespace backend
