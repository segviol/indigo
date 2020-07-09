#include "arm.hpp"

#include <cmath>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>
#include <variant>

#include "../prelude/prelude.hpp"

namespace arm {

RegisterKind register_type(Reg r) {
  if (r < 16)
    return RegisterKind::GeneralPurpose;
  else if (r < 48)
    return RegisterKind::DoubleVector;
  else if (r < 64)
    return RegisterKind::QuadVector;
  else if (r < 1 << 31)
    return RegisterKind::VirtualGeneralPurpose;
  else if (r < 3 << 30)
    return RegisterKind::VirtualDoubleVector;
  else
    return RegisterKind::VirtualQuadVector;
}

uint32_t register_num(Reg r) {
  if (r < 16)
    return r;
  else if (r < 48)
    return r - 16;
  else if (r < 64)
    return r - 48;
  else if (r < 1 << 31)
    return r - 64;
  else if (r < 3 << 30)
    return r - (1 << 31);
  else
    return r - (3 << 30);
}

void display_reg_name(std::ostream &o, Reg r) {
  if (r == REG_SP)
    o << "sp";
  else if (r == REG_LR)
    o << "lr";
  else if (r == REG_PC)
    o << "pc";
  else
    switch (register_type(r)) {
      case RegisterKind::GeneralPurpose:
        o << "r" << register_num(r);
        break;
      case RegisterKind::DoubleVector:
        o << "d" << register_num(r);
        break;
      case RegisterKind::QuadVector:
        o << "q" << register_num(r);
        break;
      case RegisterKind::VirtualGeneralPurpose:
        o << "v" << register_num(r);
        break;
      case RegisterKind::VirtualDoubleVector:
        o << "vd" << register_num(r);
        break;
      case RegisterKind::VirtualQuadVector:
        o << "vq" << register_num(r);
        break;
    }
}

void display_shift(std::ostream &o, RegisterShiftKind shift) {
  switch (shift) {
    case RegisterShiftKind::Lsl:
      o << "LSL";
      break;
    case RegisterShiftKind::Lsr:
      o << "LSR";
      break;
    case RegisterShiftKind::Asr:
      o << "ASR";
      break;
    case RegisterShiftKind::Ror:
      o << "ROR";
      break;
    case RegisterShiftKind::Rrx:
      o << "RRX";
      break;
  }
}

Reg make_register(RegisterKind k, uint32_t num) {
  switch (k) {
    case RegisterKind::GeneralPurpose:
      return num + REG_GP_START;
    case RegisterKind::DoubleVector:
      return num + REG_DOUBLE_START;
    case RegisterKind::QuadVector:
      return num + REG_QUAD_START;
    case RegisterKind::VirtualGeneralPurpose:
      return num + REG_V_GP_START;
    case RegisterKind::VirtualDoubleVector:
      return num + REG_V_DOUBLE_START;
    case RegisterKind::VirtualQuadVector:
      return num + REG_V_QUAD_START;
    default:
      return num;
  }
}

void RegisterOperand::display(std::ostream &o) const {
  display_reg_name(o, reg);

  if (shift != RegisterShiftKind::Lsl || shift_amount != 0) {
    display_shift(o, shift);
    if (shift != RegisterShiftKind::Rrx) {
      o << " #" << shift_amount;
    }
  }
}

bool is_valid_immediate(uint32_t val) {
  if (val <= 0xff)
    return true;
  else if (val <= 0x00ffffff) {
    int highest_bit = log2(val & -val) + 1;
    return (val & ~(0xff << highest_bit)) == 0 && highest_bit % 2 == 0;
  } else {
    val = prelude::rotl32(val, 8);
    int highest_bit = log2(val & -val) + 1;
    return (val & ~(0xff << highest_bit)) == 0 && highest_bit % 2 == 0;
  }
}

void Operand2::display(std::ostream &o) const {
  if (auto x = std::get_if<RegisterOperand>(this)) {
    x->display(o);
  } else if (auto x = std::get_if<int32_t>(this)) {
    o << "#" << *x;
  }
}

void display_op(OpCode op, std::ostream &o) {
  switch (op) {
    case OpCode::Nop:
      o << "nop";
      break;
    case OpCode::B:
      o << "b";
      break;
    case OpCode::Bl:
      o << "bl";
      break;
    case OpCode::Bx:
      o << "bx";
      break;
    case OpCode::Cbz:
      o << "cbz";
      break;
    case OpCode::Cbnz:
      o << "cbnz";
      break;
    case OpCode::Mov:
      o << "mov";
      break;
    case OpCode::MovT:
      o << "movt";
      break;
    case OpCode::Add:
      o << "add";
      break;
    case OpCode::Sub:
      o << "sub";
      break;
    case OpCode::Rsb:
      o << "rsb";
      break;
    case OpCode::Mul:
      o << "mul";
      break;
    case OpCode::SDiv:
      o << "sdiv";
      break;
    case OpCode::And:
      o << "and";
      break;
    case OpCode::Orr:
      o << "orr";
      break;
    case OpCode::Eor:
      o << "eor";
      break;
    case OpCode::Bic:
      o << "bic";
      break;
    case OpCode::Asr:
      o << "asr";
      break;
    case OpCode::Cmp:
      o << "cmp";
      break;
    case OpCode::Cmn:
      o << "cmn";
      break;
    case OpCode::LdR:
      o << "ldr";
      break;
    case OpCode::LdM:
      o << "ldm";
      break;
    case OpCode::StR:
      o << "str";
      break;
    case OpCode::StM:
      o << "stm";
      break;
    case OpCode::Push:
      o << "push";
      break;
    case OpCode::Pop:
      o << "pop";
      break;
    case OpCode::_Label:
      // Labels are pseudo-instructions
      break;
    case OpCode::_Mod:
      o << "_MOD";

      break;
    default:
      o << "?" << (int)op;
      break;
  }
}

void display_cond(ConditionCode cond, std::ostream &o) {
  switch (cond) {
    case ConditionCode::Equal:
      o << "eq";
      break;
    case ConditionCode::NotEqual:
      o << "ne";
      break;
    case ConditionCode::CarrySet:
      o << "cs";
      break;
    case ConditionCode::CarryClear:
      o << "cc";
      break;
    case ConditionCode::UnsignedGe:
      o << "hs";
      break;
    case ConditionCode::UnsignedLe:
      o << "ls";
      break;
    case ConditionCode::UnsignedGt:
      o << "hi";
      break;
    case ConditionCode::UnsignedLt:
      o << "lo";
      break;
    case ConditionCode::MinusOrNegative:
      o << "mn";
      break;
    case ConditionCode::PositiveOrZero:
      o << "pl";
      break;
    case ConditionCode::Overflow:
      o << "vs";
      break;
    case ConditionCode::NoOverflow:
      o << "vc";
      break;
    case ConditionCode::Ge:
      o << "ge";
      break;
    case ConditionCode::Lt:
      o << "lt";
      break;
    case ConditionCode::Gt:
      o << "gt";
      break;
    case ConditionCode::Le:
      o << "le";
      break;
    case ConditionCode::Always:
      // AL is default
      break;
  }
}

ConditionCode inverse_cond(ConditionCode cond) {
  switch (cond) {
    case ConditionCode::Equal:
      return ConditionCode::NotEqual;
    case ConditionCode::NotEqual:
      return ConditionCode::Equal;

    case ConditionCode::CarrySet:
      return ConditionCode::CarryClear;
    case ConditionCode::CarryClear:
      return ConditionCode::CarrySet;

    case ConditionCode::UnsignedGe:
      return ConditionCode::UnsignedLt;
    case ConditionCode::UnsignedLt:
      return ConditionCode::UnsignedGe;
    case ConditionCode::UnsignedGt:
      return ConditionCode::UnsignedLe;
    case ConditionCode::UnsignedLe:
      return ConditionCode::UnsignedGt;

    case ConditionCode::MinusOrNegative:
      return ConditionCode::PositiveOrZero;
    case ConditionCode::PositiveOrZero:
      return ConditionCode::MinusOrNegative;

    case ConditionCode::Overflow:
      return ConditionCode::NoOverflow;
    case ConditionCode::NoOverflow:
      return ConditionCode::Overflow;

    case ConditionCode::Ge:
      return ConditionCode::Lt;
    case ConditionCode::Lt:
      return ConditionCode::Ge;
    case ConditionCode::Gt:
      return ConditionCode::Le;
    case ConditionCode::Le:
      return ConditionCode::Ge;

    case ConditionCode::Always:
    default:
      return ConditionCode::Always;
  }
}

std::string format_bb_name(std::string_view func_name, uint32_t bb_id) {
  std::stringstream name;
  name << func_name << "_$bb" << bb_id;
  return name.str();
}

void ConstValue::display(std::ostream &o) const {
  if (auto x = std::get_if<uint32_t>(this)) {
    o << ".word " << *x;
  } else if (auto x = std::get_if<std::vector<uint32_t>>(this)) {
    o << ".word ";
    bool is_first = true;
    for (auto i : *x) {
      if (!is_first) {
        o << ", ";
      }
      o << i;
    }
  } else if (auto x = std::get_if<std::string>(this)) {
    o << ".asciz \"" << *x << "\"";
  }
}

void MemoryOperand::display(std::ostream &o) const {
  auto display_offset = [&](std::ostream &o) {
    if (auto x = std::get_if<RegisterOperand>(&offset)) {
      if (neg_rm) o << "-";
      o << *x;
    } else if (auto x = std::get_if<int16_t>(&offset)) {
      o << "#" << *x;
    }
  };
  o << "[";
  display_reg_name(o, r1);
  switch (kind) {
    case MemoryAccessKind::None:
      o << ", ";
      display_offset(o);
      o << "]";
      break;
    case MemoryAccessKind::PostIndex:
      o << ", ";
      display_offset(o);
      o << "]!";
      break;
    case MemoryAccessKind::PreIndex:
      o << "], ";
      display_offset(o);
      break;
  }
}

void PureInst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
}

void Arith2Inst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  o << " ";
  display_reg_name(o, r1);
  o << ", " << r2;
}

void Arith3Inst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  o << " ";
  display_reg_name(o, rd);
  o << ", ";
  display_reg_name(o, r1);
  o << ", " << r2;
}

void BrInst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  o << " " << l;
}

void LoadStoreInst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  o << " ";
  display_reg_name(o, rd);
  o << ", ";
  if (auto m = std::get_if<std::string>(&mem)) {
    o << *m;
  } else if (auto m = std::get_if<MemoryOperand>(&mem)) {
    o << *m;
  }
}

void MultLoadStoreInst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  o << " ";
  display_reg_name(o, rn);
  o << ", {";
  for (auto i = rd.begin(); i != rd.end(); i++) {
    if (i != rd.begin()) o << ", ";
    display_reg_name(o, *i);
  }
  o << "}";
}

void PushPopInst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  o << " {";
  for (auto i = regs.begin(); i != regs.end(); i++) {
    if (i != regs.begin()) o << ", ";
    display_reg_name(o, *i);
  }
  o << "}";
}

void LabelInst::display(std::ostream &o) const { o << label << ":"; }

void Function::display(std::ostream &o) const {
  o << name << ":" << std::endl;
  for (auto &i : inst) {
    if (!dynamic_cast<LabelInst *>(&*i)) {
      o << "\t";
    }
    o << *i << std::endl;
  }
}

void ArmCode::display(std::ostream &o) const {
  for (auto &f : functions) {
    o << *f << std::endl;
  }
}

}  // namespace arm
