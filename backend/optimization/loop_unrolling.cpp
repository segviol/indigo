
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
    auto cond = mir::inst::VarId(0);
    for (int i = 0; i < times; i++) {
      var_map.clear();
      if (i) {
        var_map.insert(phi_var_pair);
      }
      for (auto& inst : blk.inst) {
        if (inst->inst_kind() != mir::inst::InstKind::Store) {
          copy_var(inst->dest);
        }
      }
      for (auto& inst : blk.inst) {
        if (inst->inst_kind() == mir::inst::InstKind::Phi) {
          if (i == 0) {
            auto ptr = std::unique_ptr<mir::inst::Inst>(inst->deep_copy());
            auto& i = *ptr;
            auto phi = dynamic_cast<mir::inst::PhiInst*>(&i);
            phi->vars.erase(
                std::find(phi->vars.begin(), phi->vars.end(), change_var));
            ptr->dest = var_map.at(ptr->dest);
            res.push_back(std::move(ptr));
          }
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
          if (inst->inst_kind() == mir::inst::InstKind::Op) {
            auto& i = *inst;
            auto opist = dynamic_cast<mir::inst::OpInst*>(&i);
            if (opist->op == mir::inst::Op::Lt) {
              cond = ptr->dest;
            }
          }
          res.push_back(std::move(ptr));
        }
      }
    }
    if (res.size()) {
      auto& inst = res.front();
      assert(inst->inst_kind() == mir::inst::InstKind::Phi);
      auto& i = *inst;
      auto phi = dynamic_cast<mir::inst::PhiInst*>(&i);
      phi->vars.push_back(mir::inst::VarId(new_change_var.id));
    }
    blk.jump.cond_or_ret = cond;
    return res;
  }

  mir::inst::VarId get_new_VarId() {
    auto end_iter = func.variables.end();
    end_iter--;
    return mir::inst::VarId(end_iter->first + 1);
  }

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
  mir::inst::VarId init_var;
  std::pair<mir::inst::Op, mir::inst::VarId> cmp;  // op must be <
  std::pair<mir::inst::Op, int> change;            // must be +1
  mir::inst::VarId phi_var;
  mir::inst::VarId change_var;
  loop_info(mir::types::LabelId loop_start, mir::inst::VarId init_var,
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
    auto end_iter = func.variables.end();
    end_iter--;
    mir::inst::VarId new_var(end_iter->first + 1);
    func.variables.insert({new_var, func.variables.at(var.id)});
    return new_var;
  }
  int get_times() { return unrolling_times; }

  void rewrite_jump_cond(mir::inst::BasicBlk& blk) {
    auto val = blk.jump.cond_or_ret.value();
    auto iter = blk.inst.rbegin();
    for (; iter != blk.inst.rend(); ++iter) {
      if (iter->get()->dest == val) {
        break;
      }
    }
    auto& x = *iter;
    auto& i = *x;
    auto& inst = *dynamic_cast<mir::inst::OpInst*>(&i);
    ;
    auto multimes = copy_var(inst.dest);
    blk.inst.push_back(std::make_unique<mir::inst::OpInst>(
        multimes, inst.lhs, get_times(), mir::inst::Op::Add));
    auto cond = copy_var(inst.dest);
    blk.inst.push_back(std::make_unique<mir::inst::OpInst>(
        cond, multimes, inst.rhs, mir::inst::Op::Lt));
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
    if (bb_true == blk.id) {
      return res;
    }
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

      auto& i = **iter;
      auto start_cmp_Inst = dynamic_cast<mir::inst::OpInst*>(&i);

      if (start_cmp_Inst->op != mir::inst::Op::Lt ||
          start_cmp_Inst->rhs.is_immediate()) {
        return res;
      }

      auto& loop_blk = func.basic_blks.at(bb_true);
      std::set<mir::inst::VarId> inst_dests;
      for (auto& inst : loop_blk.inst) {
        inst_dests.insert(inst->dest);
      }
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
      start_cond = loop_blk.jump.cond_or_ret.value();
      for (auto& inst : loop_blk.inst) {
        if (inst->dest == start_cond) {
          assert(inst->inst_kind() == mir::inst::InstKind::Op);
          auto& i = *inst;
          auto opInst = dynamic_cast<mir::inst::OpInst*>(&i);
        }
      }

      mir::inst::VarId init_var = mir::inst::VarId(0);
      std::pair<mir::inst::Op, mir::inst::VarId> cmp = {
          start_cmp_Inst->op, std::get<mir::inst::VarId>(start_cmp_Inst->rhs)};
      std::pair<mir::inst::Op, int> change = {mir::inst::Op::Add, 1};
      mir::inst::VarId var;
      mir::inst::VarId change_var;

      bool flag = false;
      for (auto var : phi_vars) {
        if (!inst_dests.count(var)) {
          init_var = var;
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

std::optional<loop_info> find_loop_start(
    mir::inst::MirFunction& func, std::set<mir::types::LabelId> base_blks) {
  std::optional<loop_info> res;
  for (auto& blkpair : func.basic_blks) {
    if (!base_blks.count(blkpair.first) || blkpair.first == 13) {
      continue;
    }
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
  LOG(TRACE) << "init_val: " << info.init_var << std::endl;
  LOG(TRACE) << "change : "
             << mir::inst::OpInst(info.init_var, info.init_var,
                                  info.change.second, info.change.first)
             << std::endl;
  LOG(TRACE) << "continue when: "
             << mir::inst::OpInst(mir::inst::VarId(0), info.init_var,
                                  info.cmp.second, info.cmp.first)
             << std::endl;
  Rewriter rwt(func, blk, {info.phi_var, info.init_var}, info.change_var);
  info.rewrite_jump_cond(func.basic_blks.at(info.loop_start));

  auto end_iter = func.basic_blks.end();
  end_iter--;
  end_iter--;

  // extra loop
  auto extra_loop_start = end_iter->first + 1;
  auto extra_loop = end_iter->first + 2;
  func.basic_blks.insert(
      {extra_loop_start, mir::inst::BasicBlk(extra_loop_start)});
  func.basic_blks.insert({extra_loop, mir::inst::BasicBlk(extra_loop)});
  auto& extra_loop_start_blk = func.basic_blks.at(extra_loop_start);
  auto& extra_loop_blk = func.basic_blks.at(extra_loop);
  extra_loop_start_blk.preceding =
      std::set<mir::types::LabelId>{loop_start.id, blk.id};
  extra_loop_blk.preceding =
      std::set<mir::types::LabelId>{extra_loop_start_blk.id, extra_loop_blk.id};

  // original loop's end
  func.basic_blks.at(loop_start.jump.bb_false).preceding =
      std::set<mir::types::LabelId>{extra_loop_start_blk.id, extra_loop_blk.id};

  // copy original loop insts in extra loop blk
  for (auto& inst : blk.inst) {
    extra_loop_blk.inst.push_back(
        std::unique_ptr<mir::inst::Inst>(inst->deep_copy()));
  }
  extra_loop_blk.jump = mir::inst::JumpInstruction(
      mir::inst::JumpInstructionKind::BrCond, extra_loop_blk.id,
      loop_start.jump.bb_false, blk.jump.cond_or_ret,
      mir::inst::JumpKind::Loop);

  // after copy then can be rewrrote
  info.rewrite_jump_cond(blk);

  // init extra loop start 's insts
  int times = info.get_times();
  mir::inst::VarId new_change_var;
  auto insts = rwt.get_insts(times, new_change_var);
  auto new_phi = info.copy_var(info.init_var);
  extra_loop_start_blk.inst.push_back(std::make_unique<mir::inst::PhiInst>(
      new_phi, std::vector<mir::inst::VarId>{info.init_var, new_change_var}));
  auto cond = info.copy_var(blk.jump.cond_or_ret.value());
  extra_loop_start_blk.inst.push_back(std::make_unique<mir::inst::OpInst>(
      cond, new_phi, info.cmp.second, info.cmp.first));
  extra_loop_start_blk.jump = mir::inst::JumpInstruction(
      mir::inst::JumpInstructionKind::BrCond, extra_loop_blk.id,
      loop_start.jump.bb_false, cond, mir::inst::JumpKind::Loop);

  blk.jump = mir::inst::JumpInstruction(
      mir::inst::JumpInstructionKind::BrCond, blk.id, extra_loop_start_blk.id,
      blk.jump.cond_or_ret.value(), mir::inst::JumpKind::Loop);
  auto& inst = extra_loop_blk.inst.front();
  auto& i = *inst;
  assert(inst->inst_kind() == mir::inst::InstKind::Phi);
  auto phiInst = dynamic_cast<mir::inst::PhiInst*>(&i);
  phiInst->vars.erase(
      std::find(phiInst->vars.begin(), phiInst->vars.end(), info.init_var));
  phiInst->vars.push_back(new_phi);
  blk.inst.clear();
  for (auto& inst : insts) {
    blk.inst.push_back(std::move(inst));
  }
  func.basic_blks.at(loop_start.jump.bb_false)
      .preceding.erase(loop_start.jump.bb_true);

  loop_start.jump = mir::inst::JumpInstruction(
      mir::inst::JumpInstructionKind::BrCond, loop_start.jump.bb_true,
      extra_loop_start_blk.id, loop_start.jump.cond_or_ret.value());
  // auto& end_loop_blk = func.basic_blks.at(loop_start.jump.bb_true);
  // for (auto iter = end_loop_blk.inst.begin();
  //      iter != end_loop_blk.inst.end() &&
  //      iter->get()->inst_kind() == mir::inst::InstKind::Phi;
  //      iter++) {
  //   auto& inst = *iter;
  //   auto& i = *inst;
  //   auto phiInst = dynamic_cast<mir::inst::PhiInst*>(&i);
  //   if (phiInst->useVars().count(info.change_var)) {
  //     phiInst->vars.erase(std::find(phiInst->vars.begin(),
  //     phiInst->vars.end(),
  //                                   info.change_var));
  //   }
  //   if (times) {
  //     if (phiInst->useVars().count(info.init_var)) {
  //       phiInst->vars.erase(std::find(
  //           phiInst->vars.begin(), phiInst->vars.end(),
  //           info.init_var));
  //       phiInst->vars.push_back(new_change_var);
  //     }
  //   }
  // }
}

void Loop_Unrolling::optimize_func(mir::inst::MirFunction& func) {
  std::set<mir::types::LabelId> base_blks;
  for (auto& blkpair : func.basic_blks) {
    base_blks.insert(blkpair.first);
  }
  auto& startblk = func.basic_blks.begin()->second;
  while (true) {
    auto info = find_loop_start(func, base_blks);
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
