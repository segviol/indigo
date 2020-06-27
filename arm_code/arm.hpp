#pragma once

#include <bits/stdint-uintn.h>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "../prelude/prelude.hpp"

namespace arm {

bool is_valid_immediate(uint32_t val);

enum class RegisterShiftKind : uint8_t { Asr, Lsl, Lsr, Ror, Rrx };
enum class MemoryAccessKind : uint8_t { None, PreIndex, PostIndex };
enum class RegisterKind {
  GeneralPurpose,
  DoubleVector,
  QuadVector,
  VirtualGeneralPurpose,
  VirtualDoubleVector,
  VirtualQuadVector
};

// Register type. Any value larger or equal to 64 is considered as a virtual
// register, and any value larger or equal to 2^31 is considered as a virtual
// vector register.
typedef uint32_t Reg;

const uint32_t REG_GP_START = 0;
const uint32_t REG_DOUBLE_START = 16;
const uint32_t REG_QUAD_START = 48;
const uint32_t REG_V_GP_START = 64;
const uint32_t REG_V_DOUBLE_START = 1 << 31;
const uint32_t REG_V_QUAD_START = 3 << 30;

inline bool is_virtual_register(Reg r);
RegisterKind register_type(Reg r);
uint32_t register_num(Reg r);
void display_reg_name(std::ostream& o, Reg r);

Reg make_register(RegisterKind ty, uint32_t num);

struct RegisterOperand {
  RegisterOperand() : RegisterOperand(0) {}

  RegisterOperand(Reg reg) : RegisterOperand(0, RegisterShiftKind::Lsl, 0) {}

  RegisterOperand(Reg reg, RegisterShiftKind shift, uint8_t shift_amount)
      : reg(reg), shift(shift), shift_amount(shift_amount) {}

  Reg reg;
  RegisterShiftKind shift;
  uint8_t shift_amount;
};

struct MemoryOperand {
  MemoryOperand(Reg r1, int16_t offset = 0,
                MemoryAccessKind kind = MemoryAccessKind::None)
      : MemoryOperand(r1, std::nullopt, false, offset, kind) {}

  MemoryOperand(Reg r1, Reg rm, MemoryAccessKind kind = MemoryAccessKind::None)
      : MemoryOperand(r1, rm, false, 0, kind) {}

  MemoryOperand(Reg r1, Reg rm, bool neg_rm, int16_t offset,
                MemoryAccessKind kind = MemoryAccessKind::None)
      : kind(kind), r1(r1), rm(rm), neg_rm(neg_rm), offset(offset) {}

  MemoryOperand(Reg r1, std::optional<Reg> rm, bool neg_rm, int16_t offset,
                MemoryAccessKind kind = MemoryAccessKind::None)
      : kind(kind), r1(r1), rm(rm), neg_rm(neg_rm), offset(offset) {}

  // Pre- or post-indexed memory access
  MemoryAccessKind kind;
  // Base register
  Reg r1;
  // Offset register
  std::optional<Reg> rm;
  // is offset negative?
  bool neg_rm;
  // another offset
  int16_t offset;
};

struct NoOperand {};

typedef std::variant<RegisterOperand, uint32_t> Operand2;
typedef std::string Label;

enum class OpCode {
  Nop,
  // Branches

  // Branch
  B,
  // Branch and link (call)
  Bl,
  // Branch exchange (return)
  Bx,
  // Compare and branch if zero
  Cbz,
  // Compare and branch if not zero
  Cbnz,
  // // If then
  // IT,

  // Arithmetic

  // Move
  Mov,
  // Add
  Add,
  // Subtract
  Sub,
  // Reverse subtract
  Rsb,
  // Multiply
  Mul,
  // Signed Divide
  SDiv,
  // And
  And,
  // Or
  Orr,
  // Xor
  Eor,
  // Bit clear
  Bic,
  // Arithmetic Shift Right
  Asr,
  // Compare
  Cmp,
  // Compare Minus
  Cmn,

  // Memory

  // Load Register
  LdR,
  // Load Multiple Register
  LdM,
  // Store Register
  StR,
  // Store Multiple Register
  StM,
  // Push
  Push,
  // Pop
  Pop,

  // Label
  _Label
};

void display_op(OpCode op, std::ostream& o);

enum class ConditionCode : uint8_t {
  Equal,
  NotEqual,
  CarrySet,
  UnsignedGe,
  CarryClear,
  UnsignedLt,
  MinusOrNegative,
  PositiveOrZero,
  Overflow,
  NoOverflow,
  UnsignedGt,
  UnsignedLe,
  Ge,
  Lt,
  Gt,
  Le,
  Always,
};

void display_cond(ConditionCode cond, std::ostream& o);
ConditionCode inverse_cond(ConditionCode cond);

struct Inst : public prelude::Displayable {
  OpCode op;
  ConditionCode cond;

  virtual void display(std::ostream& o) = 0;
  virtual ~Inst() {}
};

struct PureInst final : public Inst {
  virtual void display(std::ostream& o) const;
  virtual ~PureInst() {}
};

struct Arith3Inst final : public Inst {
  RegisterOperand rd;
  RegisterOperand r1;
  Operand2 r2;

  virtual void display(std::ostream& o) const;
  virtual ~Arith3Inst() {}
};

struct Arith2Inst final : public Inst {
  RegisterOperand r1;
  Operand2 r2;

  virtual void display(std::ostream& o) const;
  virtual ~Arith2Inst() {}
};

struct BrInst final : public Inst {
  Label l;

  virtual void display(std::ostream& o) const;
  virtual ~BrInst() {}
};

struct LoadStoreInst final : public Inst {
  Reg rd;
  MemoryOperand mem;

  virtual void display(std::ostream& o) const;
  virtual ~LoadStoreInst() {}
};

struct MultLoadStoreInst final : public Inst {
  std::vector<Reg> rd;
  MemoryOperand mem;

  virtual void display(std::ostream& o) const;
  virtual ~MultLoadStoreInst() {}
};

struct PushPopInst final : public Inst {
  std::vector<Reg> regs;

  virtual void display(std::ostream& o) const;
  virtual ~PushPopInst() {}
};

struct LabelInst final : public Inst {
  Label label;

  virtual void display(std::ostream& o) const;
  virtual ~LabelInst() {}
};

struct Function {
  std::string name;
  std::vector<std::unique_ptr<Inst>> inst;
  std::map<std::string, std::unique_ptr<char[]>> local_const;
};

struct ArmCode {
  std::vector<std::unique_ptr<Function>> functions;
  std::map<std::string, std::unique_ptr<char[]>> consts;
};

}  // namespace arm
