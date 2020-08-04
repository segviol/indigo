#include "./exit_ahead.hpp"

#include <deque>
#include <memory>
#include <vector>

#include "../../include/aixlog.hpp"
#include "../optimization/optimization.hpp"

namespace optimization::exit_ahead {

struct Rewriter {
  mir::inst::BasicBlk& blk;
  Rewriter(mir::inst::BasicBlk& blk) : blk(blk) {}
  std::vector<std::unique_ptr<mir::inst::Inst>> get_insts(
      mir::inst::VarId phi_var) {
    std::vector<std::unique_ptr<mir::inst::Inst>> res;
    for (auto& inst : blk.inst) {
      auto& i = *inst;
      if (inst->inst_kind() == mir::inst::InstKind::Call) {
        auto callInst = dynamic_cast<mir::inst::CallInst*>(&i);
        res.push_back(std::make_unique<mir::inst::CallInst>(
            callInst->dest, callInst->func, callInst->params));
      }
    }
    res.push_back(
        std::make_unique<mir::inst::AssignInst>(mir::inst::VarId(0), phi_var));
    return res;
  }
  void modify_jump(mir::inst::JumpInstruction& jump) {
    jump.kind = mir::inst::JumpInstructionKind::Return;
    jump.cond_or_ret = mir::inst::VarId(0);
  }
};

void Exit_Ahead::optimize_mir(
    mir::inst::MirPackage& mir,
    std::map<std::string, std::any>& extra_data_repo) {
  for (auto& f : mir.functions) {
    if (f.second.type->is_extern) continue;
    optimize_func(f.second);
  }
}

void Exit_Ahead::optimize_func(mir::inst::MirFunction& func) {
  auto end_iter = func.basic_blks.end();
  end_iter--;
  if (end_iter->first < 1000000) {  // merged already
    return;
  }
  auto& end_blk = end_iter->second;
  std::set<mir::inst::VarId> phi_vars;
  mir::inst::VarId phi_var(0);
  for (auto& inst : end_blk.inst) {
    if (inst->inst_kind() == mir::inst::InstKind::Phi) {
      if (phi_vars.size() > 0) {  // multiple phis in end blk
        return;
      }
      phi_vars = inst->useVars();
    } else if (inst->inst_kind() == mir::inst::InstKind::Assign) {
      if (inst->dest.id != 0) {
        return;  // illegal assign
      }
      phi_var = *inst->useVars().begin();
    } else if (inst->inst_kind() != mir::inst::InstKind::Call) {  // not free
      return;
    }
  }
  auto rwt = Rewriter(end_iter->second);
  for (auto pre : end_blk.preceding) {
    auto& pre_blk = func.basic_blks.at(pre);
    for (auto& inst : pre_blk.inst) {
      if (phi_vars.count(inst->dest)) {
        phi_var = inst->dest;
        break;
      }
    }
    if (pre_blk.inst.size()) {
      auto iter = pre_blk.inst.end() - 1;
      if (iter->get()->dest == phi_var) {
        for (auto& inst : rwt.get_insts(phi_var)) {
          if (inst->inst_kind() == mir::inst::InstKind::Call) {
            pre_blk.inst.insert(pre_blk.inst.end() - 1, std::move(inst));
          } else {
            pre_blk.inst.push_back(std::move(inst));
          }
        }
      } else {
        for (auto& inst : rwt.get_insts(phi_var)) {
          pre_blk.inst.push_back(std::move(inst));
        }
      }
    }

    rwt.modify_jump(pre_blk.jump);
  }
  // func.basic_blks.erase(end_iter);
}

}  // namespace optimization::exit_ahead
