#include "bb_rearrange.hpp"

#include <deque>
#include <vector>

#include "../../include/aixlog.hpp"
#include "../optimization/optimization.hpp"

namespace backend::codegen {

struct CycleSolver {
  CycleSolver(mir::inst::MirFunction& f) : f(f) {}
  mir::inst::MirFunction& f;

  std::map<int, int> counter;
  std::set<int> visited;
  std::set<int> path;

  std::map<int, int> solve() {
    dfs(f.basic_blks.begin()->first);
    return std::move(counter);
  }
  void dfs(int id);
};

void CycleSolver::dfs(int id) {
  if (path.find(id) != path.end()) {
    if (counter.find(id) == counter.end()) {
      counter.insert({id, 1});
    } else {
      counter[id]++;
    }
  } else if (visited.find(id) != visited.end()) {
    return;
  } else {
    path.insert(id);
    visited.insert(id);
    auto& bb = f.basic_blks.at(id);
    switch (bb.jump.kind) {
      case mir::inst::JumpInstructionKind::Br:
        dfs(bb.jump.bb_true);
        break;
      case mir::inst::JumpInstructionKind::BrCond:
        dfs(bb.jump.bb_true);
        dfs(bb.jump.bb_false);
        break;
      default:
        break;
    }
    path.erase(id);
  }
}

void BasicBlkRearrange::optimize_mir(
    mir::inst::MirPackage& mir,
    std::map<std::string, std::any>& extra_data_repo) {
  optimization::BasicBlockOrderingType ordering_map;
  optimization::CycleStartType cycle_map;
  optimization::InlineBlksType inline_map;
  for (auto& f : mir.functions) {
    if (f.second.type->is_extern) continue;
    auto res = optimize_func(f.second);
    auto arrange = std::move(std::get<0>(res));
    auto cycle = std::move(std::get<1>(res));
    auto inline_ = std::move(std::get<2>(res));

    LOG(TRACE) << "bb arrangement for " << f.second.name << " is:" << std::endl;
    for (auto i : arrange) {
      LOG(TRACE) << i << " ";
    }
    LOG(TRACE) << std::endl;

    ordering_map.insert_or_assign(f.second.name, std::move(arrange));
    cycle_map.insert_or_assign(f.second.name, std::move(cycle));
    inline_map.insert_or_assign(f.second.name, std::move(inline_));
  }
  extra_data_repo.insert_or_assign(optimization::BASIC_BLOCK_ORDERING_DATA_NAME,
                                   std::move(ordering_map));
  extra_data_repo.insert_or_assign(optimization::CYCLE_START_DATA_NAME,
                                   std::move(cycle_map));
  extra_data_repo.insert_or_assign(optimization::inline_blks,
                                   std::move(inline_map));
}

std::tuple<std::vector<uint32_t>, std::set<uint32_t>, std::set<uint32_t>>
BasicBlkRearrange::optimize_func(mir::inst::MirFunction& f) {
  auto cycles = CycleSolver(f).solve();
  std::set<int> visited;
  std::set<uint32_t> inline_blks;
  std::deque<int> bfs;
  std::map<int, int> input_count;

  std::vector<uint32_t> arrangement;

  bool has_common_exit_blk = f.basic_blks.find(1048576) != f.basic_blks.end();

  for (auto& bb : f.basic_blks) {
    if (bb.second.inst.size() > 8) continue;
    if (bb.second.preceding.size() != 1) continue;
    if (bb.second.jump.kind != mir::inst::JumpInstructionKind::Br) continue;
    bool can_inline = true;
    for (auto it = bb.second.inst.begin(); it != bb.second.inst.end(); it++) {
      auto& inst = *it;
      auto& i = *inst;
      if (auto x = dynamic_cast<mir::inst::OpInst*>(&i)) {
        if (x->op == mir::inst::Op::Div || x->op == mir::inst::Op::Rem) {
          can_inline = false;
          break;
        }
        if (it != bb.second.inst.end() - 1 &&
            (x->op == mir::inst::Op::Gt || x->op == mir::inst::Op::Lt ||
             x->op == mir::inst::Op::Gte || x->op == mir::inst::Op::Lt ||
             x->op == mir::inst::Op::Eq || x->op == mir::inst::Op::Neq)) {
          can_inline = false;
          break;
        }
      } else if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
        can_inline = false;
        break;
      } else if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
      } else if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
      } else if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
      } else if (auto x = dynamic_cast<mir::inst::LoadOffsetInst*>(&i)) {
      } else if (auto x = dynamic_cast<mir::inst::StoreOffsetInst*>(&i)) {
      } else if (auto x = dynamic_cast<mir::inst::RefInst*>(&i)) {
      } else if (auto x = dynamic_cast<mir::inst::PhiInst*>(&i)) {
      } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst*>(&i)) {
      } else {
        throw new std::bad_cast();
      }
    }
    if (can_inline) inline_blks.insert(bb.first);
  }

  for (auto& bb : f.basic_blks) {
    input_count.insert({bb.first, bb.second.preceding.size()});
  }
  input_count.insert_or_assign(f.basic_blks.begin()->first, 1);

  LOG(TRACE) << "func: " << f.name << std::endl;
  for (auto& x : input_count) {
    LOG(TRACE) << x.first << " " << x.second;
    auto cycle = cycles.find(x.first);
    if (cycle != cycles.end()) LOG(TRACE) << " " << cycle->second;
    LOG(TRACE) << std::endl;
  }

  bfs.push_back(f.basic_blks.begin()->first);
  while (bfs.size() > 0) {
    int id = bfs.front();
    bfs.pop_front();
    int& cnt = input_count[id];
    if (cnt > 0) cnt--;
    auto x = cycles.find(id);

    if ((x != cycles.end() && cnt > x->second) ||
        (x == cycles.end() && cnt > 0))
      continue;

    if (visited.find(id) != visited.end())
      continue;
    else
      visited.insert(id);

    arrangement.push_back(id);

    auto& bb = f.basic_blks.at(id);
    if (bb.jump.kind == mir::inst::JumpInstructionKind::Br) {
      bfs.push_back(bb.jump.bb_true);
    } else if (bb.jump.kind == mir::inst::JumpInstructionKind::BrCond) {
      if (inline_blks.find(bb.jump.bb_false) != inline_blks.end() &&
          inline_blks.find(bb.jump.bb_true) == inline_blks.end()) {
        bfs.push_back(bb.jump.bb_false);
        bfs.push_back(bb.jump.bb_true);
      } else {
        bfs.push_back(bb.jump.bb_true);
        bfs.push_back(bb.jump.bb_false);
      }
    } else if (bb.jump.kind == mir::inst::JumpInstructionKind::Return &&
               has_common_exit_blk) {
      bfs.push_back(1048576);
    }
  }
  auto cycle_start_set = std::set<uint32_t>();
  for (auto c : cycles) {
    if (c.second != 0) cycle_start_set.insert(c.first);
  }
  return {std::move(arrangement), std::move(cycle_start_set),
          std::move(inline_blks)};
}
}  // namespace backend::codegen
