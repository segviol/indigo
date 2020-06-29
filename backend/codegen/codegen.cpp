#include "codegen.hpp"

#include <cassert>
#include <exception>
#include <memory>
#include <optional>
#include <typeinfo>

namespace backend::codegen {

using namespace arm;

arm::Function Codegen::translate_function() {
  for (auto& bb : func.basic_blks) {
    translate_basic_block(bb.second);
  }
  return arm::Function{func.name, std::move(this->inst),
                       std::move(this->consts)};
}

void Codegen::translate_basic_block(mir::inst::BasicBlk& blk) {
  for (auto& inst : blk.inst) {
    auto& i = *inst;
    if (auto x = dynamic_cast<mir::inst::OpInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::RefInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::PhiInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst*>(&i)) {
      translate_inst(*x);
    } else {
      throw new std::bad_cast();
    }
  }
}

arm::Reg Codegen::get_or_alloc_vgp(mir::inst::VarId v) {
  auto found = reg_map.find(v);
  if (found != reg_map.end()) {
    assert(arm::register_type(found->second) ==
           arm::RegisterKind::VirtualGeneralPurpose);
    return found->second;
  } else {
    return alloc_vgp();
  }
}

arm::Reg Codegen::get_or_alloc_vd(mir::inst::VarId v) {
  auto found = reg_map.find(v);
  if (found != reg_map.end()) {
    assert(arm::register_type(found->second) ==
           arm::RegisterKind::VirtualDoubleVector);
    return found->second;
  } else {
    return alloc_vd();
  }
}

arm::Reg Codegen::get_or_alloc_vq(mir::inst::VarId v) {
  auto found = reg_map.find(v);
  if (found != reg_map.end()) {
    assert(arm::register_type(found->second) ==
           arm::RegisterKind::VirtualQuadVector);
    return found->second;
  } else {
    return alloc_vq();
  }
}

arm::Reg Codegen::alloc_vgp() {
  return arm::make_register(arm::RegisterKind::VirtualGeneralPurpose,
                            vreg_gp_counter++);
}

arm::Reg Codegen::alloc_vd() {
  return arm::make_register(arm::RegisterKind::VirtualDoubleVector,
                            vreg_gp_counter++);
}

arm::Reg Codegen::alloc_vq() {
  return arm::make_register(arm::RegisterKind::VirtualQuadVector,
                            vreg_gp_counter++);
}

arm::Operand2 Codegen::translate_value_to_operand2(mir::inst::Value& v) {
  if (auto x = v.get_if<int32_t>()) {
    auto int_value = *x;
    if (is_valid_immediate(int_value)) {
      return int_value;
    } else {
      auto reg = alloc_vgp();
      uint32_t imm_u = int_value;
      inst.push_back(
          std::make_unique<Arith2Inst>(arm::OpCode::Mov, reg, imm_u & 0xffff));
      if (imm_u > 0xffff) {
        inst.push_back(std::make_unique<Arith2Inst>(arm::OpCode::MovT, reg,
                                                    imm_u & 0xffff0000));
      }
      return reg;
    }
  } else if (auto x = v.get_if<mir::inst::VarId>()) {
    return get_or_alloc_vgp(*x);
  } else {
    throw new prelude::UnreachableException();
  }
}

arm::Reg Codegen::translate_value_to_reg(mir::inst::Value& v) {
  // TODO
  throw new std::exception();
}

arm::Reg Codegen::translate_var_reg(mir::inst::VarId v) {
  return get_or_alloc_vgp(v);
}

void Codegen::translate_inst(mir::inst::AssignInst& i) {
  if (i.src.is_immediate()) {
    auto imm = *i.src.get_if<int32_t>();
    uint32_t imm_u = imm;
    inst.push_back(std::make_unique<Arith2Inst>(
        arm::OpCode::Mov, translate_var_reg(i.dest.id), imm_u & 0xffff));
    if (imm_u > 0xffff) {
      inst.push_back(std::make_unique<Arith2Inst>(
          arm::OpCode::MovT, translate_var_reg(i.dest.id), imm_u & 0xffff0000));
    }
  } else {
    inst.push_back(std::make_unique<Arith2Inst>(
        arm::OpCode::Mov, translate_var_reg(i.dest.id),
        translate_value_to_operand2(i.src)));
  }
}

void Codegen::translate_inst(mir::inst::PhiInst& i) {
  // noop
}

void Codegen::translate_inst(mir::inst::CallInst& i) {
  // TODO: make call stuff
  throw new prelude::NotImplementedException();
}

void Codegen::translate_inst(mir::inst::StoreInst& i) {
  auto ins = std::make_unique<LoadStoreInst>(arm::OpCode::StR,
                                             translate_value_to_reg(i.val),
                                             translate_var_reg(i.dest.id));
  inst.push_back(std::move(ins));
}

void Codegen::translate_inst(mir::inst::LoadInst& i) {
  auto ins = std::make_unique<LoadStoreInst>(arm::OpCode::LdR,
                                             translate_value_to_reg(i.val),
                                             translate_var_reg(i.dest.id));
  inst.push_back(std::move(ins));
}

void Codegen::translate_inst(mir::inst::RefInst& i) {
  // TODO: Should we just check whether the variable is stack variable?
  throw new prelude::NotImplementedException();
}

void Codegen::translate_inst(mir::inst::PtrOffsetInst& i) {
  auto ins = std::make_unique<Arith3Inst>(
      arm::OpCode::Add, translate_var_reg(i.dest.id),
      translate_var_reg(i.ptr.id), translate_value_to_operand2(i.offset));
  inst.push_back(std::move(ins));
}

void Codegen::translate_inst(mir::inst::OpInst& i) {
  bool reverse_params = i.lhs.is_immediate() && !i.rhs.is_immediate() &&
                        (i.op != mir::inst::Op::Div);

  mir::inst::Value& lhs = reverse_params ? i.lhs : i.rhs;
  mir::inst::Value& rhs = reverse_params ? i.rhs : i.lhs;

  switch (i.op) {
    case mir::inst::Op::Add:
      inst.push_back(std::make_unique<Arith3Inst>(
          arm::OpCode::Add, translate_var_reg(i.dest.id),
          translate_value_to_reg(lhs), translate_value_to_operand2(rhs)));
      break;

    case mir::inst::Op::Sub:
      if (reverse_params) {
        inst.push_back(std::make_unique<Arith3Inst>(
            arm::OpCode::Rsb, translate_var_reg(i.dest.id),
            translate_value_to_reg(lhs), translate_value_to_operand2(lhs)));
      } else {
        inst.push_back(std::make_unique<Arith3Inst>(
            arm::OpCode::Sub, translate_var_reg(i.dest.id),
            translate_value_to_reg(rhs), translate_value_to_operand2(rhs)));
      }
      break;

    case mir::inst::Op::Mul:
      inst.push_back(std::make_unique<Arith3Inst>(
          arm::OpCode::Mul, translate_var_reg(i.dest.id),
          translate_value_to_reg(lhs), translate_value_to_operand2(rhs)));
      break;

    case mir::inst::Op::Div:
      inst.push_back(std::make_unique<Arith3Inst>(
          arm::OpCode::SDiv, translate_var_reg(i.dest.id),
          translate_value_to_reg(lhs), translate_value_to_operand2(rhs)));
      break;

    case mir::inst::Op::And:
      inst.push_back(std::make_unique<Arith3Inst>(
          arm::OpCode::And, translate_var_reg(i.dest.id),
          translate_value_to_reg(lhs), translate_value_to_operand2(rhs)));
      break;

    case mir::inst::Op::Or:
      inst.push_back(std::make_unique<Arith3Inst>(
          arm::OpCode::Orr, translate_var_reg(i.dest.id),
          translate_value_to_reg(lhs), translate_value_to_operand2(rhs)));
      break;

    case mir::inst::Op::Rem:
      // WARN: Mod is a pseudo-instruction!
      // Mod operations needs to be eliminated in later passes.
      inst.push_back(std::make_unique<Arith3Inst>(
          arm::OpCode::_Mod, translate_var_reg(i.dest.id),
          translate_value_to_reg(lhs), translate_value_to_operand2(rhs)));

    case mir::inst::Op::Gt:
      emit_compare(i.dest.id, i.lhs, i.rhs, ConditionCode::Gt, reverse_params);
      break;
    case mir::inst::Op::Lt:
      emit_compare(i.dest.id, i.lhs, i.rhs, ConditionCode::Lt, reverse_params);
      break;
    case mir::inst::Op::Gte:
      emit_compare(i.dest.id, i.lhs, i.rhs, ConditionCode::Ge, reverse_params);
      break;
    case mir::inst::Op::Lte:
      emit_compare(i.dest.id, i.lhs, i.rhs, ConditionCode::Le, reverse_params);
      break;
    case mir::inst::Op::Eq:
      emit_compare(i.dest.id, i.lhs, i.rhs, ConditionCode::Equal,
                   reverse_params);
      break;
    case mir::inst::Op::Neq:
      emit_compare(i.dest.id, i.lhs, i.rhs, ConditionCode::NotEqual,
                   reverse_params);
      break;
      break;
  }
}

void Codegen::emit_compare(mir::inst::VarId& dest, mir::inst::Value& lhs,
                           mir::inst::Value& rhs, arm::ConditionCode cond,
                           bool reversed) {
  auto lhsv = translate_value_to_reg(lhs);
  auto rhsv = translate_value_to_operand2(rhs);

  inst.push_back(std::make_unique<Arith2Inst>(
      reversed ? OpCode::Cmn : OpCode::Cmp, translate_value_to_reg(lhs),
      translate_value_to_operand2(rhs)));

  inst.push_back(std::make_unique<Arith2Inst>(
      OpCode::Mov, translate_var_reg(dest), Operand2(0)));
  inst.push_back(std::make_unique<Arith2Inst>(
      OpCode::Mov, translate_var_reg(dest), Operand2(1), cond));
}

void Codegen::translate_branch(mir::inst::JumpInstruction& j) {
  switch (j.kind) {
    case mir::inst::JumpInstructionKind::Br:
      inst.push_back(std::make_unique<BrInst>(
          OpCode::B, format_bb_label(func.name, j.bb_true)));
      break;
    case mir::inst::JumpInstructionKind::BrCond: {
      auto back = inst.end() - 2;

      // Find if a comparison exists
      std::optional<ConditionCode> cond = std::nullopt;
      if (inst.size() >= 2) {
        auto b1 = back->get();
        auto b2 = (back + 1)->get();
        if (b1->op == arm::OpCode::Mov && b2->op == arm::OpCode::Mov) {
          auto b1m = dynamic_cast<Arith2Inst*>(b1);
          auto b2m = dynamic_cast<Arith2Inst*>(b2);
          if (b1m->r1 == b2m->r1 && b1m->r2 == 1 && b2m->r2 == 0 &&
              b1m->cond == arm::ConditionCode::Always &&
              b2m->cond != arm::ConditionCode::Always) {
            cond = {b2m->cond};
          }
        }
      }
      if (cond) {
        inst.push_back(std::make_unique<BrInst>(
            OpCode::B, format_bb_label(func.name, j.bb_false),
            inverse_cond(cond.value())));
        inst.push_back(std::make_unique<BrInst>(
            OpCode::B, format_bb_label(func.name, j.bb_false)));
      } else {
        inst.push_back(std::make_unique<Arith2Inst>(
            OpCode::Cmp, translate_var_reg(j.cond_or_ret.value()),
            Operand2(0)));
        inst.push_back(std::make_unique<BrInst>(
            OpCode::B, format_bb_label(func.name, j.bb_false),
            ConditionCode::NotEqual));
        inst.push_back(std::make_unique<BrInst>(
            OpCode::B, format_bb_label(func.name, j.bb_false)));
      }
    } break;
    case mir::inst::JumpInstructionKind::Return:
      // TODO: Clean up before return
      inst.push_back(std::make_unique<PureInst>(OpCode::Bx));
      break;
    case mir::inst::JumpInstructionKind::Undefined:
      // WARN: Error about undefined blocks
      throw new std::exception();
      break;
    case mir::inst::JumpInstructionKind::Unreachable:
      // TODO: Discard unreachable blocks beforhand
      break;
  }
}

}  // namespace backend::codegen
