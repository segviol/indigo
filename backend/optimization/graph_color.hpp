#pragma once

#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "../backend.hpp"
#include "livevar_analyse.hpp"

namespace optimization::graph_color {

typedef u_int32_t color;  // 0 means illegal register
typedef std::map<mir::inst::VarId, color> Color_Map;
class Conflict_Map {
 public:
  std::map<mir::inst::VarId, livevar_analyse::VariableSet> static_Map;
  std::map<mir::inst::VarId, livevar_analyse::VariableSet> dynamic_Map;
  std::map<u_int, livevar_analyse::VariableSet> edge_vars;
  std::stack<mir::inst::VarId> remove_Nodes;
  std::shared_ptr<Color_Map> color_map;
  u_int color_num = 8;
  Conflict_Map(std::shared_ptr<Color_Map> color_map, u_int color_num = 8)
      : color_num(color_num), color_map(color_map) {}
  void add_conflict(mir::inst::VarId defVar,
                    std::set<mir::inst::VarId> useVars) {
    for (auto useVar : useVars) {
      if (!static_Map.count(defVar)) {
        static_Map[defVar] = std::set<mir::inst::VarId>();
        dynamic_Map[defVar] = std::set<mir::inst::VarId>();
      }
      if (!static_Map.count(useVar)) {
        static_Map[useVar] = std::set<mir::inst::VarId>();
        dynamic_Map[useVar] = std::set<mir::inst::VarId>();
      }
      static_Map[defVar].insert(useVar);
      static_Map[useVar].insert(defVar);
      dynamic_Map[defVar].insert(useVar);
      dynamic_Map[useVar].insert(defVar);
    }
  }

  void init_edge_vars() {
    for (auto iter = static_Map.begin(); iter != static_Map.end(); ++iter) {
      auto size = iter->second.size();
      if (!edge_vars.count(size)) {
        edge_vars[size] = std::set<mir::inst::VarId>();
      }
      edge_vars[size].insert(iter->first);
    }
  }

  void delete_edge(mir::inst::VarId var) {
    assert(dynamic_Map.count(var));
    auto size = dynamic_Map[var].size();
    if (!size) {
      return;
    }
    edge_vars[size].erase(var);
    if (!edge_vars[size].size()) {
      edge_vars.erase(size);
    }
    if (!edge_vars.count(size - 1)) {
      edge_vars[size - 1] = std::set<mir::inst::VarId>();
    }
    edge_vars[size - 1].insert(var);
  }

  void remove_node_tmper(mir::inst::VarId var) {
    if (!dynamic_Map.count(var)) {
      return;
    }
    edge_vars[dynamic_Map[var].size()].erase(var);
    for (auto iter = dynamic_Map[var].begin(); iter != dynamic_Map[var].end();
         ++iter) {
      dynamic_Map[*iter].erase(var);
      delete_edge(*iter);
    }
    dynamic_Map.erase(var);
    remove_Nodes.push(var);
  }

  void remove_node_forever(mir::inst::VarId var) {
    if (!static_Map.count(var)) {
      return;
    }
    for (auto iter = static_Map[var].begin(); iter != static_Map[var].end();
         ++iter) {
      static_Map[*iter].erase(var);
    }
    assert(dynamic_Map.count(var));
    edge_vars[dynamic_Map[var].size()].erase(var);
    for (auto iter = dynamic_Map[var].begin(); iter != dynamic_Map[var].end();
         ++iter) {
      dynamic_Map[*iter].erase(var);
      delete_edge(*iter);
    }
    dynamic_Map.erase(var);
  }

  void remove_edge_less_colors() {
    while (true) {
      if (!edge_vars.size()) {
        return;
      }
      auto min_edge_iter = edge_vars.begin();
      auto min_edge = min_edge_iter->first;
      std::set<mir::inst::VarId> vars_to_remove;
      if (min_edge >= color_num) {
        return;
      }
      for (auto iter = edge_vars.begin();
           iter != edge_vars.end() && iter->first < color_num; ++iter) {
        vars_to_remove.insert(iter->second.begin(), iter->second.end());
      }
      for (auto var : vars_to_remove) {
        remove_node_tmper(var);
      }
    }
  }
  void remove_edge_larger_colors() {
    if (!edge_vars.size()) {
      return;
    }
    auto min_edge_iter = edge_vars.begin();
    auto min_edge = min_edge_iter->first;
    assert(min_edge >= color_num);
    auto iter = min_edge_iter->second.begin();
    remove_node_forever(*iter);
  }

  bool empty() { return dynamic_Map.empty(); }

  color available_color(std::set<mir::inst::VarId>& neighbors) {
    std::vector<bool> colors(color_num, true);
    for (auto var : neighbors) {
      colors[color_map.get()->at(var)] = false;
    }
    for (int i = 0; i < color_num; i++) {
      if (colors[i]) {
        return i + 1;
      }
    }
    // errors
    assert(false);
    return 0;
  }

  void add_edge(mir::inst::VarId var1, mir::inst::VarId var2) {
    assert(dynamic_Map.count(var1) && dynamic_Map.count(var2));
    dynamic_Map[var1].insert(var2);
    dynamic_Map[var2].insert(var1);
  }

  void add_node(mir::inst::VarId var) {
    for (auto neighbor : static_Map[var]) {
      if (dynamic_Map[var].count(neighbor)) {
        add_edge(var, neighbor);
      }
    }
    (*color_map)[var] = available_color(dynamic_Map[var]);
  }

  void rebuild() {
    while (remove_Nodes.size()) {
      add_node(remove_Nodes.top());
      remove_Nodes.pop();
    }
  }
};

class Graph_Color : backend::MirOptimizePass {
 public:
  u_int color_num = 8;
  const std::string name = "graph_color";
  std::map<mir::types::LabelId,
           std::shared_ptr<livevar_analyse::Livevar_Analyse>>
      blk_livevar_analyse;
  std::set<mir::inst::VarId> cross_blk_vars;
  std::map<mir::inst::VarId, color> var_color_map;
  std::map<mir::types::LabelId, std::shared_ptr<Color_Map>> func_color_map;
  std::map<mir::types::LabelId, std::shared_ptr<Conflict_Map>>
      func_conflict_map;
  Graph_Color(u_int color_num) : color_num(color_num) {}
  std::string pass_name() const { return name; }
  std::map<mir::inst::VarId, std::set<mir::types::LabelId>> var_blks;

  // Here's a trick. In format SSA,if a var crosses blocks, there must be a
  // block which the var is in it's live_vars out. This is because only one
  // define point is allowed for each var
  void init_cross_blk_vars(livevar_analyse::sharedPtrBlkLivevar blv) {
    cross_blk_vars.insert(blv->live_vars_out->begin(),
                          blv->live_vars_out->end());
  }

  void init_conflict_map(mir::inst::BasicBlk& blk,
                         std::map<u_int, mir::inst::Variable> vartable,
                         std::shared_ptr<Conflict_Map> conflict_map) {
    for (auto iter = blk.inst.begin(); iter != blk.inst.end(); ++iter) {
      auto defVar = iter->get()->dest;
      if (vartable[defVar.id].ty->kind() == mir::types::TyKind::Void ||
          !cross_blk_vars.count(defVar)) {
        continue;
      }
      auto useVars = iter->get()->useVars();
      livevar_analyse::VariableSet cross_use_vars;
      std::set_intersection(useVars.begin(), useVars.end(),
                            cross_blk_vars.begin(), cross_blk_vars.end(),
                            cross_use_vars.begin());
      conflict_map->add_conflict(defVar, cross_use_vars);
    }
  }

  void optimize_func(mir::types::LabelId funcId, mir::inst::MirFunction& func) {
    func_color_map[funcId] =
        std::make_shared<Color_Map>(std::map<mir::inst::VarId, color>());
    func_conflict_map[funcId] = std::make_shared<Conflict_Map>(
        Conflict_Map(func_color_map[funcId], color_num));
    auto conflict_map = func_conflict_map[funcId];
    livevar_analyse::Livevar_Analyse lva(func);
    lva.build();
    for (auto iter = lva.livevars.begin(); iter != lva.livevars.end(); ++iter) {
      init_cross_blk_vars(iter->second);
    }
    for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
         ++iter) {
      init_conflict_map(iter->second, func.variables, conflict_map);
    }
    while (!conflict_map->dynamic_Map.empty()) {
      conflict_map->remove_edge_less_colors();
      conflict_map->remove_edge_larger_colors();
    }
    conflict_map->rebuild();
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         iter++) {
      optimize_func(iter->first, iter->second);
      extra_data_repo.insert(func_color_map.begin(), func_color_map.end());
    }
  }
};

}  // namespace optimization::graph_color