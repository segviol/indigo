
#include "./loop_unrolling.hpp"

#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <vector>

#include "../../include/aixlog.hpp"

namespace optimization::loop_unrolling {

struct Rewriter {
  mir::inst::MirFunction& func;
  mir::inst::BasicBlk& blk;
  std::map<mir::inst::VarId, mir::inst::VarId> var_map;
  std::pair<mir::inst::VarId, mir::inst::VarId> phi_var_pair;
  mir::inst::VarId change_var;
  int varId;
  Rewriter(mir::inst::MirFunction& func, mir::inst::BasicBlk& blk,
           std::pair<mir::inst::VarId, mir::inst::VarId> phi_var_pair,
           mir::inst::VarId change_var)
      : func(func),
        blk(blk),
        phi_var_pair(phi_var_pair),
        change_var(change_var) {
    auto end_iter = func.variables.end();
    end_iter--;
    varId = end_iter->first;
  }

  std::vector<std::unique_ptr<mir::inst::Inst>> get_insts(
      int times, mir::inst::VarId& new_change_var) {
    std::vector<std::unique_ptr<mir::inst::Inst>> res;
    while (times--) {
      var_map.clear();
      var_map.insert(phi_var_pair);
      for (auto& inst : blk.inst) {
        if (inst->inst_kind() != mir::inst::InstKind::Store) {
          copy_var(inst->dest);
        }
      }
      for (auto& inst : blk.inst) {
        if (inst->inst_kind() == mir::inst::InstKind::Phi) {
          continue;
        } else {
          auto ptr = std::unique_ptr<mir::inst::Inst>(inst->deep_copy());
          bool flag = inst->dest == change_var;
          for (auto var : ptr->useVars()) {
            if (var_map.count(var)) ptr->replace(var, var_map.at(var));
          }
          if (var_map.count(ptr->dest)) {
            ptr->dest = var_map.at(ptr->dest);
          }
          if (flag) {
            new_change_var = ptr->dest;
            phi_var_pair.second = ptr->dest;
          }
          res.push_back(std::move(ptr));
        }
      }
    }
    return res;
  }

  mir::inst::VarId get_new_VarId() { return ++varId; }

  mir::inst::VarId copy_var(mir::inst::VarId var) {
    if (var_map.count(var)) {
      return var_map.at(var);
    }
    auto id = get_new_VarId();
    var_map.insert({var, id});
    func.variables.insert({id, func.variables.at(var)});
    return id;
  }
};

struct loop_info {
  const int unrolling_times = 4;
  mir::types::LabelId loop_start;
  mir::inst::MirFunction& func;
  std::pair<mir::inst::VarId, mir::inst::Value> init_var;
  std::pair<mir::inst::Op, mir::inst::VarId> cmp;  // op must be <
  std::pair<mir::inst::Op, int> change;            // must be +1
  mir::inst::VarId phi_var;
  mir::inst::VarId change_var;
  loop_info(mir::types::LabelId loop_start,
            std::pair<mir::inst::VarId, mir::inst::Value> init_var,
            std::pair<mir::inst::Op, mir::inst::VarId> cmp,
            std::pair<mir::inst::Op, int> change, mir::inst::VarId phi_var,
            mir::inst::VarId change_var, mir::inst::MirFunction& func)
      : loop_start(loop_start),
        init_var(init_var),
        cmp(cmp),
        change(change),
        phi_var(phi_var),
        change_var(change_var),
        func(func) {}
  mir::inst::VarId copy_var(mir::inst::VarId var) {
    auto end_iter = func.basic_blks.end();
    end_iter--;
    mir::inst::VarId new_var(end_iter->first + 1);
    func.variables.insert({new_var, func.variables.at(var.id)});
    return new_var;
  }
  int get_times() { return unrolling_times; }

  void rewrite_jump_cond(mir::inst::BasicBlk& blk, mir::inst::OpInst& inst) {
    auto val = blk.jump.cond_or_ret.value();

    auto sub = copy_var(inst.dest);
    blk.inst.push_back(std::make_unique<mir::inst::OpInst>(
        sub, inst.rhs, init_var.second, mir::inst::Op::Sub));
    auto multimes = copy_var(inst.dest);
    blk.inst.push_back(std::make_unique<mir::inst::OpInst>(
        multimes, inst.lhs, get_times(), mir::inst::Op::Mul));
    auto cond = copy_var(inst.dest);
    blk.inst.push_back(std::make_unique<mir::inst::OpInst>(cond, multimes, sub,
                                                           mir::inst::Op::Lt));
    blk.jump.cond_or_ret = cond;
  }
};

void Loop_Unrolling::optimize_mir(
    mir::inst::MirPackage& mir,
    std::map<std::string, std::any>& extra_data_repo) {
  for (auto& f : mir.functions) {
    if (f.second.type->is_extern) continue;
    optimize_func(f.second);
  }
}

std::optional<loop_info> is_loop_start(mir::inst::MirFunction& func,
                                       mir::inst::BasicBlk& blk) {
  std::optional<loop_info> res;
  if (blk.jump.jump_kind == mir::inst::JumpKind::Loop) {
    auto bb_true = blk.jump.bb_true;
    auto bb_false = blk.jump.bb_false;
    if (bb_true == func.basic_blks.at(bb_true).jump.bb_true &&
        bb_false == func.basic_blks.at(bb_true).jump.bb_false) {
      auto start_cond = blk.jump.cond_or_ret.value();
      auto iter = blk.inst.begin();
      for (; iter != blk.inst.end(); iter++) {
        auto& inst = *iter;
        if (inst->dest == start_cond) {
          break;
        }
      }
      if (iter == blk.inst.end() ||
          iter->get()->inst_kind() != mir::inst::InstKind::Op) {
        return res;
      }
      std::map<mir::inst::VarId, mir::inst::Value> assign_map;
      for (auto& inst : blk.inst) {
        if (inst->inst_kind() == mir::inst::InstKind::Assign) {
          auto& i = *inst;
          auto ist = dynamic_cast<mir::inst::AssignInst*>(&i);
          assign_map.insert({inst->dest, ist->src});
        }
      }
      auto& i = **iter;
      auto start_cmp_Inst = dynamic_cast<mir::inst::OpInst*>(&i);

      if (!start_cmp_Inst->lhs.is_immediate() &&
              !assign_map.count(
                  std::get<mir::inst::VarId>(start_cmp_Inst->lhs)) ||
          start_cmp_Inst->op != mir::inst::Op::Lt ||
          start_cmp_Inst->rhs.is_immediate()) {
        return res;
      }

      auto& loop_blk = func.basic_blks.at(bb_true);
      auto& loop_cond = loop_blk.jump.cond_or_ret.value();
      iter = loop_blk.inst.begin();
      for (; iter != loop_blk.inst.end(); iter++) {
        auto& inst = *iter;
        if (inst->dest == loop_cond) {
          break;
        }
      }
      if (iter == loop_blk.inst.end() ||
          iter->get()->inst_kind() != mir::inst::InstKind::Op) {
        return res;
      }
      auto& i1 = **iter;
      auto loop_cmp_Inst = dynamic_cast<mir::inst::OpInst*>(&i1);
      if (loop_cmp_Inst->op != mir::inst::Op::Lt ||
          loop_cmp_Inst->rhs != start_cmp_Inst->rhs) {
        return res;
      }

      auto phi_count = 0;
      mir::inst::VarId phi_dest;
      std::set<mir::inst::VarId> phi_vars;
      for (auto& inst : loop_blk.inst) {
        if (inst->inst_kind() == mir::inst::InstKind::Phi) {
          phi_count++;
          if (inst->useVars().size() ==
              2) {  // one for initial val,and one for loop val
            phi_vars = inst->useVars();
            phi_dest = inst->dest;
          }
        }
      }
      if (phi_count != 1) {
        return res;
      }
      for (; iter != blk.inst.end(); iter++) {
        auto& inst = *iter;
        if (inst->dest == start_cond) {
          break;
        }
      }
      start_cond = loop_blk.jump.cond_or_ret.value();
      mir::inst::Op in_op;
      for (auto& inst : loop_blk.inst) {
        if (inst->dest == start_cond) {
          assert(inst->inst_kind() == mir::inst::InstKind::Op);
          auto& i = *inst;
          auto opInst = dynamic_cast<mir::inst::OpInst*>(&i);
          in_op = opInst->op;
        }
      }

      std::pair<mir::inst::VarId, mir::inst::Value> init_var = {
          mir::inst::VarId(0), 0};
      std::pair<mir::inst::Op, mir::inst::VarId> cmp = {
          start_cmp_Inst->op, std::get<mir::inst::VarId>(start_cmp_Inst->rhs)};
      std::pair<mir::inst::Op, int> change = {mir::inst::Op::Add, 1};
      mir::inst::VarId var;
      mir::inst::VarId change_var;

      bool flag = false;
      for (auto var : phi_vars) {
        if (assign_map.count(var)) {
          init_var = {var, assign_map.at(var)};
        } else {
          change_var = var;
          for (auto& inst : loop_blk.inst) {
            if (inst->dest == loop_blk.jump.cond_or_ret.value() &&
                !inst->useVars().count(var)) {
              return res;
            }
            if (inst->dest == var &&
                inst->inst_kind() == mir::inst::InstKind::Op) {
              mir::inst::Value val(0);
              auto& i = *inst;
              auto ist = dynamic_cast<mir::inst::OpInst*>(&i);
              if (ist->op != mir::inst::Op::Add &&
                  ist->rhs != mir::inst::Value(1)) {
                return res;
              }
            }
          }
        }
      }

      return loop_info(blk.id, init_var, cmp, change, phi_dest, change_var,
                       func);
    }
  }
  return res;
}

std::optional<loop_info> find_loop_start(mir::inst::MirFunction& func) {
  std::optional<loop_info> res;
  for (auto& blkpair : func.basic_blks) {
    auto& blk = blkpair.second;
    auto res = is_loop_start(func, blk);
    if (res.has_value()) {
      return res;
    }
  }
  return res;
}

void expand_loop(mir::inst::MirFunction& func, mir::inst::BasicBlk& blk,
                 loop_info info) {
  auto& loop_start = func.basic_blks.at(info.loop_start);
  LOG(TRACE) << "unroll loop starts at  " << info.loop_start << std::endl;
  LOG(TRACE) << "init_val: "
             << mir::inst::AssignInst(info.init_var.first, info.init_var.second)
             << std::endl;
  LOG(TRACE) << "change : "
             << mir::inst::OpInst(info.init_var.first, info.init_var.first,
                                  info.change.second, info.change.first)
             << std::endl;
  LOG(TRACE) << "continue when: "
             << mir::inst::OpInst(mir::inst::VarId(0), info.init_var.first,
                                  info.cmp.second, info.cmp.first)
             << std::endl;
  Rewriter rwt(func, blk, {info.phi_var, info.init_var.first}, info.change_var);
  blk.jump = mir::inst::JumpInstruction(mir::inst::JumpInstructionKind::Br,
                                        blk.jump.bb_false);
  int times = info.get_times();
  mir::inst::VarId new_change_var;
  auto insts = rwt.get_insts(times, new_change_var);
  blk.inst.clear();
  for (auto& inst : insts) {
    loop_start.inst.push_back(std::move(inst));
  }
  func.basic_blks.at(loop_start.jump.bb_false)
      .preceding.erase(loop_start.jump.bb_true);

  loop_start.jump = mir::inst::JumpInstruction(
      mir::inst::JumpInstructionKind::Br, loop_start.jump.bb_false);
  func.basic_blks.erase(blk.id);
  auto& end_loop_blk = func.basic_blks.at(loop_start.jump.bb_true);
  for (auto iter = end_loop_blk.inst.begin();
       iter != end_loop_blk.inst.end() &&
       iter->get()->inst_kind() == mir::inst::InstKind::Phi;
       iter++) {
    auto& inst = *iter;
    auto& i = *inst;
    auto phiInst = dynamic_cast<mir::inst::PhiInst*>(&i);
    if (phiInst->useVars().count(info.change_var)) {
      phiInst->vars.erase(std::find(phiInst->vars.begin(), phiInst->vars.end(),
                                    info.change_var));
    }
    if (times) {
      if (phiInst->useVars().count(info.init_var.first)) {
        phiInst->vars.erase(std::find(
            phiInst->vars.begin(), phiInst->vars.end(), info.init_var.first));
        phiInst->vars.push_back(new_change_var);
      }
    }
  }
}

void Loop_Unrolling::optimize_func(mir::inst::MirFunction& func) {
  auto& startblk = func.basic_blks.begin()->second;
  while (true) {
    auto info = find_loop_start(func);
    if (!info.has_value()) {
      break;
    }
    int size = func.variable_table_size();
    std::set<mir::inst::VarId> vars;
    auto& blk =
        func.basic_blks.at(func.basic_blks.at(info->loop_start).jump.bb_true);

    int times = info->get_times();
    for (auto& inst : blk.inst) {
      if (inst->inst_kind() == mir::inst::InstKind::Phi) {
        continue;
      }
      size += times * func.variables.at(inst->dest.id).size();
    }

    // if (size >= 1024) {
    //   return;
    // }
    expand_loop(func, blk, info.value());
  }
}

}  // namespace optimization::loop_unrolling
