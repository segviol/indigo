#pragma once
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include "../prelude/prelude.hpp"

namespace mir::types {

const int INT_SIZE = 4;
const int PTR_SIZE = 4;

class Ty;

typedef std::shared_ptr<Ty> SharedTyPtr;
typedef int LabelId;

enum class TyKind { Int, Void, Array, Ptr, RestParam };

/// Base class for types
class Ty : public prelude::Displayable {
 public:
  virtual TyKind kind() const = 0;
  virtual bool is_value_type() const = 0;
  virtual int size() const = 0;
  virtual void display(std::ostream& o) const = 0;
  virtual ~Ty(){};
};

/// Int type. `i32` or `int32` in some languages.
class IntTy final : public Ty {
 public:
  IntTy() {}

  virtual TyKind kind() const { return TyKind::Int; }
  virtual bool is_value_type() const { return true; }
  virtual int size() const { return INT_SIZE; };
  virtual void display(std::ostream& o) const;
  virtual ~IntTy() {}
};

/// Void or unit type.
class VoidTy final : public Ty {
 public:
  VoidTy() {}

  virtual TyKind kind() const { return TyKind::Void; }
  virtual bool is_value_type() const { return true; }
  virtual int size() const { return 0; };
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
  virtual int size() const { return item->size() * len; };
  virtual void display(std::ostream& o) const;
  virtual ~ArrayTy() {}
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
  virtual int size() const { return PTR_SIZE; };
  virtual void display(std::ostream& o) const;
  virtual ~PtrTy() {}

 private:
  void reduce_array();
};

/// Rest param for variadic function. This type only appears in standard library
/// `putf(char*, ...rest)` function.
class RestParamTy final : public Ty {
 public:
  RestParamTy() {}

  virtual TyKind kind() const { return TyKind::RestParam; }
  virtual bool is_value_type() const { return true; }
  virtual int size() const { return 0; };
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

void display_op(std::ostream& o, Op val);

// TODO: how are variable and values typed?
class Variable : public prelude::Displayable {
  virtual void display(std::ostream& o) const;
};

class Value : public prelude::Displayable {
  virtual void display(std::ostream& o) const;
};

class JumpInstruction;
class Inst;

// TODO: Constructors for all these types
/// Base class for instruction
class Inst : public prelude::Displayable {
 public:
  /// Instruction destination variable
  Variable dest;

  virtual InstKind inst_kind() = 0;
  virtual void display(std::ostream& o) const = 0;
  virtual ~Inst() {}
};

/// Assign instruction. `$dest = $src`
class AssignInst final : public Inst {
 public:
  Value src;

  virtual InstKind inst_kind() { return InstKind::Assign; }
  virtual void display(std::ostream& o) const;
  virtual ~AssignInst() {}
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
};

/// Call instruction. `$dest = call $func(...$params)`
class CallInst final : public Inst {
 public:
  Variable func;
  std::vector<Value> params;

  virtual InstKind inst_kind() { return InstKind::Call; }
  virtual void display(std::ostream& o) const;
  virtual ~CallInst() {}
};

/// Reference instruction. `$dest = &$val`
class RefInst final : public Inst {
 public:
  Variable val;

  virtual InstKind inst_kind() { return InstKind::Ref; }
  virtual void display(std::ostream& o) const;
  virtual ~RefInst() {}
};

/// Dereference instruction. `$dest = load $val`
class LoadInst final : public Inst {
 public:
  Value val;

  virtual InstKind inst_kind() { return InstKind::Load; }
  virtual void display(std::ostream& o) const;
  virtual ~LoadInst() {}
};

/// Store instruction. `store $val to $dest`
class StoreInst final : public Inst {
 public:
  Value val;

  virtual InstKind inst_kind() { return InstKind::Store; }
  virtual void display(std::ostream& o) const;
  virtual ~StoreInst() {}
};

/// Offset ptr by offset. `$dest = $ptr + $offset`
class PtrOffsetInst final : public Inst {
 public:
  Variable ptr;
  Value offset;

  virtual InstKind inst_kind() { return InstKind::PtrOffset; }
  virtual void display(std::ostream& o) const;
  virtual ~PtrOffsetInst() {}
};

/// Phi instruction. `$dest = phi(...$vars)`
class PhiInst final : public Inst {
 public:
  std::vector<Variable> vars;

  virtual InstKind inst_kind() { return InstKind::Phi; }
  virtual void display(std::ostream& o) const;
  virtual ~PhiInst() {}
};

class JumpInstruction final : public prelude::Displayable {
 public:
  JumpInstruction(JumpInstructionKind kind, int bb_true = -1, int bb_false = -1,
                  std::optional<Variable> cond_ = {},
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
};

class MirFunction : public prelude::Displayable {
  // TODO: function definition?

  std::map<mir::types::LabelId, BasicBlk> basic_blks;

  virtual void display(std::ostream& o) const {}
};

class MirPackage : public prelude::Displayable {
  // TODO: package definition?

  virtual void display(std::ostream& o) const {}
};

}  // namespace mir::inst
