#include "bb_rearrange.hpp"

#include <bits/stdint-uintn.h>

#include <deque>
#include <vector>

#include "../optimization/optimization.hpp"
#include "spdlog/fmt/bundled/core.h"

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
  for (auto& f : mir.functions) {
    if (f.second.type->is_extern) continue;
    auto res = optimize_func(f.second);

    std::cout << "bb arrangement for " << f.second.name << " is:" << std::endl;
    for (auto i : res) {
      std::cout << i << " ";
    }
    std::cout << std::endl;

    ordering_map.insert({f.second.name, std::move(res)});
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
    input_count.insert({bb.first, bb.second.preceding.size()});
  }
  input_count.insert_or_assign(f.basic_blks.begin()->first, 1);

  std::cout << "func: " << f.name << std::endl;
  for (auto& x : input_count) {
    std::cout << x.first << " " << x.second;
    auto cycle = cycles.find(x.first);
    if (cycle != cycles.end()) std::cout << " " << cycle->second;
    std::cout << std::endl;
  }

  bfs.push_back(f.basic_blks.begin()->first);
  while (bfs.size() > 0) {
    int id = bfs.front();
    bfs.pop_front();
    int& cnt = input_count[id];
    if (cnt > 0) cnt--;
    fmt::print("cyclesolver {}: count {}", id, cnt);
    auto x = cycles.find(id);

    if (x == cycles.end())
      fmt::print("cycle {}\n", x->second);
    else
      fmt::print("cycle 0\n");
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
      bfs.push_back(bb.jump.bb_true);
      bfs.push_back(bb.jump.bb_false);
    }
  }
  return std::move(arrangement);
}
}  // namespace backend::codegen
