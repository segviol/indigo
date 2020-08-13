#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "../../arm_code/arm.hpp"
#include "../../include/aixlog.hpp"
#include "../../mir/mir.hpp"
#include "../backend.hpp"

namespace optimization::cycle {

enum class Color { White, Gray, Black };

class Node {
 public:
  int id;
  Color color;
  std::vector<int> pi;
  std::vector<int> next;

  Node(int _id) : id(_id), color(Color::White) {}
};

class Edge {
 public:
  int begin;
  int end;

  Edge(int _begin, int _end) : begin(_begin), end(_end) {}
};

class Cycle : public backend::MirOptimizePass {
 public:
  std::string name = "Cycle";
  std::vector<Node> graph;
  std::vector<Edge> back_edge;
  std::map<std::vector<int>, int> loop;
  std::vector<int> trace;
  std::set<int> searched;
  mir::types::LabelId label_id = 0;
  std::map<int, int> preheader;
  std::map<uint32_t, int> define;
  std::map<int, std::vector<int>> invar;
  std::set<uint32_t> add;
  std::set<uint32_t> phi_para;

  std::string pass_name() const { return name; }

  int find_node(int id) {
    for (int i = 0; i < graph.size(); i++) {
      if (graph[i].id == id) {
        return i;
      }
    }
    return -1;
  }

  void DFS_VISIT(int u) {
    graph[u].color = Color::Gray;
    for (int i = 0; i < graph[u].next.size(); i++) {
      int v = find_node(graph[u].next[i]);
      if (v >= 0) {
        if (graph[v].color == Color::White) {
          graph[v].pi.push_back(graph[u].id);
          DFS_VISIT(v);
        } else if (graph[v].color == Color::Gray) {
          Edge be(graph[u].id, graph[v].id);
          back_edge.push_back(be);
        }
      }
    }
    graph[u].color = Color::Black;
  }

  void findCycle(int v) {
    // cout << "cc " << v << endl;
    int j = -1;
    for (int i = 0; i < trace.size(); i++) {
      if (trace[i] == v) {
        j = i;
        break;
      }
    }
    if (j != -1) {
      std::vector<int> circle;
      while (j < trace.size()) {
        searched.insert(trace[j]);
        circle.push_back(trace[j]);
        j++;
      }
      // std::vector<Edge> e;
      int predomin = 0;
      for (int k = 0; k < circle.size(); k++) {
        Edge edge(circle[k], circle[(k + 1) % circle.size()]);
        for (int kk = 0; kk < back_edge.size(); kk++) {
          if (edge.begin == back_edge[kk].begin &&
              edge.end == back_edge[kk].end) {
            predomin = back_edge[kk].end;
          }
        }
      }
      sort(circle.begin(), circle.end());
      loop.insert(
          std::map<std::vector<int>, int>::value_type(circle, predomin));
      return;
    }
    trace.push_back(v);
    for (int i = 0; i < graph.size(); i++) {
      if (graph[i].id == v) {
        for (int k = 0; k < graph[i].next.size(); k++) {
          findCycle(graph[i].next[k]);
        }
      }
    }
    trace.pop_back();
  }

  void optimize_func(mir::inst::MirFunction& func) {
    std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      if (bit->first > label_id && bit->first < 1048576) {
        label_id = bit->first;
      }
      auto& bb = bit->second;
      if (bb.preceding.size() == 0) {
        Node node(bb.id);
        if (bb.jump.bb_true >= 0) {
          node.next.push_back(bb.jump.bb_true);
        }
        if (bb.jump.bb_false >= 0) {
          node.next.push_back(bb.jump.bb_false);
        }
        graph.push_back(node);
        // break;
      }
    }
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      if (bb.preceding.size() != 0) {
        Node node(bb.id);
        if (bb.jump.bb_true >= 0) {
          node.next.push_back(bb.jump.bb_true);
        }
        if (bb.jump.bb_false >= 0) {
          node.next.push_back(bb.jump.bb_false);
        }
        graph.push_back(node);
      }
    }
    for (int i = 0; i < graph.size(); i++) {
      if (graph[i].color == Color::White) {
        DFS_VISIT(i);
      }
    }
    trace.clear();
    searched.clear();
    loop.clear();
    for (int i = 0; i < graph.size(); i++) {
      if (searched.find(graph[i].id) == searched.end()) {
        trace.clear();
        findCycle(graph[i].id);
      }
    }
  }

  void set_defined(mir::inst::MirFunction& func) {
    for (auto i = func.variables.begin(); i != func.variables.end(); i++) {
      for (auto bit = func.basic_blks.begin(); bit != func.basic_blks.end();
           bit++) {
        auto& bb = bit->second;
        for (auto& inst : bb.inst) {
          auto& j = *inst;
          if (j.dest == mir::inst::VarId(i->first)) {
            define.insert(
                std::map<uint32_t, int>::value_type(i->first, bit->second.id));
          }
        }
      }
    }
  }

  bool is_incycle(int i, std::vector<int> lo) {
    for (int j = 0; j < lo.size(); j++) {
      if (lo[j] == i) {
        return true;
      }
    }
    return false;
  }

  void find_invariant(mir::inst::MirFunction& func, std::vector<int> lo) {
    invar.clear();
    for (int i = 0; i < lo.size(); i++) {
      auto in = func.basic_blks.find(lo[i]);
      if (in != func.basic_blks.end()) {
        auto& bb = in->second;
        std::set<uint32_t> invars;
        std::vector<int> vec;
        int index = 0;
        for (auto& inst : bb.inst) {
          auto& ii = *inst;
          if (auto x = dynamic_cast<mir::inst::AssignInst*>(&ii)) {
            if (x->src.index() == 0) {
              auto find = func.variables.find(x->dest);
              vec.push_back(index);
              if (find->second.is_temp_var == true) invars.insert(x->dest);
            } else {
              auto find = define.find(std::get<1>(x->src));
              if (find != define.end() && !is_incycle(find->second, lo) &&
                  phi_para.find(x->dest) == phi_para.end() &&
                  phi_para.find(std::get<1>(x->src)) == phi_para.end()) {
                auto find = func.variables.find(x->dest);
                vec.push_back(index);
                if (find->second.is_temp_var == true) invars.insert(x->dest);
              }
              if (invars.find(std::get<1>(x->src)) != invars.end()) {
                auto find = func.variables.find(x->dest);
                vec.push_back(index);
                if (find->second.is_temp_var == true) invars.insert(x->dest);
              }
            }
          } else if (auto x = dynamic_cast<mir::inst::OpInst*>(&ii)) {
            bool l_t = false, r_t = false;
            if (x->lhs.index() == 0) {
              l_t = true;
            } else {
              auto find = define.find(std::get<1>(x->lhs));

              if (find != define.end() && !is_incycle(find->second, lo) &&
                  phi_para.find(x->dest) == phi_para.end() &&
                  phi_para.find(std::get<1>(x->lhs)) == phi_para.end()) {
                l_t = true;
              }
              if (invars.find(std::get<1>(x->lhs)) != invars.end()) {
                l_t = true;
              }
            }
            if (x->rhs.index() == 0) {
              r_t = true;
            } else {
              auto find = define.find(std::get<1>(x->rhs));

              if (find != define.end() && !is_incycle(find->second, lo) &&
                  phi_para.find(x->dest) == phi_para.end() &&
                  phi_para.find(std::get<1>(x->rhs)) == phi_para.end()) {
                r_t = true;
              }
              if (invars.find(std::get<1>(x->rhs)) != invars.end()) {
                r_t = true;
              }
            }
            if (l_t && r_t) {
              auto find = func.variables.find(x->dest);
              vec.push_back(index);
              if (find->second.is_temp_var == true) invars.insert(x->dest);
            }
          } else if (auto x = dynamic_cast<mir::inst::CallInst*>(&ii)) {
          } else if (auto x = dynamic_cast<mir::inst::LoadOffsetInst*>(&ii)) {
            bool l_t = false, r_t = false;
            if (x->offset.index() == 0) {
              l_t = true;
            } else {
              auto find = define.find(std::get<1>(x->offset));

              if (find != define.end() && !is_incycle(find->second, lo) &&
                  phi_para.find(x->dest) == phi_para.end() &&
                  phi_para.find(std::get<1>(x->offset)) == phi_para.end()) {
                l_t = true;
              }
              if (invars.find(std::get<1>(x->offset)) != invars.end()) {
                l_t = true;
              }
            }
            if (x->src.index() == 0) {
              r_t = true;
            } else {
              auto find = define.find(std::get<1>(x->src));

              if (find != define.end() && !is_incycle(find->second, lo) &&
                  phi_para.find(x->dest) == phi_para.end() &&
                  phi_para.find(std::get<1>(x->src)) == phi_para.end() &&
                  add.find(std::get<1>(x->src)) != add.end()) {
                r_t = true;
              }
            }
            if (l_t && r_t) {
              auto find = func.variables.find(x->dest);
              vec.push_back(index);
              if (find->second.is_temp_var == true) invars.insert(x->dest);
            }
          } else if (auto x = dynamic_cast<mir::inst::StoreInst*>(&ii)) {
          } else if (auto x = dynamic_cast<mir::inst::PhiInst*>(&ii)) {
          } else if (auto x = dynamic_cast<mir::inst::RefInst*>(&ii)) {
          } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst*>(&ii)) {
          }
          index++;
        }
        if (vec.size() > 3) {
          invar.insert(
              std::map<int, std::vector<int>>::value_type(in->first, vec));
        }
      }
    }
  }

  void set_add(std::vector<int> lo, mir::inst::MirFunction& func) {
    for (int i = 0; i < lo.size(); i++) {
      auto in = func.basic_blks.find(lo[i]);
      if (in != func.basic_blks.end()) {
        auto& bb = in->second;
        for (auto& inst : bb.inst) {
          auto& ii = *inst;
          if (auto x = dynamic_cast<mir::inst::LoadOffsetInst*>(&ii)) {
            if (x->src.index() == 1) {
              add.insert(std::get<1>(x->src));
            }
          }
        }
      }
    }
    for (int i = 0; i < lo.size(); i++) {
      auto in = func.basic_blks.find(lo[i]);
      if (in != func.basic_blks.end()) {
        auto& bb = in->second;
        for (auto& inst : bb.inst) {
          auto& ii = *inst;
          if (auto x = dynamic_cast<mir::inst::StoreOffsetInst*>(&ii)) {
            add.erase(x->dest);
          }
        }
      }
    }
  }

  void extract(mir::inst::MirFunction& func, int num, std::vector<int> lo) {
    for (int i = 0; i < lo.size(); i++) {
      auto bb = func.basic_blks.find(lo[i]);
      auto j = invar.find(lo[i]);
      if (j != invar.end()) {
        for (int k = j->second.size() - 1; k >= 0; k--) {
          auto find = func.variables.find(bb->second.inst[j->second[k]]->dest);
          if (find != func.variables.end()) {
            if (find->second.is_temp_var == true) {
              find->second.is_temp_var = false;
              auto pre_int = preheader.find(num);
              if (pre_int == preheader.end()) {
                int i;
                for (i = 0; i < back_edge.size(); i++) {
                  if (back_edge[i].end == num) {
                    break;
                  }
                }
                int next = back_edge[i].end;
                std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator
                    bit = func.basic_blks.find(next);
                if (bit != func.basic_blks.end()) {
                  mir::inst::BasicBlk bb(++label_id);
                  bb.preceding = bit->second.preceding;
                  bb.preceding.erase(back_edge[i].begin);
                  bb.jump = mir::inst::JumpInstruction(
                      mir::inst::JumpInstructionKind::Br, bit->second.id);

                  bit->second.preceding.clear();
                  bit->second.preceding.insert(label_id);
                  bit->second.preceding.insert(back_edge[i].begin);
                  std::set<mir::types::LabelId>::iterator it;
                  for (it = bb.preceding.begin(); it != bb.preceding.end();
                       it++) {
                    if (*it != back_edge[i].begin) {
                      auto preb = func.basic_blks.find(*it);
                      if (preb->second.jump.bb_true == next) {
                        preb->second.jump.bb_true = label_id;
                      }
                      if (preb->second.jump.bb_false == next) {
                        preb->second.jump.bb_false = label_id;
                      }
                    }
                  }
                  func.basic_blks.insert({label_id, std::move(bb)});
                  preheader.insert(
                      std::map<int, int>::value_type(next, label_id));
                }
              }
              pre_int = preheader.find(num);
              auto pre = func.basic_blks.find(pre_int->second);
              pre->second.inst.insert(pre->second.inst.begin(),
                                      std::move(bb->second.inst[j->second[k]]));
              bb->second.inst.erase(bb->second.inst.begin() + j->second[k]);
            }
          }
        }
      }
    }
  }

  void sort_prehead(mir::inst::MirFunction& func) {
    std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      bool find = false;
      for (auto i = preheader.begin(); i != preheader.end(); i++) {
        if (i->second == bit->second.id) {
          find = true;
          break;
        }
      }
      if (find) {
        bool change = true;
        while (change) {
          change = false;
          auto& bb = bit->second;
          int index = 0;
          std::map<mir::inst::VarId, Edge> info;
          for (auto& inst : bb.inst) {
            auto& ii = *inst;
            if (auto x = dynamic_cast<mir::inst::AssignInst*>(&ii)) {
              auto f = info.find(x->dest);
              if (f == info.end()) {
                Edge inn((-1), index);
                info.insert(
                    std::map<mir::inst::VarId, Edge>::value_type(x->dest, inn));
              } else {
                f->second.end = index;
              }
              if (x->src.index() == 1) {
                auto f = info.find(std::get<1>(x->src));
                if (f == info.end()) {
                  Edge in(index, -2);
                  info.insert(std::map<mir::inst::VarId, Edge>::value_type(
                      std::get<1>(x->src), in));
                } else {
                  f->second.begin = index;
                }
              }
            } else if (auto x = dynamic_cast<mir::inst::OpInst*>(&ii)) {
              auto f = info.find(x->dest);
              if (f == info.end()) {
                Edge inn((-1), index);
                info.insert(
                    std::map<mir::inst::VarId, Edge>::value_type(x->dest, inn));
              } else {
                f->second.end = index;
              }
              if (x->lhs.index() == 1) {
                auto f = info.find(std::get<1>(x->lhs));
                if (f == info.end()) {
                  Edge in(index, -2);
                  info.insert(std::map<mir::inst::VarId, Edge>::value_type(
                      std::get<1>(x->lhs), in));
                } else {
                  f->second.begin = index;
                }
              }
              if (x->rhs.index() == 1) {
                auto f = info.find(std::get<1>(x->rhs));
                if (f == info.end()) {
                  Edge in(index, -2);
                  info.insert(std::map<mir::inst::VarId, Edge>::value_type(
                      std::get<1>(x->rhs), in));
                } else {
                  f->second.begin = index;
                }
              }
            }
            index++;
          }
          for (auto i = info.begin(); i != info.end(); i++) {
            if (i->second.begin < i->second.end && i->second.begin != -1) {
              change = true;
              bb.inst.push_back(std::move(bb.inst[i->second.begin]));
              bb.inst.erase(bb.inst.begin() + i->second.begin);
              break;
            }
          }
        }
      }
    }
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         ++iter) {
      std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
      for (bit = iter->second.basic_blks.begin();
           bit != iter->second.basic_blks.end(); bit++) {
        auto& bb = bit->second;
        for (auto& inst : bb.inst) {
          auto& ii = *inst;
          if (auto x = dynamic_cast<mir::inst::CallInst*>(&ii)) {
            for (int i = 0; i < x->params.size(); i++) {
              if (x->params[i].index() == 1) {
                phi_para.insert(std::get<1>(x->params[i]));
              }
            }
          }
        }
      }
    }
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         ++iter) {
      optimize_func(iter->second);
    }
    std::set<std::vector<int>>::iterator it;
    for (auto i = loop.begin(); i != loop.end(); i++) {
      for (auto iter = package.functions.begin();
           iter != package.functions.end(); ++iter) {
        auto find = iter->second.basic_blks.find(i->first[0]);
        if (find != iter->second.basic_blks.end()) {
          define.clear();
          add.clear();
          set_add(i->first, iter->second);
          for (int ii = 0; ii < i->first.size(); ii++) {
            std::cout << i->first[ii] << " ";
          }
          std::cout << std::endl;
          for (auto ii = add.begin(); ii != add.end(); ii++) {
            std::cout << *ii << std::endl;
          }
          set_defined(iter->second);
          find_invariant(iter->second, i->first);
          extract(iter->second, i->second, i->first);
        }
      }
    }
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         ++iter) {
      sort_prehead(iter->second);
    }

    std::cout << "ud chain:" << std::endl;
    for (auto i = define.begin(); i != define.end(); i++) {
      std::cout << i->first << " : " << i->second << std::endl;
    }
    std::cout << "invar:" << std::endl;
    for (auto i = invar.begin(); i != invar.end(); i++) {
      std::cout << i->first << " : ";
      for (int j = 0; j < i->second.size(); j++) {
        std::cout << i->second[j] << " ";
      }
      std::cout << std::endl;
    }
    std::cout << "loop:" << std::endl;
    for (auto it = loop.begin(); it != loop.end(); it++) {
      std::cout << it->second << " : ";
      for (int i = 0; i < it->first.size(); i++) {
        std::cout << it->first[i] << " ";
      }
      std::cout << std::endl;
    }
    std::cout << "back edge:" << std::endl;
    for (int i = 0; i < back_edge.size(); i++) {
      std::cout << back_edge[i].begin << " " << back_edge[i].end << std::endl;
    }
    std::cout << "preheader:" << std::endl;
    for (auto i = preheader.begin(); i != preheader.end(); i++) {
      std::cout << i->first << " " << i->second << std::endl;
    }
  }
};
}  // namespace optimization::cycle
