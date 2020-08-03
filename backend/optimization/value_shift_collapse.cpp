#include "value_shift_collapse.hpp"

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "../../include/aixlog.hpp"

namespace optimization::value_shift_collapse {
void ValueShiftCollapse::optimize_mir(
    mir::inst::MirPackage &package,
    std::map<std::string, std::any> &extra_data_repo) {
  for (auto &func : package.functions) {
    optimize_function(func.second);
  }
}

const std::unordered_set<mir::inst::Op> shift_ops = {
    mir::inst::Op::Shl, mir::inst::Op::Shr, mir::inst::Op::ShrA};

arm::RegisterShiftKind mir_op_to_arm_shift(mir::inst::Op op) {
  switch (op) {
    case mir::inst::Op::Shl:
      return arm::RegisterShiftKind::Lsl;
    case mir::inst::Op::Shr:
      return arm::RegisterShiftKind::Lsr;
    case mir::inst::Op::ShrA:
      return arm::RegisterShiftKind::Asr;
    default:
      throw std::logic_error("Input is not a valid shift!");
  }
}

void ValueShiftCollapse::optimize_function(mir::inst::MirFunction &func) {
  for (auto &bb : func.basic_blks) {
    auto shift_map = std::unordered_map<
        mir::inst::VarId,
        std::tuple<mir::inst::VarId, arm::RegisterShiftKind, uint8_t>>();
    for (auto &inst_ : bb.second.inst) {
      if (auto inst = dynamic_cast<mir::inst::OpInst *>(&*inst_)) {
        if (shift_ops.find(inst->op) != shift_ops.end() &&
            std::holds_alternative<mir::inst::VarId>(inst->lhs) &&
            std::holds_alternative<int32_t>(inst->rhs)) {
          auto var_id = std::get<mir::inst::VarId>(inst->lhs);
          auto shift = mir_op_to_arm_shift(inst->op);
          auto shift_amount = std::get<int32_t>(inst->rhs);

          shift_map.insert(
              {inst->dest, {var_id, shift, (uint8_t)shift_amount}});
        }
      }
    }

    auto map_shift = [&](mir::inst::Value &val, mir::inst::VarId &id) -> void {
      if (auto x = shift_map.find(id); x != shift_map.end()) {
        auto [var_id, shift, amount] = x->second;
        if (val.shift_amount == 0 || val.shift == shift) {
          val.shift = shift;
          val.shift_amount += amount;
          id = var_id;
        }
      }
    };

    for (auto &inst : bb.second.inst) {
      auto &i = *inst;
      if (auto x = dynamic_cast<mir::inst::AssignInst *>(&i)) {
        x->src.map_if_varid(map_shift);
      } else if (auto x = dynamic_cast<mir::inst::CallInst *>(&i)) {
        for (auto &val : x->params) {
          val.map_if_varid(map_shift);
        }
      } else if (auto x = dynamic_cast<mir::inst::OpInst *>(&i)) {
        x->lhs.map_if_varid(map_shift);
        x->rhs.map_if_varid(map_shift);
      } else if (auto x = dynamic_cast<mir::inst::LoadInst *>(&i)) {
      } else if (auto x = dynamic_cast<mir::inst::StoreInst *>(&i)) {
      } else if (auto x = dynamic_cast<mir::inst::LoadOffsetInst *>(&i)) {
        x->offset.map_if_varid(map_shift);
      } else if (auto x = dynamic_cast<mir::inst::StoreOffsetInst *>(&i)) {
        x->offset.map_if_varid(map_shift);
      } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst *>(&i)) {
      } else if (auto x = dynamic_cast<mir::inst::PhiInst *>(&i)) {
      }
    }
  }
}
}  // namespace optimization::value_shift_collapse
