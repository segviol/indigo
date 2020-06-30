#pragma once

#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../backend.hpp"
#include "livevar_analyse.hpp"

namespace optimization::graph_color {

typedef u_int32_t color;  // 0 means illegal register

std::map<mir::inst::VarId, color> color_map;

class graph_color : backend::MirOptimizePass {
 public:
  u_int color_count = 1;
  const std::string name = "graph_color";
  std::map<mir::types::LabelId,
           std::shared_ptr<livevar_analyse::Livevar_Analyse>>
      blk_livevar_analyse;
  graph_color(u_int color_count) : color_count(color_count) {}
  std::string pass_name() const { return name; }
  std::map<mir::inst::VarId, std::set<mir::types::LabelId>> var_blks;
  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         iter++) {
      blk_livevar_analyse[iter->first] =
          std::make_shared<livevar_analyse::Livevar_Analyse>();
      blk_livevar_analyse[iter->first]->build(iter->second);
    }
  }
};

}  // namespace optimization::graph_color