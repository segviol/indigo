#include "value_shift_collapse.hpp"

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "../../include/aixlog.hpp"

namespace optimization::value_shift_collapse {
using namespace mir::inst;
void ValueShiftCollapse::optimize_mir(
    MirPackage &package, std::map<std::string, std::any> &extra_data_repo) {
  for (auto &func : package.functions) {
    optimize_function(func.second);
  }
}

const std::unordered_set<Op> shift_ops = {Op::Shl, Op::Shr, Op::ShrA};

arm::RegisterShiftKind mir_op_to_arm_shift(Op op) {
  switch (op) {
    case Op::Shl:
      return arm::RegisterShiftKind::Lsl;
    case Op::Shr:
      return arm::RegisterShiftKind::Lsr;
    case Op::ShrA:
      return arm::RegisterShiftKind::Asr;
    default:
      throw std::logic_error("Input is not a valid shift!");
  }
}

void ValueShiftCollapse::optimize_function(MirFunction &func) {
  for (auto &bb : func.basic_blks) {
    auto shift_map = std::unordered_map<
        VarId, std::tuple<VarId, arm::RegisterShiftKind, uint8_t>>();
    for (auto &inst_ : bb.second.inst) {
      if (auto inst = dynamic_cast<OpInst *>(&*inst_)) {
        if (shift_ops.find(inst->op) != shift_ops.end() &&
            std::holds_alternative<VarId>(inst->lhs) &&
            std::holds_alternative<int32_t>(inst->rhs)) {
          auto var_id = std::get<VarId>(inst->lhs);
          auto shift = mir_op_to_arm_shift(inst->op);
          auto shift_amount = std::get<int32_t>(inst->rhs);

          shift_map.insert(
              {inst->dest, {var_id, shift, (uint8_t)shift_amount}});
        }
      }
    }

    auto map_shift = [&](Value &val, VarId &id) -> void {
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
      if (auto x = dynamic_cast<AssignInst *>(&i)) {
        x->src.map_if_varid(map_shift);
      } else if (auto x = dynamic_cast<CallInst *>(&i)) {
        for (auto &val : x->params) {
          val.map_if_varid(map_shift);
        }
      } else if (auto x = dynamic_cast<OpInst *>(&i)) {
        const std::set<Op> no_optimize = {Op::Mul, Op::MulSh, Op::Div};
        if (x->lhs.is_immediate() || x->rhs.is_immediate() ||
            no_optimize.find(x->op) != no_optimize.end()) {
          continue;
        } else {
          x->lhs.map_if_varid(map_shift);
          x->rhs.map_if_varid(map_shift);
        }
      } else if (auto x = dynamic_cast<LoadInst *>(&i)) {
      } else if (auto x = dynamic_cast<StoreInst *>(&i)) {
      } else if (auto x = dynamic_cast<LoadOffsetInst *>(&i)) {
        x->offset.map_if_varid(map_shift);
      } else if (auto x = dynamic_cast<StoreOffsetInst *>(&i)) {
        x->offset.map_if_varid(map_shift);
      } else if (auto x = dynamic_cast<PtrOffsetInst *>(&i)) {
      } else if (auto x = dynamic_cast<PhiInst *>(&i)) {
      }
    }
  }
}
}  // namespace optimization::value_shift_collapse
