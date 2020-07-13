#pragma once
#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "../../mir/mir.hpp"
#include "../backend.hpp"

namespace inlineFunc {

class Inline_Func : public backend::MirOptimizePass {
 public:
  void optimize_func(std::string funcId, mir::inst::MirFunction& func) {}

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         iter++) {
      optimize_func(iter->first, iter->second);
    }
  }
};

}  // namespace inlineFunc