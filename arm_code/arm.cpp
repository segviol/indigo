#include "arm.hpp"

#include <cmath>
#include <cstdint>

#include "../prelude/prelude.hpp"

namespace arm {

bool is_valid_immediate(uint32_t val) {
  if (val <= 0xff)
    return true;
  else if (val <= 0x00ffffff) {
    int highest_bit = log2(val & -val) + 1;
    return (val & ~(0xff << highest_bit)) == 0;
  } else {
    val = prelude::rotl32(val, 8);
    int highest_bit = log2(val & -val) + 1;
    return (val & ~(0xff << highest_bit)) == 0;
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
    default:
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

void PureInst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
}

void Arith2Inst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  // TODO
}

void Arith3Inst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  // TODO
}

void BrInst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  // TODO
}

void LoadStoreInst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  // TODO
}

void MultLoadStoreInst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  // TODO
}

void PushPopInst::display(std::ostream &o) const {
  display_op(op, o);
  display_cond(cond, o);
  // TODO
}

void LabelInst::display(std::ostream &o) const { o << label << ":"; }

}  // namespace arm
