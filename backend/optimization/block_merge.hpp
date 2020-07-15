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

namespace optimization::mergeBlocks {

class Merge_Block : public backend::MirOptimizePass {
 public:
  std::set<std::string> uninlineable_funcs;
  std::string name = "MergeBlock";
  bool recursively;
  std::string pass_name() const { return name; }

  void optimize_func(mir::inst::MirFunction& func) {
    while (true) {
      bool flag = false;
      for (auto& blkiter : func.basic_blks) {
        auto& blk = blkiter.second;
        if (blk.preceding.size() == 1) {
          auto& preBlk = func.basic_blks.at(*blk.preceding.begin());
          if (preBlk.jump.kind == mir::inst::JumpInstructionKind::Br &&
              preBlk.jump.bb_true == blk.id) {
            for (auto& inst : blk.inst) {
              preBlk.inst.push_back(std::move(inst));
            }
            preBlk.jump = mir::inst::JumpInstruction(
                blk.jump.kind, blk.jump.bb_true, blk.jump.bb_false,
                blk.jump.cond_or_ret, blk.jump.jump_kind);
            if (func.basic_blks.count(preBlk.jump.bb_true)) {
              auto& subBlk = func.basic_blks.at(preBlk.jump.bb_true);
              subBlk.preceding.erase(blk.id);
              subBlk.preceding.insert(preBlk.id);
            }
            if (func.basic_blks.count(preBlk.jump.bb_false)) {
              auto& subBlk = func.basic_blks.at(preBlk.jump.bb_false);
              subBlk.preceding.erase(blk.id);
              subBlk.preceding.insert(preBlk.id);
            }
            flag = true;
            func.basic_blks.erase(blk.id);
            break;
          }
        }
      }
      if (!flag) {
        break;
      }
    }
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         iter++) {
      optimize_func(iter->second);
    }
  }
};

}  // namespace optimization::mergeBlocks
