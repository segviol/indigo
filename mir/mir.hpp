#pragma once

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <variant>
#include <vector>

#include "../prelude/prelude.hpp"

namespace mir::types {

const int INT_SIZE = 4;
const int PTR_SIZE = 4;

class Ty;

typedef std::shared_ptr<Ty> SharedTyPtr;
typedef int LabelId;

enum class TyKind { Int, Void, Array, Ptr, Fn, RestParam, Vector };

/// Base class for types
class Ty : public prelude::Displayable {
 public:
  virtual TyKind kind() const = 0;
  virtual bool is_value_type() const = 0;
  virtual std::optional<int> size() const = 0;
  virtual void display(std::ostream& o) const = 0;
  virtual ~Ty(){};
};

/// Int type. `i32` or `int32` in some languages.
class IntTy final : public Ty {
 public:
  IntTy() {}

  virtual TyKind kind() const { return TyKind::Int; }
  virtual bool is_value_type() const { return true; }
  virtual std::optional<int> size() const { return INT_SIZE; };
  virtual void display(std::ostream& o) const;
  virtual ~IntTy() {}
};

/// Void or unit type.
class VoidTy final : public Ty {
 public:
  VoidTy() {}

  virtual TyKind kind() const { return TyKind::Void; }
  virtual bool is_value_type() const { return true; }
  virtual std::optional<int> size() const { return 0; };
  virtual void display(std::ostream& o) const;
  virtual ~VoidTy() {}
};

/// Array type. `item[len]`
class ArrayTy final : public Ty {
 public:
  ArrayTy(SharedTyPtr item, int len) : item(item), len(len) {}

  SharedTyPtr item;
  int len;

  virtual TyKind kind() const { return TyKind::Array; }
  virtual bool is_value_type() const { return true; }
  virtual std::optional<int> size() const {
    auto size = item->size();
    if (size.has_value())
      return (*size) * len;
    else
      return std::nullopt;
  };
  virtual void display(std::ostream& o) const;
  virtual ~ArrayTy() {}
};

/// Array type. `item[len]`
class VectorTy final : public Ty {
 public:
  VectorTy(SharedTyPtr item, int len) : item(item), len(len) {
    if (item->kind() != TyKind::Int) {
      // TODO: Throw exception
    }
  }

  SharedTyPtr item;
  int len;

  virtual TyKind kind() const { return TyKind::Vector; }
  virtual bool is_value_type() const { return true; }
  virtual std::optional<int> size() const {
    auto size = item->size();
    if (size.has_value())
      return (*size) * len;
    else
      return std::nullopt;
  };
  virtual void display(std::ostream& o) const;
  virtual ~VectorTy() {}
};

/// Pointer type. `item*`
///
/// When using `Array<T, n>` to construct a Ptr, instead of `Ptr<Array<T, n>>`
/// it wil automagically reduce to `Ptr<T>`.
class PtrTy final : public Ty {
 public:
  PtrTy(SharedTyPtr item) : item(item) {
    // reduce "to" type to array item if it's an array
    reduce_array();
  }

  SharedTyPtr item;
  virtual TyKind kind() const { return TyKind::Ptr; }
  virtual bool is_value_type() const { return false; }
  virtual std::optional<int> size() const { return PTR_SIZE; };
  virtual void display(std::ostream& o) const;
  virtual ~PtrTy() {}

 private:
  void reduce_array();
};

/// Represents a function.
class FunctionTy final : public Ty {
 public:
  FunctionTy(SharedTyPtr return_val, std::vector<SharedTyPtr> params,
             bool is_extern = false)
      : ret(return_val), params(params), is_extern(is_extern) {}

  SharedTyPtr ret;
  std::vector<SharedTyPtr> params;

  bool is_extern;

  virtual TyKind kind() const { return TyKind::Fn; }
  virtual bool is_value_type() const { return false; }
  virtual std::optional<int> size() const { return std::nullopt; };
  virtual void display(std::ostream& o) const;
  virtual ~FunctionTy() {}
};

/// Rest param for variadic function. This type only appears in standard library
/// `putf(char*, ...rest)` function.
class RestParamTy final : public Ty {
 public:
  RestParamTy() {}

  virtual TyKind kind() const { return TyKind::RestParam; }
  virtual bool is_value_type() const { return true; }
  virtual std::optional<int> size() const { return std::nullopt; };
  virtual void display(std::ostream& o) const;
  virtual ~RestParamTy() {}
};

/// Create a new IntTy behind a shared pointer
std::shared_ptr<IntTy> new_int_ty();

/// Create a new VoidTy behind shared pointer
std::shared_ptr<VoidTy> new_void_ty();

/// Create a new ArrayTy behind shared pointer
std::shared_ptr<ArrayTy> new_array_ty(SharedTyPtr item, int len);

/// Create a new PtrTy behind shared pointer
std::shared_ptr<PtrTy> new_ptr_ty(SharedTyPtr item);

}  // namespace mir::types

namespace mir::inst {

enum class InstKind { Assign, Op, Ref, Load, Store, PtrOffset, Call, Phi };

enum class Op { Add, Sub, Mul, Div, Rem, Gt, Lt, Gte, Lte, Eq, Neq, And, Or };

enum class ValueKind { Imm, Void, Variable };

enum class JumpInstructionKind { Undefined, Return, BrCond, Br, Unreachable };

enum class JumpKind { Undefined, Branch, Loop };

enum class VarScope { Local, Global };

void display_op(std::ostream& o, Op val);

class GlobalValue
    : public std::variant<uint32_t, std::vector<uint32_t>, std::string>,
      public prelude::Displayable {
 public:
  virtual void display(std::ostream& o) const;
};

struct VarId : public prelude::Displayable {
  VarId(uint32_t localVarName) : localName(localVarName), globalName() {}
  VarId(types::LabelId globalVarName)
      : globalName(globalVarName), localName() {}

  void display(std::ostream& o) const;

  std::optional<uint32_t> localName;
  std::optional<types::LabelId> globalName;

  VarScope scope() {
    if (localName.has_value())
      return VarScope::Local;
    else
      return VarScope::Global;
  }

  std::optional<uint32_t> get_local_id() { return localName; }

  std::optional<types::LabelId> get_global_id() {
    if (globalName.has_value())
      return {globalName.value()};
    else
      return {};
  }

  bool operator<(const VarId& other) const {
    if (this->localName.has_value()) {
      if (other.localName.has_value()) {
        return localName.value() < other.localName.value();
      } else {
        return true;
      }
    } else {
      if (other.localName.has_value()) {
        return false;
      } else {
        return globalName.value() < other.globalName.value();
      }
    }
  }
};  // namespace mir::inst

class Variable : public prelude::Displayable {
 public:
  VarId id;
  types::SharedTyPtr ty;

  virtual void display(std::ostream& o) const;
};

class Value : public std::variant<int32_t, VarId>, public prelude::Displayable {
 public:
  virtual void display(std::ostream& o) const;

  template <typename T>
  T* get_if() {
    return std::get_if<T>(this);
  }

  bool is_immediate() { return std::holds_alternative<int32_t>(*this); }
};

class JumpInstruction;
class Inst;

// TODO: Constructors for all these types
/// Base class for instruction
class Inst : public prelude::Displayable {
 public:
  Inst(Variable& dest) : dest(dest) {}
  /// Instruction destination variable
  Variable& dest;

  virtual InstKind inst_kind() = 0;
  virtual void display(std::ostream& o) const = 0;
  virtual std::set<VarId> useVars() const = 0;
  virtual ~Inst() {}
};

/// Assign instruction. `$dest = $src`
class AssignInst final : public Inst {
 public:
  Value src;

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
};

/// Operator instruction. `$dest = $lhs op $rhs`
class OpInst final : public Inst {
 public:
  Value lhs;
  Value rhs;
  Op op;

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
};

/// Call instruction. `$dest = call $func(...$params)`
class CallInst final : public Inst {
 public:
  VarId func;
  std::vector<Value> params;

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
};

/// Reference instruction. `$dest = &$val`
class RefInst final : public Inst {
 public:
  VarId val;

  virtual InstKind inst_kind() { return InstKind::Ref; }
  virtual void display(std::ostream& o) const;
  virtual ~RefInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    s.insert(val);
    return s;
  }
};

/// Dereference instruction. `$dest = load $val`
class LoadInst final : public Inst {
 public:
  Value val;

  virtual InstKind inst_kind() { return InstKind::Load; }
  virtual void display(std::ostream& o) const;
  virtual ~LoadInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    if (val.index() == 1) {
      s.insert(std::get<VarId>(val));
    }
    return s;
  }
};

/// Store instruction. `store $val to $dest`
class StoreInst final : public Inst {
 public:
  Value val;

  virtual InstKind inst_kind() { return InstKind::Store; }
  virtual void display(std::ostream& o) const;
  virtual ~StoreInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    if (val.index() == 1) {
      s.insert(std::get<VarId>(val));
    }
    return s;
  }
};

/// Offset ptr by offset. `$dest = $ptr + $offset`
class PtrOffsetInst final : public Inst {
 public:
  Variable ptr;
  Value offset;

  virtual InstKind inst_kind() { return InstKind::PtrOffset; }
  virtual void display(std::ostream& o) const;
  virtual ~PtrOffsetInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    s.insert(ptr.id);
    if (offset.index() == 1) {
      s.insert(std::get<VarId>(offset));
    }
    return s;
  }
};

/// Phi instruction. `$dest = phi(...$vars)`
class PhiInst final : public Inst {
 public:
  std::vector<Variable> vars;

  virtual InstKind inst_kind() { return InstKind::Phi; }
  virtual void display(std::ostream& o) const;
  virtual ~PhiInst() {}
  std::set<VarId> useVars() const {
    auto s = std::set<VarId>();
    for (auto var : vars) {
      s.insert(var.id);
    }
    return s;
  }
};

class JumpInstruction final : public prelude::Displayable {
 public:
  JumpInstruction(JumpInstructionKind kind, int bb_true = -1, int bb_false = -1,
                  std::optional<Variable> cond_or_ret = {},
                  JumpKind jump_kind = JumpKind::Undefined)
      : cond_or_ret(cond_or_ret),
        kind(kind),
        bb_true(bb_true),
        bb_false(bb_false),
        jump_kind(jump_kind) {}

  JumpInstructionKind kind;
  std::optional<Variable> cond_or_ret;
  int bb_true;
  int bb_false;
  JumpKind jump_kind;
  std::set<VarId> useVars() {
    std::set<VarId> s;
    if (cond_or_ret.has_value()) {
      s.insert(cond_or_ret->id);
    }
    return s;
  }
  virtual void display(std::ostream& o) const;
  virtual ~JumpInstruction() {}
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

  BasicBlk(const BasicBlk& other) = delete;
};

class MirFunction : public prelude::Displayable {
 public:
  std::string name;
  std::shared_ptr<types::FunctionTy> type;
  std::map<uint32_t, Variable> variables;
  std::map<mir::types::LabelId, BasicBlk> basic_blks;

  virtual void display(std::ostream& o) const;

  MirFunction(const MirFunction& other) = delete;
};

class MirPackage : public prelude::Displayable {
 public:
  std::map<mir::types::LabelId, MirFunction> functions;
  std::map<mir::types::LabelId, GlobalValue> global_values;

  virtual void display(std::ostream& o) const;

  MirPackage(MirPackage& other) = delete;
  MirPackage(MirPackage&& other) = default;
};

}  // namespace mir::inst
