#include "./init_end_block.hpp"

#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <deque>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "../../include/aixlog.hpp"

namespace optimization::init_end_block {

void InitEndBlock::optimize_mir(
    mir::inst::MirPackage& mir,
    std::map<std::string, std::any>& extra_data_repo) {
  for (auto& f : mir.functions) {
    if (f.second.type->is_extern) continue;
    optimize_func(f.second);
  }
}

void InitEndBlock::optimize_func(mir::inst::MirFunction& f) {
  for (auto exit : f.get_exits()) {
    auto& jump = f.basic_blks.at(exit).jump;
    if (!f.basic_blks.count(jump.bb_true)) {
      f.basic_blks.insert({jump.bb_true, mir::inst::BasicBlk(jump.bb_true)});
      f.basic_blks.at(jump.bb_true).jump =
          mir::inst::JumpInstruction(mir::inst::JumpInstructionKind::Undefined);
    }
    f.basic_blks.at(jump.bb_true).preceding.insert(exit);
  }
}
}  // namespace optimization::init_end_block
