#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../arm_code/arm.hpp"
#include "../../mir/mir.hpp"
#include "../backend.hpp"

namespace optimization::const_propagation {

class Const_Propagation : public backend::MirOptimizePass {
 public:
  std::string name = "ConstPropagation";

  std::string pass_name() const { return name; }

  void optimize_func(mir::inst::MirFunction& func) {
    /*
    if src of assign is constant
    all the dest can be replaced by the src of assign
    */
    std::map<mir::inst::VarId, int32_t> const_assign;
    std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      for (auto& inst : bb.inst) {
        auto& i = *inst;
        if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
          if (x->src.index() == 0) {
            int src = std::get<0>(x->src);
            if (src != 0) 
            const_assign.insert(std::map<mir::inst::VarId, int32_t>::value_type(
                x->dest, std::get<0>(x->src)));
          }
        }
      }
    }
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      std::map<int, std::unique_ptr<mir::inst::Inst>> replace;
      std::vector<int> del;
      for (auto& inst : bb.inst) {
        auto& i = *inst;
        if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
          if (x->src.index() == 1) {
            std::map<mir::inst::VarId, int32_t>::iterator it =
                const_assign.find(std::get<1>(x->src));
            if (it != const_assign.end()) {
              x->src.emplace<0>(it->second);
            }
          }
        } else if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
          for (int j = 0; j < x->params.size(); j++) {
            if (x->params[j].index() == 1) {
              std::map<mir::inst::VarId, int32_t>::iterator it =
                  const_assign.find(std::get<1>(x->params[j]));
              if (it != const_assign.end()) {
                x->params[j].emplace<0>(it->second);
              }
            }
          }
        } else if (auto x = dynamic_cast<mir::inst::OpInst*>(&i)) {
          if (x->lhs.index() == 1) {
            std::map<mir::inst::VarId, int32_t>::iterator it =
                const_assign.find(std::get<1>(x->lhs));
            if (it != const_assign.end()) {
              x->lhs.emplace<0>(it->second);
            }
          }
          if (x->rhs.index() == 1) {
            std::map<mir::inst::VarId, int32_t>::iterator it =
                const_assign.find(std::get<1>(x->rhs));
            if (it != const_assign.end()) {
              x->rhs.emplace<0>(it->second);
            }
          }
        } else if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
          if (x->src.index() == 1) {
            std::map<mir::inst::VarId, int32_t>::iterator it =
                const_assign.find(std::get<1>(x->src));
            if (it != const_assign.end()) {
              x->src.emplace<0>(it->second);
            }
          }
        } else if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
          if (x->val.index() == 1) {
            std::map<mir::inst::VarId, int32_t>::iterator it =
                const_assign.find(std::get<1>(x->val));
            if (it != const_assign.end()) {
              x->val.emplace<0>(it->second);
            }
          }
        } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst*>(&i)) {
          if (x->offset.index() == 1) {
            std::map<mir::inst::VarId, int32_t>::iterator it =
                const_assign.find(std::get<1>(x->offset));
            if (it != const_assign.end()) {
              x->offset.emplace<0>(it->second);
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
      optimize_func(iter->second);
    }
  }
};
}  // namespace optimization::const_propagation
