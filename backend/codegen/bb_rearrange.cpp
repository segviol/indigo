#include "bb_rearrange.hpp"

#include <bits/stdint-uintn.h>

#include <deque>
#include <vector>

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
  if (visited.find(id) != visited.end()) {
    return;
  } else if (path.find(id) != path.end()) {
    auto res = counter.try_emplace(id, 1);
    if (!res.second) {
      res.first->second++;
    }
  } else {
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
  }
}

void BasicBlkRearrange::optimize_mir(
    mir::inst::MirPackage& mir,
    std::map<std::string, std::any>& extra_data_repo) {
  optimization::BasicBlockOrderingType ordering_map;
  for (auto& f : mir.functions) {
    if (f.second.type->is_extern) continue;
    auto res = optimize_func(f.second);
    ordering_map.insert({f.first, std::move(res)});
  }
  extra_data_repo.insert(
      {optimization::BASIC_BLOCK_ORDERING_DATA_NAME, std::move(ordering_map)});
}

std::vector<uint32_t> BasicBlkRearrange::optimize_func(
    mir::inst::MirFunction& f) {
  auto cycles = CycleSolver(f).solve();
  std::set<int> visited;
  std::deque<int> bfs;
  std::map<int, int> input_count;

  std::vector<uint32_t> arrangement;

  for (auto& bb : f.basic_blks) {
    for (auto id : bb.second.preceding) {
      auto res = input_count.try_emplace(id, 1);
      if (!res.second) res.first->second++;
    }
  }
  input_count.insert({f.basic_blks.begin()->first, 1});

  bfs.push_back(f.basic_blks.begin()->first);
  while (bfs.size() > 0) {
    int id = bfs.front();
    bfs.pop_front();
    int cnt = 0;
    {
      auto cnt__ = input_count.find(id);
      if (cnt__ != input_count.end()) {
        cnt__->second--;
        cnt = cnt__->second;
      }
    }

    if (auto x = cycles.find(id); (x != cycles.end() && cnt > x->second) ||
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
      bfs.push_back(bb.jump.bb_true);
      bfs.push_back(bb.jump.bb_false);
    }
  }
  return std::move(arrangement);
}
}  // namespace backend::codegen
