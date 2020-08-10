#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "../../arm_code/arm.hpp"
#include "../../mir/mir.hpp"
#include "../backend.hpp"

namespace optimization::memvar_propagation {

enum class VarKind { GlobalVar, GlobalArray, LocalArray, Uncertain };

class Dest {
 public:
  mir::inst::VarId dest;
  mir::inst::Value offset;
  Dest(mir::inst::VarId _dest, mir::inst::Value _offset)
      : dest(_dest), offset(_offset) {}
  bool operator<(const Dest& d) const {
    if (dest == d.dest) {
      if (offset.index() == 0) {
        if (d.offset.index() == 0) {
          return std::get<0>(offset) < std::get<0>(d.offset);
        } else {
          return false;
        }
      } else {
        if (d.offset.index() == 1) {
          return std::get<1>(offset) < std::get<1>(d.offset);
        } else {
          return true;
        }
      }
    } else {
      return dest < d.dest;
    }
  }
  bool operator==(const Dest& d) const {
    if (dest == d.dest) {
      if (offset.index() == 0) {
        if (d.offset.index() == 0) {
          return std::get<0>(offset) == std::get<0>(d.offset);
        } else {
          return false;
        }
      } else {
        if (d.offset.index() == 1) {
          return std::get<1>(offset) == std::get<1>(d.offset);
        } else {
          return false;
        }
      }
    } else {
      return false;
    }
  }
};

class Var {
 public:
  mir::inst::VarId id;
  VarKind kind;
  Var(mir::inst::VarId _id, VarKind _kind) : id(_id), kind(_kind) {}
  bool operator<(const Var& d) const { return id < d.id; }
  bool operator==(const Var& d) const { return id == d.id; }
};

class Memory_Var_Propagation : public backend::MirOptimizePass {
 public:
  std::string name = "MemoryVarPropagation";
  bool aftercast;
  std::set<std::string> unaffected_call;

  std::string pass_name() const { return name; }

  Memory_Var_Propagation(bool _aftercast = false) : aftercast(_aftercast) {}

  void optimize_func(mir::inst::MirFunction& func) {
    /*
    store val to s_dest
    l_dest = load src
    if src is the same as s_dest
    l_dest can be repalced by val to
      1. the next store has same s_dest
      2. call
      3. end of the block
    */
    std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
    std::map<mir::inst::VarId, mir::inst::VarId> reg_load;
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      // calculate call num
      int index = 0;
      std::vector<int> call_index;
      call_index.push_back(0);
      for (auto& inst : bb.inst) {
        auto& i = *inst;
        if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
          auto find = unaffected_call.find(x->func);
          if (find == unaffected_call.end()) {
            call_index.push_back(index + 1);
          }
        }
        if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
          call_index.push_back(index);
        }
        index++;
      }
      std::vector<int> del_index;
      for (int j = 0; j < call_index.size(); j++) {
        int upper_bound =
            (j == (call_index.size() - 1)) ? bb.inst.size() : call_index[j + 1];
        std::map<mir::inst::VarId, std::variant<int32_t, mir::inst::VarId>>
            store;
        std::map<mir::inst::VarId, std::variant<int32_t, mir::inst::VarId>>
            load;
        std::map<mir::inst::VarId,
                 std::variant<int32_t, mir::inst::VarId>>::iterator it;
        index = 0;
        for (auto& inst : bb.inst) {
          if (index >= call_index[j] && index < upper_bound) {
            auto& i = *inst;
            if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
              std::variant<int32_t, mir::inst::VarId> value;
              if (x->val.index() == 1) {
                value.emplace<1>(std::get<1>(x->val));
              } else {
                value.emplace<0>(std::get<0>(x->val));
              }
              it = store.find(x->dest);
              if (it == store.end()) {
                store.insert(std::map<mir::inst::VarId,
                                      std::variant<int32_t, mir::inst::VarId>>::
                                 value_type(x->dest, value));
              } else {
                store[x->dest] = value;
              }
            } else if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
              if (x->src.index() == 1) {
                it = store.find(std::get<1>(x->src));
                if (it != store.end()) {
                  std::map<mir::inst::VarId,
                           std::variant<int32_t, mir::inst::VarId>>::iterator
                      iter;
                  iter = load.find(x->dest);
                  if (iter == load.end()) {
                    load.insert(
                        std::map<mir::inst::VarId,
                                 std::variant<int32_t, mir::inst::VarId>>::
                            value_type(x->dest, it->second));
                    if (it->second.index() == 1) {
                      reg_load.insert(
                          std::map<mir::inst::VarId, mir::inst::VarId>::
                              value_type(x->dest, std::get<1>(it->second)));
                    }
                  } else {
                    load[x->dest] = it->second;
                  }
                  if (it->second.index() == 1) {
                    del_index.push_back(index);
                  }
                } /*else {
                  std::variant<int32_t, mir::inst::VarId> value;
                  value.emplace<1>(x->dest);
                  if (x->src.index() == 1) {
                    store.insert(
                        std::map<mir::inst::VarId,
                                 std::variant<int32_t, mir::inst::VarId>>::
                            value_type(std::get<1>(x->src), value));
                  }
                }*/
              }
            }
          }
          index++;
        }
        for (it = load.begin(); it != load.end(); it++) {
          std::map<mir::inst::VarId,
                   std::variant<int32_t, mir::inst::VarId>>::iterator iet;
          for (iet = load.begin(); iet != load.end(); iet++) {
            if (iet->second.index() == 1 &&
                it->first == std::get<1>(iet->second)) {
              load[iet->first] = it->second;
            }
          }
        }
        // replace
        index = 0;
        for (auto& inst : bb.inst) {
          if (index >= call_index[j] && index < upper_bound) {
            auto& i = *inst;
            if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
              if (x->src.index() == 1) {
                std::map<mir::inst::VarId,
                         std::variant<int32_t, mir::inst::VarId>>::iterator
                    lit = load.find(std::get<1>(x->src));
                if (lit != load.end()) {
                  if (lit->second.index() == 0) {
                    x->src.emplace<0>(std::get<0>(lit->second));
                  } else {
                    x->src.emplace<1>(std::get<1>(lit->second));
                  }
                }
              }
            } else if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
              for (int j = 0; j < x->params.size(); j++) {
                if (x->params[j].index() == 1) {
                  std::map<mir::inst::VarId,
                           std::variant<int32_t, mir::inst::VarId>>::iterator
                      lit = load.find(std::get<1>(x->params[j]));
                  if (lit != load.end()) {
                    if (lit->second.index() == 0) {
                      x->params[j].emplace<0>(std::get<0>(lit->second));
                    } else {
                      x->params[j].emplace<1>(std::get<1>(lit->second));
                    }
                  }
                }
              }
            } else if (auto x = dynamic_cast<mir::inst::OpInst*>(&i)) {
              if (x->lhs.index() == 1) {
                std::map<mir::inst::VarId,
                         std::variant<int32_t, mir::inst::VarId>>::iterator
                    lit = load.find(std::get<1>(x->lhs));
                if (lit != load.end()) {
                  if (lit->second.index() == 0) {
                    x->lhs.emplace<0>(std::get<0>(lit->second));
                  } else {
                    x->lhs.emplace<1>(std::get<1>(lit->second));
                  }
                }
              }
              if (x->rhs.index() == 1) {
                std::map<mir::inst::VarId,
                         std::variant<int32_t, mir::inst::VarId>>::iterator
                    lit = load.find(std::get<1>(x->rhs));
                if (lit != load.end()) {
                  if (lit->second.index() == 0) {
                    x->rhs.emplace<0>(std::get<0>(lit->second));
                  } else {
                    x->rhs.emplace<1>(std::get<1>(lit->second));
                  }
                }
              }
            } else if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
              if (x->src.index() == 1) {
                std::map<mir::inst::VarId,
                         std::variant<int32_t, mir::inst::VarId>>::iterator
                    lit = load.find(std::get<1>(x->src));
                if (lit != load.end()) {
                  if (lit->second.index() == 0) {
                    x->src.emplace<0>(std::get<0>(lit->second));
                  } else {
                    x->src.emplace<1>(std::get<1>(lit->second));
                  }
                }
              }
            } else if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
              if (x->val.index() == 1) {
                std::map<mir::inst::VarId,
                         std::variant<int32_t, mir::inst::VarId>>::iterator
                    lit = load.find(std::get<1>(x->val));
                if (lit != load.end()) {
                  if (lit->second.index() == 0) {
                    x->val.emplace<0>(std::get<0>(lit->second));
                  } else {
                    x->val.emplace<1>(std::get<1>(lit->second));
                  }
                }
              }
            } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst*>(&i)) {
              if (x->offset.index() == 1) {
                std::map<mir::inst::VarId,
                         std::variant<int32_t, mir::inst::VarId>>::iterator
                    lit = load.find(std::get<1>(x->offset));
                if (lit != load.end()) {
                  if (lit->second.index() == 0) {
                    x->offset.emplace<0>(std::get<0>(lit->second));
                  } else {
                    x->offset.emplace<1>(std::get<1>(lit->second));
                  }
                }
              }
            }
          }
          index++;
        }
      }
      // delete load
      for (int i = del_index.size() - 1; i >= 0; i--) {
        auto iter = bit->second.inst.begin() + del_index[i];
        bit->second.inst.erase(iter);
      }
    }
    std::map<mir::inst::VarId, mir::inst::VarId>::iterator iet;
    for (iet = reg_load.begin(); iet != reg_load.end(); iet++) {
      std::map<mir::inst::VarId, mir::inst::VarId>::iterator iet1;
      for (iet1 = reg_load.begin(); iet1 != reg_load.end(); iet1++) {
        if (iet->first == iet1->second) {
          reg_load[iet1->first] = iet->second;
        }
      }
    }
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      for (auto& inst : bb.inst) {
        auto& i = *inst;
        if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
          if (x->src.index() == 1) {
            std::map<mir::inst::VarId, mir::inst::VarId>::iterator lit =
                reg_load.find(std::get<1>(x->src));
            if (lit != reg_load.end()) {
              x->src.emplace<1>(lit->second);
            }
          }
        } else if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
          for (int j = 0; j < x->params.size(); j++) {
            if (x->params[j].index() == 1) {
              std::map<mir::inst::VarId, mir::inst::VarId>::iterator lit =
                  reg_load.find(std::get<1>(x->params[j]));
              if (lit != reg_load.end()) {
                x->params[j].emplace<1>(lit->second);
              }
            }
          }
        } else if (auto x = dynamic_cast<mir::inst::OpInst*>(&i)) {
          if (x->lhs.index() == 1) {
            std::map<mir::inst::VarId, mir::inst::VarId>::iterator lit =
                reg_load.find(std::get<1>(x->lhs));
            if (lit != reg_load.end()) {
              x->lhs.emplace<1>(lit->second);
            }
          }
          if (x->rhs.index() == 1) {
            std::map<mir::inst::VarId, mir::inst::VarId>::iterator lit =
                reg_load.find(std::get<1>(x->rhs));
            if (lit != reg_load.end()) {
              x->rhs.emplace<1>(lit->second);
            }
          }
        } else if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
          if (x->src.index() == 1) {
            std::map<mir::inst::VarId, mir::inst::VarId>::iterator lit =
                reg_load.find(std::get<1>(x->src));
            if (lit != reg_load.end()) {
              x->src.emplace<1>(lit->second);
            }
          }
        } else if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
          if (x->val.index() == 1) {
            std::map<mir::inst::VarId, mir::inst::VarId>::iterator lit =
                reg_load.find(std::get<1>(x->val));
            if (lit != reg_load.end()) {
              x->val.emplace<1>(lit->second);
            }
          }
        } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst*>(&i)) {
          if (x->offset.index() == 1) {
            std::map<mir::inst::VarId, mir::inst::VarId>::iterator lit =
                reg_load.find(std::get<1>(x->offset));
            if (lit != reg_load.end()) {
              x->offset.emplace<1>(lit->second);
            }
          }
          std::map<mir::inst::VarId, mir::inst::VarId>::iterator lit =
              reg_load.find(x->ptr);
          if (lit != reg_load.end()) {
            x->ptr = lit->second;
          }
        } else if (auto x = dynamic_cast<mir::inst::PhiInst*>(&i)) {
          for (int j = 0; j < x->vars.size(); j++) {
            std::map<mir::inst::VarId, mir::inst::VarId>::iterator lit =
                reg_load.find(x->vars[j]);
            if (lit != reg_load.end()) {
              x->vars[j] = lit->second;
            }
          }
        }
      }
    }
  }

  void delete_store1(mir::inst::MirFunction& func) {
    std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      int index = 0;
      std::map<mir::inst::VarId, std::vector<int>> store;
      std::vector<int> del;
      bool redu = false;
      for (auto& inst : bb.inst) {
        auto& i = *inst;
        if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
          auto find = store.find(x->dest);
          if (find == store.end()) {
            std::vector<int> ind;
            ind.push_back(index);
            store.insert(
                std::map<mir::inst::VarId, std::vector<int>>::value_type(
                    x->dest, ind));
          } else {
            find->second.push_back(index);
          }
        }
        index++;
      }

      for (auto i = store.begin(); i != store.end(); i++) {
        if (i->second.size() > 1) {
          for (int j = 0; j < i->second.size() - 1; j++) {
            index = 0;
            bool dele = true;
            for (auto& inst : bb.inst) {
              auto& ii = *inst;
              if (index >= i->second[j] && index < i->second[j + 1]) {
                if (auto x = dynamic_cast<mir::inst::LoadInst*>(&ii)) {
                  if (x->src.index() == 1 && std::get<1>(x->src) == i->first) {
                    dele = false;
                  }
                }
                if (auto x = dynamic_cast<mir::inst::CallInst*>(&ii)) {
                  dele = false;
                }
              }
              index++;
            }
            if (dele) {
              del.push_back(i->second[j]);
            }
          }
        }
      }
      sort(del.begin(), del.end());
      for (int i = del.size() - 1; i >= 0; i--) {
        bit->second.inst.erase(bit->second.inst.begin() + del[i]);
      }
    }
  }

  void delete_store2(mir::inst::MirFunction& func) {
    std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      int index = 0;
      std::map<Dest, std::vector<int>> store;
      std::vector<int> del;
      bool redu = false;
      for (auto& inst : bb.inst) {
        auto& i = *inst;
        if (auto x = dynamic_cast<mir::inst::StoreOffsetInst*>(&i)) {
          Dest d(x->dest, x->offset);
          auto find = store.find(d);
          if (find == store.end()) {
            std::vector<int> ind;
            ind.push_back(index);
            store.insert(std::map<Dest, std::vector<int>>::value_type(d, ind));
          } else {
            find->second.push_back(index);
          }
        }
        index++;
      }

      for (auto i = store.begin(); i != store.end(); i++) {
        if (i->second.size() > 1) {
          for (int j = 0; j < i->second.size() - 1; j++) {
            index = 0;
            bool dele = true;
            for (auto& inst : bb.inst) {
              auto& ii = *inst;
              if (index >= i->second[j] && index < i->second[j + 1]) {
                if (auto x = dynamic_cast<mir::inst::LoadOffsetInst*>(&ii)) {
                  if (x->src.index() == 1) {
                    Dest d(std::get<1>(x->src), x->offset);
                    if (d == i->first) {
                      dele = false;
                    }
                  }
                }
                if (auto x = dynamic_cast<mir::inst::CallInst*>(&ii)) {
                  dele = false;
                }
              }
              index++;
            }
            if (dele) {
              del.push_back(i->second[j]);
            }
          }
        }
      }
      sort(del.begin(), del.end());
      for (int i = del.size() - 1; i >= 0; i--) {
        bit->second.inst.erase(bit->second.inst.begin() + del[i]);
      }
    }
  }

  bool check_effect(mir::inst::MirFunction& func) {
    bool b = true;
    std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      for (auto& inst : bb.inst) {
        auto& i = *inst;
        if (auto x = dynamic_cast<mir::inst::StoreOffsetInst*>(&i)) {
          b = false;
        }
        if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
          b = false;
        }
        if (auto x = dynamic_cast<mir::inst::LoadOffsetInst*>(&i)) {
          b = false;
        }
        if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
          b = false;
        }
        if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
          b = false;
        }
      }
    }
    return b;
  }

  void optimize_func1(mir::inst::MirFunction& func,
                      mir::inst::MirPackage& package) {
    std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
    std::set<Var> address;

    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      std::vector<int> temp;
      for (auto& inst : bb.inst) {
        auto& i = *inst;
        if (auto x = dynamic_cast<mir::inst::RefInst*>(&i)) {
          if (x->val.index() == 0) {
            Var var(x->dest, VarKind::LocalArray);
            address.insert(var);
          } else {
            auto find = package.global_values.find(std::get<1>(x->val));
            if (find->second.index() == 0) {
              Var var(x->dest, VarKind::GlobalVar);
              address.insert(var);
            } else if (find->second.index() == 1) {
              Var var(x->dest, VarKind::GlobalArray);
              address.insert(var);
            }
          }
        }
      }
    }
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      std::vector<int> temp;
      for (auto& inst : bb.inst) {
        auto& i = *inst;
        if (auto x = dynamic_cast<mir::inst::StoreOffsetInst*>(&i)) {
          bool has = false;
          for (auto k = address.begin(); k != address.end(); k++) {
            if ((*k).id == x->dest) {
              has = true;
              break;
            }
          }
          if (!has) {
            Var var(x->dest, VarKind::Uncertain);
            address.insert(var);
          }
        } else if (auto x = dynamic_cast<mir::inst::LoadOffsetInst*>(&i)) {
          bool has = false;
          if (x->src.index() == 1) {
            for (auto k = address.begin(); k != address.end(); k++) {
              if ((*k).id == std::get<1>(x->src)) {
                has = true;
                break;
              }
            }
            if (!has) {
              Var var(std::get<1>(x->src), VarKind::Uncertain);
              address.insert(var);
            }
          }
        }
      }
    }
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      for (auto k = address.begin(); k != address.end(); k++) {
        int index = 0;
        std::vector<int> call_index;
        call_index.push_back(0);
        for (auto& inst : bb.inst) {
          auto& i = *inst;
          if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
            auto find = unaffected_call.find(x->func);
            if (find == unaffected_call.end()) {
              call_index.push_back(index + 1);
            }
          }
          if (auto x = dynamic_cast<mir::inst::StoreOffsetInst*>(&i)) {
            if (x->dest == (*k).id) {
              call_index.push_back(index);
            }
          }
          index++;
        }
        std::vector<int> del_index;
        for (int j = 0; j < call_index.size(); j++) {
          int upper_bound = (j == (call_index.size() - 1)) ? bb.inst.size()
                                                           : call_index[j + 1];
          std::map<Dest, std::variant<int32_t, mir::inst::VarId>> store;
          std::map<mir::inst::VarId, std::variant<int32_t, mir::inst::VarId>>
              load;
          std::map<Dest, std::variant<int32_t, mir::inst::VarId>>::iterator it;
          index = 0;
          for (auto& inst : bb.inst) {
            if (index >= call_index[j] && index < upper_bound) {
              auto& i = *inst;
              if (auto x = dynamic_cast<mir::inst::StoreOffsetInst*>(&i)) {
                std::variant<int32_t, mir::inst::VarId> value;
                if (x->val.index() == 1) {
                  value.emplace<1>(std::get<1>(x->val));
                } else {
                  value.emplace<0>(std::get<0>(x->val));
                }
                Dest d(x->dest, x->offset);
                it = store.find(d);
                if (it == store.end()) {
                  store.insert(
                      std::map<Dest, std::variant<int32_t, mir::inst::VarId>>::
                          value_type(d, value));
                } else {
                  store[d] = value;
                }
              } else if (auto x =
                             dynamic_cast<mir::inst::LoadOffsetInst*>(&i)) {
                std::cout << x->dest << std::endl;
                if (x->src.index() == 1) {
                  Dest d(std::get<1>(x->src), x->offset);
                  it = store.find(d);
                  if (it != store.end()) {
                    std::map<mir::inst::VarId,
                             std::variant<int32_t, mir::inst::VarId>>::iterator
                        iter;
                    iter = load.find(x->dest);
                    if (iter == load.end()) {
                      load.insert(
                          std::map<mir::inst::VarId,
                                   std::variant<int32_t, mir::inst::VarId>>::
                              value_type(x->dest, it->second));
                    } else {
                      load[x->dest] = it->second;
                    }
                    if (it->second.index() == 1) {
                      del_index.push_back(index);
                    }
                  } else {
                    std::variant<int32_t, mir::inst::VarId> value;
                    value.emplace<1>(x->dest);
                    Dest d(std::get<1>(x->src), x->offset);
                    store.insert(
                        std::map<Dest,
                                 std::variant<int32_t, mir::inst::VarId>>::
                            value_type(d, value));
                  }
                }
              }
            }
            index++;
          }
          std::map<mir::inst::VarId,
                   std::variant<int32_t, mir::inst::VarId>>::iterator itt;
          for (itt = load.begin(); itt != load.end(); itt++) {
            std::map<mir::inst::VarId,
                     std::variant<int32_t, mir::inst::VarId>>::iterator iet;
            for (iet = load.begin(); iet != load.end(); iet++) {
              if (iet->second.index() == 1 &&
                  itt->first == std::get<1>(iet->second)) {
                load[iet->first] = itt->second;
              }
            }
          }
          // replace
          index = 0;
          for (auto& inst : bb.inst) {
            if (index >= call_index[j] && index < upper_bound) {
              auto& i = *inst;
              if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
                if (x->src.index() == 1) {
                  std::map<mir::inst::VarId,
                           std::variant<int32_t, mir::inst::VarId>>::iterator
                      lit = load.find(std::get<1>(x->src));
                  if (lit != load.end()) {
                    if (lit->second.index() == 0) {
                      x->src.emplace<0>(std::get<0>(lit->second));
                    } else {
                      x->src.emplace<1>(std::get<1>(lit->second));
                    }
                  }
                }
              } else if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
                for (int j = 0; j < x->params.size(); j++) {
                  if (x->params[j].index() == 1) {
                    std::map<mir::inst::VarId,
                             std::variant<int32_t, mir::inst::VarId>>::iterator
                        lit = load.find(std::get<1>(x->params[j]));
                    if (lit != load.end()) {
                      if (lit->second.index() == 0) {
                        x->params[j].emplace<0>(std::get<0>(lit->second));
                      } else {
                        x->params[j].emplace<1>(std::get<1>(lit->second));
                      }
                    }
                  }
                }
              } else if (auto x = dynamic_cast<mir::inst::OpInst*>(&i)) {
                if (x->lhs.index() == 1) {
                  std::map<mir::inst::VarId,
                           std::variant<int32_t, mir::inst::VarId>>::iterator
                      lit = load.find(std::get<1>(x->lhs));
                  if (lit != load.end()) {
                    if (lit->second.index() == 0) {
                      x->lhs.emplace<0>(std::get<0>(lit->second));
                    } else {
                      x->lhs.emplace<1>(std::get<1>(lit->second));
                    }
                  }
                }
                if (x->rhs.index() == 1) {
                  std::map<mir::inst::VarId,
                           std::variant<int32_t, mir::inst::VarId>>::iterator
                      lit = load.find(std::get<1>(x->rhs));
                  if (lit != load.end()) {
                    if (lit->second.index() == 0) {
                      x->rhs.emplace<0>(std::get<0>(lit->second));
                    } else {
                      x->rhs.emplace<1>(std::get<1>(lit->second));
                    }
                  }
                }
              } else if (auto x =
                             dynamic_cast<mir::inst::LoadOffsetInst*>(&i)) {
                if (x->src.index() == 1) {
                  std::map<mir::inst::VarId,
                           std::variant<int32_t, mir::inst::VarId>>::iterator
                      lit = load.find(std::get<1>(x->src));
                  if (lit != load.end()) {
                    if (lit->second.index() == 0) {
                      x->src.emplace<0>(std::get<0>(lit->second));
                    } else {
                      x->src.emplace<1>(std::get<1>(lit->second));
                    }
                  }
                }
                if (x->offset.index() == 1) {
                  std::map<mir::inst::VarId,
                           std::variant<int32_t, mir::inst::VarId>>::iterator
                      lit = load.find(std::get<1>(x->offset));
                  if (lit != load.end()) {
                    if (lit->second.index() == 0) {
                      x->offset.emplace<0>(std::get<0>(lit->second));
                    } else {
                      x->offset.emplace<1>(std::get<1>(lit->second));
                    }
                  }
                }
              } else if (auto x =
                             dynamic_cast<mir::inst::StoreOffsetInst*>(&i)) {
                if (x->val.index() == 1) {
                  std::map<mir::inst::VarId,
                           std::variant<int32_t, mir::inst::VarId>>::iterator
                      lit = load.find(std::get<1>(x->val));
                  if (lit != load.end()) {
                    if (lit->second.index() == 0) {
                      x->val.emplace<0>(std::get<0>(lit->second));
                    } else {
                      x->val.emplace<1>(std::get<1>(lit->second));
                    }
                  }
                }
                if (x->offset.index() == 1) {
                  std::map<mir::inst::VarId,
                           std::variant<int32_t, mir::inst::VarId>>::iterator
                      lit = load.find(std::get<1>(x->offset));
                  if (lit != load.end()) {
                    if (lit->second.index() == 0) {
                      x->offset.emplace<0>(std::get<0>(lit->second));
                    } else {
                      x->offset.emplace<1>(std::get<1>(lit->second));
                    }
                  }
                }
              } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst*>(&i)) {
                if (x->offset.index() == 1) {
                  std::map<mir::inst::VarId,
                           std::variant<int32_t, mir::inst::VarId>>::iterator
                      lit = load.find(std::get<1>(x->offset));
                  if (lit != load.end()) {
                    if (lit->second.index() == 0) {
                      x->offset.emplace<0>(std::get<0>(lit->second));
                    } else {
                      x->offset.emplace<1>(std::get<1>(lit->second));
                    }
                  }
                }
              }
            }
            index++;
          }
        }
        // delete load
        for (int i = del_index.size() - 1; i >= 0; i--) {
          auto iter = bit->second.inst.begin() + del_index[i];
          bit->second.inst.erase(iter);
        }
      }
    }
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         ++iter) {
      if (check_effect(iter->second)) {
        unaffected_call.insert(iter->first);
      }
    }
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         ++iter) {
      if (aftercast) {
        optimize_func1(iter->second, package);
        delete_store2(iter->second);
      } else {
        optimize_func(iter->second);
      }
    }
  }
};
}  // namespace optimization::memvar_propagation