
#include "const_loop_expand.hpp"

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

namespace optimization::loop_expand {

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

  std::vector<std::unique_ptr<mir::inst::Inst>> get_insts(int times) {
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
            phi_var_pair.second = ptr->dest;
          }
          std::cout << *ptr << std::endl;
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
  mir::types::LabelId loop_start;
  std::pair<mir::inst::VarId, int> init_var;
  mir::inst::Op in_op;
  int in_value;
  mir::inst::Op change_op;
  int change_value;
  mir::inst::VarId phi_var;
  mir::inst::VarId change_var;
  loop_info(mir::types::LabelId loop_start,
            std::pair<mir::inst::VarId, int> init_var, mir::inst::Op in_op,
            int in_value, mir::inst::Op change_op, int change_value,
            mir::inst::VarId phi_var, mir::inst::VarId change_var)
      : loop_start(loop_start),
        init_var(init_var),
        in_op(in_op),
        in_value(in_value),
        change_op(change_op),
        change_value(change_value),
        phi_var(phi_var),
        change_var(change_var) {}
  int get_times() {
    int res = 0;
    loop_info init(*this);
    while (init < *this) {
      init++;
      res++;
    }
    return res;
  }

  bool operator<(const loop_info& other) const {
    switch (in_op) {
      case mir::inst::Op::Eq:
        return init_var.second == other.in_value;
      case mir::inst::Op::Neq:
        return init_var.second != other.in_value;
      case mir::inst::Op::Lt:
        return init_var.second < other.in_value;
      case mir::inst::Op::Lte:
        return init_var.second <= other.in_value;
      case mir::inst::Op::Gt:
        return init_var.second > other.in_value;
      case mir::inst::Op::Gte:
        return init_var.second >= other.in_value;
      default: {
        LOG(ERROR) << "ERROR ! Unrecognized in_op";
        mir::inst::display_op(LOG(ERROR), in_op);
        LOG(ERROR) << " ! " << std::endl;
        return false;
      }
    }
  }

  loop_info operator++(int) {
    loop_info res(*this);
    switch (change_op) {
      case mir::inst::Op::Add: {
        init_var.second += change_value;
        break;
      }
      case mir::inst::Op::Sub: {
        init_var.second -= change_value;
        break;
      }
      case mir::inst::Op::Mul: {
        init_var.second *= change_value;
        break;
      }
      case mir::inst::Op::Div: {
        init_var.second /= change_value;
        break;
      }
      case mir::inst::Op::Rem: {
        init_var.second %= change_value;
        break;
      }
      default: {
        LOG(ERROR) << "ERROR ! Unrecognized change_op";
        mir::inst::display_op(LOG(ERROR), change_op);
        LOG(ERROR) << " ! " << std::endl;
        break;
      }
    }
    return res;
  }
};

void Const_Loop_Expand::optimize_mir(
    mir::inst::MirPackage& mir,
    std::map<std::string, std::any>& extra_data_repo) {
  for (auto& f : mir.functions) {
    if (f.second.type->is_extern) continue;
    optimize_func(f.second);
  }
}

std::optional<loop_info> find_loop_start(mir::inst::MirFunction& func) {
  std::optional<loop_info> res;
  for (auto& blkpair : func.basic_blks) {
    auto& blk = blkpair.second;
    if (blk.jump.jump_kind == mir::inst::JumpKind::Loop) {
      auto bb_true = blk.jump.bb_true;
      auto bb_false = blk.jump.bb_false;
      if (bb_true == func.basic_blks.at(bb_true).jump.bb_true &&
          bb_false == func.basic_blks.at(bb_true).jump.bb_false) {
        auto cond = blk.jump.cond_or_ret.value();
        auto iter = blk.inst.begin();
        for (; iter != blk.inst.end(); iter++) {
          auto& inst = *iter;
          if (inst->dest == cond) {
            break;
          }
        }
        if (iter == blk.inst.end() ||
            iter->get()->inst_kind() != mir::inst::InstKind::Op) {
          continue;
        }
        std::map<mir::inst::VarId, int> const_map;
        for (auto& inst : blk.inst) {
          if (inst->inst_kind() == mir::inst::InstKind::Assign) {
            auto& i = *inst;
            auto ist = dynamic_cast<mir::inst::AssignInst*>(&i);
            if (ist->src.is_immediate()) {
              const_map.insert({inst->dest, std::get<int>(ist->src)});
            }
          }
        }
        auto& i = **iter;
        auto opInst = dynamic_cast<mir::inst::OpInst*>(&i);

        if (!opInst->lhs.is_immediate() &&
                !const_map.count(std::get<mir::inst::VarId>(opInst->lhs)) ||
            !opInst->rhs.is_immediate() &&
                !const_map.count(std::get<mir::inst::VarId>(opInst->rhs))) {
          continue;
        }

        auto& loop_blk = func.basic_blks.at(bb_true);

        auto phi_count = 0;
        mir::inst::VarId phi_dest;
        std::set<mir::inst::VarId> phi_vars;
        for (auto& inst : loop_blk.inst) {
          if (inst->inst_kind() == mir::inst::InstKind::Phi &&
              inst->useVars().size() ==
                  2) {  // one for initial val,and one for loop val
            phi_vars = inst->useVars();
            phi_dest = inst->dest;
            phi_count++;
          }
        }
        if (phi_count != 1) {
          continue;
        }
        for (; iter != blk.inst.end(); iter++) {
          auto& inst = *iter;
          if (inst->dest == cond) {
            break;
          }
        }
        cond = loop_blk.jump.cond_or_ret.value();
        mir::inst::Op in_op;
        for (auto& inst : loop_blk.inst) {
          if (inst->dest == cond) {
            assert(inst->inst_kind() == mir::inst::InstKind::Op);
            auto& i = *inst;
            auto opInst = dynamic_cast<mir::inst::OpInst*>(&i);
            in_op = opInst->op;
          }
        }

        std::pair<mir::inst::VarId, int> init_var = {mir::inst::VarId(0), 0};

        int in_value;
        mir::inst::Op change_op;
        int change_value;
        mir::inst::VarId var;
        mir::inst::VarId change_var;

        bool flag = false;
        for (auto var : phi_vars) {
          if (const_map.count(var)) {
            init_var = {var, const_map.at(var)};
            mir::inst::Value value(0);
            if (!opInst->lhs.is_immediate() &&
                std::get<mir::inst::VarId>(opInst->lhs)) {
              value = opInst->rhs;
            } else {
              value = opInst->rhs;
            }
            if (value.is_immediate()) {
              in_value = std::get<int>(value);
            } else {
              in_value = const_map.at(std::get<mir::inst::VarId>(value));
            }
            continue;
          } else {
            for (auto& inst : loop_blk.inst) {
              change_var = var;
              if (inst->dest == var &&
                  inst->inst_kind() == mir::inst::InstKind::Op) {
                mir::inst::Value val(0);
                auto& i = *inst;
                auto ist = dynamic_cast<mir::inst::OpInst*>(&i);
                if (!ist->lhs.is_immediate() &&
                    std::get<mir::inst::VarId>(ist->lhs) == phi_dest) {
                  val = ist->rhs;
                } else {
                  val = ist->lhs;
                }
                if (val.is_immediate()) {
                  change_value = std::get<int>(val);
                  change_op = ist->op;
                  flag = true;
                }
              }
            }
          }
        }

        if (!flag) {
          continue;
        }
        return loop_info(blk.id, init_var, in_op, in_value, change_op,
                         change_value, phi_dest, change_var);
      }
    }
  }
  return res;
}

void expand_loop(mir::inst::MirFunction& func, mir::inst::BasicBlk& blk,
                 loop_info info) {
  auto& loop_start = func.basic_blks.at(info.loop_start);
  LOG(TRACE) << "expand loop starts at  " << info.loop_start << std::endl;
  LOG(TRACE) << "init_val: "
             << mir::inst::AssignInst(info.init_var.first, info.init_var.second)
             << std::endl;
  LOG(TRACE) << "change : "
             << mir::inst::OpInst(info.init_var.first, info.init_var.first,
                                  info.change_value, info.change_op)
             << std::endl;
  LOG(TRACE) << "continue when: "
             << mir::inst::OpInst(mir::inst::VarId(0), info.init_var.first,
                                  info.in_value, info.in_op)
             << std::endl;
  Rewriter rwt(func, blk, {info.phi_var, info.init_var.first}, info.change_var);
  blk.jump = mir::inst::JumpInstruction(mir::inst::JumpInstructionKind::Br,
                                        blk.jump.bb_false);
  int times = info.get_times();

  auto insts = rwt.get_insts(times);
  blk.inst.clear();
  for (auto& inst : insts) {
    loop_start.inst.push_back(std::move(inst));
  }
  func.basic_blks.at(loop_start.jump.bb_false)
      .preceding.erase(loop_start.jump.bb_true);

  loop_start.jump = mir::inst::JumpInstruction(
      mir::inst::JumpInstructionKind::Br, loop_start.jump.bb_false);
  func.basic_blks.erase(blk.id);
}

void Const_Loop_Expand::optimize_func(mir::inst::MirFunction& func) {
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

    if (size >= 1024) {
      return;
    }
    expand_loop(func, blk, info.value());
  }
}

}  // namespace optimization::loop_expand
