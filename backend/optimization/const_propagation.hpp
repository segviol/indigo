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

class GlobalVar {
 public:
  std::string reg;
  int32_t value;
  bool useable;
  std::vector<mir::inst::VarId> treg;
  GlobalVar(std::string _reg)
      : reg(_reg),
      value(0),
      useable(false),
      treg() {}
};

class Const_Propagation : public backend::MirOptimizePass {
 public:
  std::string name = "ConstPropagation";

  std::string pass_name() const { return name; }

  void const_propagation(mir::inst::MirPackage& p) {
    std::map<mir::inst::VarId, int32_t> const_assign;
    std::map<std::string, mir::inst::MirFunction>::iterator pit;
    for (pit = p.functions.begin(); pit != p.functions.end(); pit++) {
      std::map<mir::types::LabelId, mir::inst::BasicBlk>::iterator bit;
      for (bit = pit->second.basic_blks.begin();
           bit != pit->second.basic_blks.end(); bit++) {
        auto& bb = bit->second;
        std::vector<GlobalVar> addr;
        for (auto& inst : bb.inst) {
          auto& i = *inst;
          if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
            if (x->src.index() == 0) {
              const_assign.insert(
                  std::map<mir::inst::VarId, int32_t>::value_type(
                      x->dest, std::get<0>(x->src)));
            }
          } else if (auto x = dynamic_cast<mir::inst::RefInst*>(&i)) {
            if (x->val.index() == 1) {
              bool has = false;
              for (int j = 0; j < addr.size(); j++) {
                if (addr[j].reg == std::get<1>(x->val)) {
                  has = true;
                  addr[j].treg.push_back(x->dest);
                  break;
                }
              }
              if (!has) {
                GlobalVar g(std::get<1>(x->val));
                g.treg.push_back(x->dest);
                addr.push_back(g);
              }
            }
          } else if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
            if (x->val.index() == 0) {
              for (int j = 0; j < addr.size(); j++) {
                for (int k = 0; k < addr[j].treg.size(); k++) {
                  if (addr[j].treg[k] == x->dest) {
                    addr[j].useable = true;

                    addr[j].value = std::get<0>(x->val);
                  }
                }
              }
            } else {
              std::map<mir::inst::VarId, int32_t>::iterator it =
                  const_assign.find(std::get<1>(x->val));
              if (it != const_assign.end()) {
                x->val.emplace<0>(it->second);
                for (int j = 0; j < addr.size(); j++) {
                  for (int k = 0; k < addr[j].treg.size(); k++) {
                    if (addr[j].treg[k] == x->dest) {
                      addr[j].useable = true;
                      addr[j].value = std::get<0>(x->val);
                    }
                  }
                }
              } else {
                for (int j = 0; j < addr.size(); j++) {
                  for (int k = 0; k < addr[j].treg.size(); k++) {
                    if (addr[j].treg[k] == x->dest) {
                      addr[j].useable = false;
                    }
                  }
                }
              }
            }
          } else if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
            if (x->src.index() == 1) {
              for (int j = 0; j < addr.size(); j++) {
                for (int k = 0; k < addr[j].treg.size(); k++) {
                  if (addr[j].treg[k] == std::get<1>(x->src) &&
                      addr[j].useable == true) {
                    const_assign.insert(
                        std::map<mir::inst::VarId, int32_t>::value_type(
                            x->dest, addr[j].value));
                  }
                }
              }
            }
          }
        }
      }
      for (bit = pit->second.basic_blks.begin();
           bit != pit->second.basic_blks.end(); bit++) {
        auto& bb = bit->second;
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
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    const_propagation(package);
  }
};
}  // namespace optimization::const_propagation