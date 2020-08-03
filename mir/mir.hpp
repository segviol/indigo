#pragma once

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <variant>
#include <vector>

#include "../arm_code/arm.hpp"
#include "../prelude/prelude.hpp"
#include "ty.hpp"

namespace mir::inst {

enum class InstKind { Assign, Op, Ref, Load, Store, PtrOffset, Call, Phi };

enum class Op {
  Add,
  Sub,
  Mul,
  Div,
  MulSh,
  Rem,
  Gt,
  Lt,
  Gte,
  Lte,
  Eq,
  Neq,
  And,
  Or,
  Xor,
  Not,
  Shl,
  Shr,
  ShrA
};

enum class ValueKind { Imm, Void, Variable };

enum class JumpInstructionKind { Undefined, Return, BrCond, Br, Unreachable };

enum class JumpKind { Undefined, Branch, Loop };

enum class VarScope { Local, Global };

void display_op(std::ostream& o, Op val);

struct VarId : public prelude::Displayable {
  constexpr VarId() : id(-1) {}
  VarId(const VarId& other) = default;
  VarId(VarId&& other) = default;
  VarId(const uint32_t id) : id(id) {}
  uint32_t id;
#if PRETTIFY_MIR_VAR
  virtual void display(std::ostream& o) const {
    if (id < 65536)
      o << "$" << id;
    else
      o << "%" << (id - 65536);
  }
#else
  virtual void display(std::ostream& o) const { o << "$" << id; }
#endif
  bool operator==(const VarId& other) const { return id == other.id; };
  VarId& operator=(const VarId& other) = default;
  bool operator<(const VarId& other) const { return id < other.id; }
  bool operator<(const uint32_t& other) const { return id < other; }
  operator uint32_t() const { return id; }
};

// struct VarId : public prelude::Displayable {
//   VarId(uint32_t localVarName) : localName(localVarName), globalName() {}
//   VarId(types::LabelId globalVarName)
//       : globalName(globalVarName), localName() {}

//   void display(std::ostream& o) const;

//   std::optional<uint32_t> localName;
//   std::optional<types::LabelId> globalName;

//   VarScope scope() {
//     if (localName.has_value())
//       return VarScope::Local;
//     else
//       return VarScope::Global;
//   }

//   std::optional<uint32_t> get_local_id() { return localName; }

//   std::optional<types::LabelId> get_global_id() {
//     if (globalName.has_value())
//       return {globalName.value()};
//     else
//       return {};
//   }

//   bool operator<(const VarId& other) const {
//     if (this->localName.has_value()) {
//       if (other.localName.has_value()) {
//         return localName.value() < other.localName.value();
//       } else {
//         return true;
//       }
//     } else {
//       if (other.localName.has_value()) {
//         return false;
//       } else {
//         return globalName.value() < other.globalName.value();
//       }
//     }
//   }
// };

class Variable : public prelude::Displayable {
 public:
  Variable() {
    is_memory_var = false;
    is_temp_var = false;
    is_phi_var = false;
  }
  Variable(types::SharedTyPtr ty, bool is_memory_var = false,
           bool is_temp_var = false, bool is_phi_var = false)
      : ty(ty),
        is_memory_var(is_memory_var),
        is_temp_var(is_temp_var),
        is_phi_var(is_phi_var) {}
  types::SharedTyPtr ty;
  bool is_memory_var;
  bool is_temp_var;
  bool is_phi_var;
  int priority = 0;

  types::SharedTyPtr type() const {
    if (is_memory_var) {
      return types::new_ptr_ty(ty);
    } else {
      return ty;
    }
  }
  int size() { return ty->size().value(); }
  virtual void display(std::ostream& o) const;
};

class Value : public std::variant<int32_t, VarId>, public prelude::Displayable {
 public:
  Value(int32_t i) : std::variant<int32_t, VarId>(i) {}
  Value(VarId i) : std::variant<int32_t, VarId>(i) {}
  Value(VarId i, arm::RegisterShiftKind shift, uint8_t shift_amount)
      : std::variant<int32_t, VarId>(i),
        shift(shift),
        shift_amount(shift_amount) {}

  arm::RegisterShiftKind shift = arm::RegisterShiftKind::Lsl;
  uint8_t shift_amount = 0;

  virtual void display(std::ostream& o) const;

  template <typename T>
  T* get_if() {
    return std::get_if<T>(this);
  }
  bool has_shift() const { return !is_immediate() && shift_amount != 0; }
  bool is_immediate() const { return std::holds_alternative<int32_t>(*this); }
  template <typename T>
  void map_if_varid(T action) {
    if (auto varid = std::get_if<VarId>(this)) {
      action(*this, *varid);
    }
  }
};

class JumpInstruction;
class Inst;

// TODO: Constructors for all thholese types
/// Base class for instruction
class Inst : public prelude::Displayable {
 public:
  Inst(VarId dest) : dest(dest) {}
  /// Instruction destination variable
  VarId dest;

  virtual InstKind inst_kind() = 0;
  virtual void display(std::ostream& o) const = 0;
  virtual std::set<VarId> useVars() const = 0;
  virtual void replace(VarId from, VarId to) = 0;
  virtual ~Inst() {}
};

/// Assign instruction. `$dest = $src`
class AssignInst final : public Inst {
 public:
  Value src;
  AssignInst(VarId _dest, Value _src) : Inst(_dest), src(_src) {}
  virtual InstKind inst_kind() { return InstKind::Assign; }
  virtual void display(std::ostream& o) const;
  virtual ~AssignInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    if (src.index() == 1) {
      s.insert(std::get<VarId>(src));
    }
    return s;
  }
  void replace(VarId from, VarId to) {
    if (src.index() == 1 && std::get<VarId>(src) == from) {
      src = to;
    }
  }
};

/// Operator instruction. `$dest = $lhs op $rhs`
class OpInst final : public Inst {
 public:
  Value lhs;
  Value rhs;
  Op op;

  OpInst(VarId _dest, Value _lhs, Value _rhs, Op _op)
      : Inst(_dest), lhs(_lhs), rhs(_rhs), op(_op) {}
  virtual InstKind inst_kind() { return InstKind::Op; }
  virtual void display(std::ostream& o) const;
  virtual ~OpInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    if (lhs.index() == 1) {
      s.insert(std::get<VarId>(lhs));
    }
    if (rhs.index() == 1) {
      s.insert(std::get<VarId>(rhs));
    }
    return s;
  }
  void replace(VarId from, VarId to) {
    if (lhs.index() == 1 && std::get<VarId>(lhs) == from) {
      lhs = to;
    }

    if (rhs.index() == 1 && std::get<VarId>(rhs) == from) {
      rhs = to;
    }
  }
};

/// Call instruction. `$dest = call $func(...$params)`
class CallInst final : public Inst {
 public:
  std::string func;
  std::vector<Value> params;

  CallInst(VarId _dest, std::string _func, std::vector<Value> _params)
      : Inst(_dest), func(_func), params(_params) {}
  virtual InstKind inst_kind() { return InstKind::Call; }
  virtual void display(std::ostream& o) const;
  virtual ~CallInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    for (auto value : params) {
      if (value.index() == 1) {
        s.insert(std::get<VarId>(value));
      }
    }
    return s;
  }
  void replace(VarId from, VarId to) {
    for (auto& para : params) {
      if (para.index() == 1 && std::get<VarId>(para) == from) {
        para = to;
      }
    }
  }
};

/// Reference instruction. `$dest = &$val`
class RefInst final : public Inst {
 public:
  std::variant<VarId, std::string> val;

  RefInst(VarId _dest, std::variant<VarId, std::string> _val)
      : Inst(_dest), val(_val) {}
  virtual InstKind inst_kind() { return InstKind::Ref; }
  virtual void display(std::ostream& o) const;
  virtual ~RefInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    if (auto val = std::get_if<VarId>(&this->val)) s.insert(*val);
    return s;
  }
  void replace(VarId from, VarId to) {}
};

/// Dereference instruction. `$dest = load $val`
class LoadInst final : public Inst {
 public:
  LoadInst(Value src, VarId dest) : src(src), Inst(dest) {}
  Value src;

  virtual InstKind inst_kind() { return InstKind::Load; }
  virtual void display(std::ostream& o) const;
  virtual ~LoadInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    if (src.index() == 1) {
      s.insert(std::get<VarId>(src));
    }
    return s;
  }
  void replace(VarId from, VarId to) {
    if (src.index() == 1 && std::get<VarId>(src) == from) {
      src = to;
    }
  }
};

/// Dereference instruction. `$dest = load $val`
class LoadOffsetInst final : public Inst {
 public:
  LoadOffsetInst(Value src, VarId dest, Value offset = 0)
      : src(src), Inst(dest), offset(offset) {}
  Value src;
  Value offset;
  virtual InstKind inst_kind() { return InstKind::Load; }
  virtual void display(std::ostream& o) const;
  virtual ~LoadOffsetInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    if (src.index() == 1) {
      s.insert(std::get<VarId>(src));
    }
    if (offset.index() == 1) {
      s.insert(std::get<VarId>(offset));
    }
    return s;
  }
  void replace(VarId from, VarId to) {
    if (src.index() == 1 && std::get<VarId>(src) == from) {
      src = to;
    }
    if (offset.index() == 1 && std::get<VarId>(offset) == from) {
      offset = to;
    }
  }
};

/// Store instruction. `store $val to $dest`
class StoreInst final : public Inst {
 public:
  StoreInst(Value val, VarId dest) : val(val), Inst(dest) {}
  Value val;

  virtual InstKind inst_kind() { return InstKind::Store; }
  virtual void display(std::ostream& o) const;
  virtual ~StoreInst() {}
  std::set<VarId> useVars()
      const {  // for storeInst,dest is also use(not defined)
    auto s = std::set<VarId>();
    s.insert(dest);
    if (val.index() == 1) {
      s.insert(std::get<VarId>(val));
    }
    return s;
  }
  void replace(VarId from, VarId to) {
    if (val.index() == 1 && std::get<VarId>(val) == from) {
      val = to;
    }
    if (dest == from) {
      dest = to;
    }
  }
};

/// Store instruction. `store $val to $dest($offset)`
class StoreOffsetInst final : public Inst {
 public:
  StoreOffsetInst(Value val, VarId dest, Value offset = 0)
      : val(val), Inst(dest), offset(offset) {}
  Value val;
  Value offset;
  virtual InstKind inst_kind() { return InstKind::Store; }
  virtual void display(std::ostream& o) const;
  virtual ~StoreOffsetInst() {}
  std::set<VarId> useVars()
      const {  // for storeInst,dest is also use(not defined)
    auto s = std::set<VarId>();
    s.insert(dest);
    if (val.index() == 1) {
      s.insert(std::get<VarId>(val));
    }
    if (offset.index() == 1) {
      s.insert(std::get<VarId>(offset));
    }
    return s;
  }
  void replace(VarId from, VarId to) {
    if (val.index() == 1 && std::get<VarId>(val) == from) {
      val = to;
    }
    if (dest == from) {
      dest = to;
    }
    if (offset.index() == 1 && std::get<VarId>(offset) == from) {
      offset = to;
    }
  }
};

/// Offset ptr by offset. `$dest = $ptr + $offset`
class PtrOffsetInst final : public Inst {
 public:
  VarId ptr;
  Value offset;

  PtrOffsetInst(VarId _dest, VarId _ptr, Value _offset)
      : Inst(_dest), ptr(_ptr), offset(_offset) {}
  virtual InstKind inst_kind() { return InstKind::PtrOffset; }
  virtual void display(std::ostream& o) const;
  virtual ~PtrOffsetInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    s.insert(ptr);
    if (offset.index() == 1) {
      s.insert(std::get<VarId>(offset));
    }
    return s;
  }
  void replace(VarId from, VarId to) {
    if (ptr == from) {
      ptr = to;
    }
    if (offset.index() == 1 && std::get<VarId>(offset) == from) {
      offset = to;
    }
  }
};

/// Phi instruction. `$dest = phi(...$vars)`
class PhiInst final : public Inst {
 public:
  PhiInst(VarId _dest, std::vector<VarId> _vars)
      : Inst(_dest), vars(_vars), ori_var(_dest){};
  VarId ori_var;
  std::vector<VarId> vars;

  virtual InstKind inst_kind() { return InstKind::Phi; }
  virtual void display(std::ostream& o) const;
  virtual ~PhiInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    for (auto var : vars) {
      s.insert(var);
    }
    return s;
  }

  void replace(VarId from, VarId to) {}  // phi should not be replaced
};

class JumpInstruction final : public prelude::Displayable {
 public:
  JumpInstruction(JumpInstructionKind kind, int bb_true = -1, int bb_false = -1,
                  std::optional<VarId> cond_or_ret = {},
                  JumpKind jump_kind = JumpKind::Undefined)
      : cond_or_ret(cond_or_ret),
        kind(kind),
        bb_true(bb_true),
        bb_false(bb_false),
        jump_kind(jump_kind) {}

  JumpInstructionKind kind;
  std::optional<VarId> cond_or_ret;
  int bb_true;
  int bb_false;
  JumpKind jump_kind;
  std::set<VarId> useVars() {
    std::set<VarId> s;
    if (cond_or_ret.has_value()) {
      s.insert(cond_or_ret.value());
    }
    return s;
  }

  void replace(VarId from, VarId to) {
    if (cond_or_ret.has_value() && cond_or_ret.value() == from) {
      cond_or_ret = to;
    }
  }
  virtual void display(std::ostream& o) const;
  virtual ~JumpInstruction() {}
  JumpInstruction(const JumpInstruction&) = delete;
  JumpInstruction(JumpInstruction&&) = default;
  JumpInstruction& operator=(JumpInstruction&&) = default;
};

/// Represents a single basic block
class BasicBlk : public prelude::Displayable {
 public:
  BasicBlk(mir::types::LabelId id)
      : id(id), preceding(), inst(), jump(JumpInstructionKind::Undefined) {}

  mir::types::LabelId id;
  std::set<mir::types::LabelId> preceding;
  std::vector<std::unique_ptr<Inst>> inst;
  JumpInstruction jump;
  virtual void display(std::ostream& o) const;

  constexpr BasicBlk(const BasicBlk& other) = delete;
  BasicBlk(BasicBlk&&) = default;
};

class MirFunction : public prelude::Displayable {
 public:
  std::string name;
  std::shared_ptr<types::FunctionTy> type;
  std::map<uint32_t, Variable> variables;
  std::map<mir::types::LabelId, BasicBlk> basic_blks;

  // MirFunction() {}
  MirFunction(std::string _name, std::shared_ptr<types::FunctionTy> _type)
      : name(_name), type(_type), basic_blks(), variables() {}
  virtual void display(std::ostream& o) const;

  MirFunction(const MirFunction& other) = delete;
  MirFunction(MirFunction&& other) = default;
  MirFunction& operator=(MirFunction&& other) = default;
};

class MirPackage : public prelude::Displayable {
 public:
  std::map<std::string, MirFunction> functions;
  std::map<std::string, arm::ConstValue> global_values;

  MirPackage() {}
  virtual void display(std::ostream& o) const;

  MirPackage(const MirPackage& other) = delete;
  MirPackage(MirPackage&& other) = default;
};

}  // namespace mir::inst
namespace std {

template <>
struct hash<mir::inst::VarId> {
  std::size_t operator()(const mir::inst::VarId& i) const {
    return std::hash<uint32_t>()(i.id);
  }
};

}  // namespace std
