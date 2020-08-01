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

#include "../../include/aixlog.hpp"
#include "../backend.hpp"
#include "livevar_analyse.hpp"

namespace optimization::const_merge {
typedef mir::inst::Op Op;

class Env {
 public:
  mir::inst::MirFunction& func;
  std::map<mir::inst::VarId, std::vector<std::pair<mir::types::LabelId, int>>>
      usepoints;
  Env(mir::inst::MirFunction& func) : func(func) {
    for (auto& blkpair : func.basic_blks) {
      for (int i = 0; i < blkpair.second.inst.size(); i++) {
        auto& inst = blkpair.second.inst[i];
        auto& is = *inst;
        if (inst->inst_kind() == mir::inst::InstKind::Assign) {
          auto assignInst = dynamic_cast<mir::inst::AssignInst*>(&is);
          if (assignInst->src.index() == 1) {
            auto srcid = std::get<mir::inst::VarId>(assignInst->src);
            add_usepoint(srcid, blkpair.first, i);
          }
        } else if (inst->inst_kind() == mir::inst::InstKind::Op) {
          auto opInst = dynamic_cast<mir::inst::OpInst*>(&is);
          if (opInst->lhs.index() == 1) {
            auto var = std::get<mir::inst::VarId>(opInst->lhs);
            add_usepoint(var, blkpair.first, i);
          }
          if (opInst->rhs.index() == 1) {
            auto var = std::get<mir::inst::VarId>(opInst->rhs);
            add_usepoint(var, blkpair.first, i);
          }
        }
      }
    }
  }

  void add_usepoint(mir::inst::VarId var, mir::types::LabelId labelId,
                    int idx) {
    if (!usepoints.count(var)) {
      usepoints.insert(
          {var, std::vector<std::pair<mir::types::LabelId, int>>()});
    }
    usepoints.at(var).push_back({labelId, idx});
  }

  std::unique_ptr<mir::inst::Inst>& get_inst(
      std::pair<mir::types::LabelId, int> pair) {
    return func.basic_blks.at(pair.first).inst.at(pair.second);
  }
};
std::shared_ptr<Env> env;
class Merge_Const : public backend::MirOptimizePass {
 public:
  std::map<mir::inst::VarId, int> const_var_map;
  std::string name = "MergeConst";
  std::string pass_name() const { return name; }
  void spread_var(mir::inst::VarId var, int val) {
    if (!env->usepoints.count(var)) {
      return;
    }
    for (auto& usepoint : env->usepoints.at(var)) {
      auto& inst = env->get_inst(usepoint);
      auto& is = *inst;
      if (inst->inst_kind() == mir::inst::InstKind::Assign) {
        auto assignInst = dynamic_cast<mir::inst::AssignInst*>(&is);
        assignInst->src = val;
        const_var_map.insert({assignInst->dest, val});
        spread_var(assignInst->dest, val);
      } else {
        auto opInst = dynamic_cast<mir::inst::OpInst*>(&is);
        if (opInst->lhs.index() == 1) {
          auto lhs = std::get<mir::inst::VarId>(opInst->lhs);
          if (lhs.id == var.id) {
            opInst->lhs = val;
          }
        }
        if (opInst->rhs.index() == 1) {
          auto rhs = std::get<mir::inst::VarId>(opInst->rhs);
          if (rhs.id == var.id) {
            opInst->rhs = val;
          }
        }
        if (opInst->lhs.index() == 0 && opInst->rhs.index() == 0) {
          optimize_inst(inst);
        }
      }
    }
  }
  bool optimize_inst(std::unique_ptr<mir::inst::Inst>& inst) {
    if (inst->inst_kind() == mir::inst::InstKind::Op) {
      auto& i = *inst;
      auto opInst = dynamic_cast<mir::inst::OpInst*>(&i);
      if (opInst->lhs.index() == 0 && opInst->rhs.index() == 0) {
        int res = 0;
        int lhs = std::get<int>(opInst->lhs);
        int rhs = std::get<int>(opInst->rhs);
        switch (opInst->op) {
          case Op::Add:
            res = lhs + rhs;
            break;
          case Op::Sub:
            res = lhs - rhs;
            break;
          case Op::Mul:
            res = lhs * rhs;
            break;
          case Op::Div:
            res = lhs / rhs;
            break;
          case Op::Rem:
            res = lhs % rhs;
            break;
          case Op::Eq:
            res = lhs == rhs;
            break;
          case Op::Neq:
            res = lhs != rhs;
            break;
          case Op::Gt:
            res = lhs > rhs;
            break;
          case Op::Lt:
            res = lhs < rhs;
            break;
          case Op::Gte:
            res = lhs >= rhs;
            break;
          case Op::Lte:
            res = lhs <= rhs;
            break;
          case Op::And:
            res = lhs & rhs;
            break;
          case Op::Or:
            res = lhs | rhs;
            break;
          case Op::Shl:
            res = lhs << rhs;
            break;
          case Op::Shr:
            res = (unsigned)lhs >> rhs;
            break;
          case Op::ShrA:
            res = lhs >> rhs;
            break;
          case Op::MulSh:
            res = (int32_t)(((int64_t)lhs * (int64_t)rhs) >> 32);
            break;
          case Op::Xor:
            res = lhs ^ rhs;
            break;
          case Op::Not:
            res = ~lhs;
            break;
          default:
            return false;
        }
        inst = std::make_unique<mir::inst::AssignInst>(opInst->dest, res);
        const_var_map.insert({opInst->dest, res});
        spread_var(opInst->dest, res);
        return true;
      }
    } else if (inst->inst_kind() == mir::inst::InstKind::PtrOffset) {
      auto& i = *inst;
      auto ptrOffsetInst = dynamic_cast<mir::inst::PtrOffsetInst*>(&i);
      if (ptrOffsetInst->offset.index() == 0) {
        int offset = std::get<int>(ptrOffsetInst->offset);
        if (offset == 0) {
          inst = std::make_unique<mir::inst::AssignInst>(ptrOffsetInst->dest,
                                                         ptrOffsetInst->ptr);
        }
      }
    }
    return false;
  }

  void optimize_func(mir::inst::MirFunction& func) {
    for (auto& blkpair : func.basic_blks) {
      env = std::make_shared<Env>(func);
      for (auto& inst : blkpair.second.inst) {
        optimize_inst(inst);
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

}  // namespace optimization::const_merge
