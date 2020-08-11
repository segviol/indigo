#include "align_code.hpp"

#include <algorithm>
#include <any>
#include <cassert>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "../optimization/optimization.hpp"

using namespace arm;
using namespace backend::codegen;

void CodeAlignOptimization::optimize_arm(
    arm::ArmCode &arm_code, std::map<std::string, std::any> &extra_data_repo) {
  for (auto &f : arm_code.functions) {
    optimize_func(*f, extra_data_repo);
  }
}

void CodeAlignOptimization::optimize_func(
    arm::Function &f, std::map<std::string, std::any> &extra_data_repo) {
  auto cycle_set = std::any_cast<optimization::CycleStartType>(
                       extra_data_repo.at(optimization::CYCLE_START_DATA_NAME))
                       .at(f.name);
  std::vector<std::unique_ptr<arm::Inst>> inst_new;
  for (auto &i : f.inst) {
    if (auto x = dynamic_cast<arm::LabelInst *>(&*i)) {
      if (x->label.find(".bb_") == 0) {
        try {
          auto bb_id_idx = x->label.find_last_of("$");
          auto id_str = x->label.substr(bb_id_idx + 1);
          auto bb_id = std::stoi(id_str);
          if (cycle_set.find(bb_id) != cycle_set.end())
            inst_new.push_back(std::make_unique<CtrlInst>("align", 6, true));
        } catch (std::exception &i) {
        }
      }
    }
    inst_new.push_back(std::move(i));
  }
  f.inst = std::move(inst_new);
}
