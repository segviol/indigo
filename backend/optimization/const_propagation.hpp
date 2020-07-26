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
    std::map<std::string, mir::inst::VarId> ref;
    std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      for (auto& inst : bb.inst) {
        auto& i = *inst;
        if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
          if (x->src.index() == 0) {
            const_assign.insert(std::map<mir::inst::VarId, int32_t>::value_type(
                x->dest, std::get<0>(x->src)));
          }
        } else if (auto x = dynamic_cast<mir::inst::RefInst*>(&i)) {
          if (x->val.index() == 1) {
            std::map<std::string, mir::inst::VarId>::iterator rit =
                ref.find(std::get<1>(x->val));
            if (rit == ref.end()) {
              ref.insert(std::map<std::string, mir::inst::VarId>::value_type(
                  std::get<1>(x->val), x->dest));
            }
          }
        }
      }
    }
    std::vector<std::unique_ptr<mir::inst::Inst>> insert;
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      int index = 0;
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
        } else if (auto x = dynamic_cast<mir::inst::RefInst*>(&i)) {
          if (x->val.index() == 1) {
            std::map<std::string, mir::inst::VarId>::iterator rit =
                ref.find(std::get<1>(x->val));
            if (rit != ref.end()) {
              mir::inst::VarId dest = x->dest;
              mir::inst::VarId src = rit->second;
              std::unique_ptr<mir::inst::Inst> assginInst =
                  std::unique_ptr<mir::inst::AssignInst>(
                      new mir::inst::AssignInst(dest, src));
              if (dest == src) {
                del.push_back(index);
                std::unique_ptr<mir::inst::Inst> refInst =
                    std::unique_ptr<mir::inst::RefInst>(
                        new mir::inst::RefInst(dest, x->val));
                insert.push_back(std::move(refInst));
              } else {
                replace.insert(
                    std::map<int, std::unique_ptr<mir::inst::Inst>>::value_type(
                        index, std::move(assginInst)));
              }
            }
          }
        }
        index++;
      }
      std::map<int, std::unique_ptr<mir::inst::Inst>>::iterator reit;
      for (reit = replace.begin(); reit != replace.end(); reit++) {
        auto iter = bit->second.inst.begin() + reit->first;
        bit->second.inst.erase(iter);
        bit->second.inst.insert(bit->second.inst.begin() + reit->first,
                                std::move(reit->second));
      }
      if (del.size() == 1) {
        auto iter = bit->second.inst.begin() + del[0];
        bit->second.inst.erase(iter);
      }
    }
    for (bit = func.basic_blks.begin(); bit != func.basic_blks.end(); bit++) {
      auto& bb = bit->second;
      if (bb.preceding.size() == 0) {
        for (int i = 0; i < insert.size(); i++) {
          bb.inst.insert(bb.inst.begin(), std::move(insert[i]));
        }
        break;
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