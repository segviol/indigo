#pragma once

#include <vector>
#include <map>

#include "../../arm_code/arm.hpp"
#include "../../mir/mir.hpp"
#include "../backend.hpp"

namespace optimization::memvar_propagation {

class Memory_Var_Propagation : public backend::MirOptimizePass {
 public:
  std::string name = "MemoryVarPropagation";

  std::string pass_name() const { return name; }

  void optimize_func(mir::inst::MirFunction& func) {
    std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      //calculate call num
      int index = 0;
      std::vector<int> call_index;
      call_index.push_back(0);
      for (auto& inst : bb.inst) {
        auto& i = *inst;
        if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
          call_index.push_back(index + 1);
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
        std::map<mir::inst::VarId, std::variant<int32_t, mir::inst::VarId>>
            ::iterator it;
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
                  } else {
                    load[x->dest] = it->second;
                  }
                  del_index.push_back(index);
                }
              }
            }
          }
          index++;
        }
        for (it = load.begin(); it != load.end(); it++) {
          std::map<mir::inst::VarId,
                   std::variant<int32_t, mir::inst::VarId>>::iterator iet;
          for (iet = load.begin(); iet != load.end(); iet++) {
            if (iet->second.index() == 1 && it->first == std::get<1>(iet->second)) {
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
                    lit =
                    load.find(std::get<1>(x->src));
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
      //delete load
      for (int i = del_index.size() - 1; i >= 0; i--) {
        auto iter = bit->second.inst.begin() + del_index[i];
        bit->second.inst.erase(iter);
      }
    }
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         ++iter) {
      optimize_func(iter->second);
    }
  }
};
}  // namespace optimization::memvar_propagation