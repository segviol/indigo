#pragma once
#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "../../mir/mir.hpp"
#include "../backend.hpp"

namespace optimization::cast_inst {

class Cast_Inst : public backend::MirOptimizePass {
 public:
  std::set<std::string> uninlineable_funcs;
  std::string name = "CastInst";
  bool recursively;
  std::string pass_name() const { return name; }

  void optimize_func(mir::inst::MirFunction& func) {
    std::map<mir::inst::VarId, std::pair<mir::inst::VarId, mir::inst::Value>>
        offset_map;
    for (auto& blkpair : func.basic_blks) {
      auto& insts = blkpair.second.inst;
      for (auto iter = insts.begin(); iter != insts.end();) {
        auto& inst = *iter;
        auto& i = *inst;
        if (inst->inst_kind() == mir::inst::InstKind::PtrOffset) {
          auto ptrInst = dynamic_cast<mir::inst::PtrOffsetInst*>(&i);
          offset_map.insert({ptrInst->dest, {ptrInst->ptr, ptrInst->offset}});
          // iter = insts.erase(iter);
          iter++;
        } else {
          iter++;
        }
      }
    }
    for (auto& blkpair : func.basic_blks) {
      auto& insts = blkpair.second.inst;
      for (auto& inst : insts) {
        auto& i = *inst;
        if (inst->inst_kind() == mir::inst::InstKind::Load) {
          auto loadInst = dynamic_cast<mir::inst::LoadInst*>(&i);

          mir::inst::Value offset = 0;
          mir::inst::Value addr = loadInst->src;
          if (offset_map.count(std::get<mir::inst::VarId>(loadInst->src))) {
            auto& pair =
                offset_map.at(std::get<mir::inst::VarId>(loadInst->src));
            offset = pair.second;
            addr = pair.first;
          }
          inst = std::make_unique<mir::inst::LoadOffsetInst>(
              addr, loadInst->dest, offset);
        } else if (inst->inst_kind() == mir::inst::InstKind::Store) {
          auto storeInst = dynamic_cast<mir::inst::StoreInst*>(&i);

          mir::inst::Value offset = 0;
          mir::inst::Value addr = storeInst->dest;
          if (offset_map.count(storeInst->dest)) {
            auto& pair = offset_map.at(storeInst->dest);
            offset = pair.second;
            addr = pair.first;
          }
          inst = std::make_unique<mir::inst::StoreOffsetInst>(
              storeInst->val, std::get<mir::inst::VarId>(addr), offset);
        }
      }
    }
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         iter++) {
      optimize_func(iter->second);
    }
  }
};

}  // namespace optimization::cast_inst
