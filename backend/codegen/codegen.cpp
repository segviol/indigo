#include "codegen.hpp"

#include <cassert>
#include <exception>
#include <memory>
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
      consts.insert({format_label(func.name, const_counter++), int_value});
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
  inst.push_back(
      std::make_unique<Arith2Inst>(arm::OpCode::Mov, translate_var_reg(0),
                                   translate_value_to_operand2(i.src)));
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
      throw new prelude::NotImplementedException();

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

}  // namespace backend::codegen
