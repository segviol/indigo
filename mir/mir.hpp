#pragma once
#include <memory>
#include <optional>
#include <vector>

namespace mir::inst {

enum class InstKind { Assign, Op, Ref, Load, Store, PtrOffset, Call, Phi };

enum class Op { Add, Sub, Mul, Div, Gt, Lt, Gte, Lte, Eq, Neq, And, Or };

enum class ValueKind { Imm, Void, Variable };

void display_op(std::ostream& o, Op val);

// TODO: how are variable and values typed?
class Variable {};

class Value {};

// TODO: Constructors for all these types
/// Base class for instruction
class Inst {
 public:
  /// Instruction destination variable
  Variable dest;

  virtual InstKind inst_kind() = 0;
  virtual ~Inst();
};

/// Assign instruction. `$dest = $src`
class AssignInst final : public Inst {
 public:
  Value src;

  virtual InstKind inst_kind() { return InstKind::Assign; }
};

/// Operator instruction. `$dest = $lhs op $rhs`
class OpInst final : public Inst {
 public:
  Value lhs;
  Value rhs;
  Op op;

  virtual InstKind inst_kind() { return InstKind::Op; }
};

/// Call instruction. `$dest = call $func(...$params)`
class CallInst final : public Inst {
 public:
  Variable func;
  std::vector<Value> params;

  virtual InstKind inst_kind() { return InstKind::Call; }
};

/// Reference instruction. `$dest = &$val`
class RefInst final : public Inst {
 public:
  Variable val;

  virtual InstKind inst_kind() { return InstKind::Ref; }
};

/// Dereference instruction. `$dest = load $val`
class LoadInst final : public Inst {
 public:
  Value val;

  virtual InstKind inst_kind() { return InstKind::Load; }
};

/// Store instruction. `store $val to $dest`
class StoreInst final : public Inst {
 public:
  Value val;

  virtual InstKind inst_kind() { return InstKind::Store; }
};

/// Offset ptr by offset. `$dest = $ptr + $offset`
class PtrOffetInst final : public Inst {
 public:
  Variable ptr;
  Value offset;

  virtual InstKind inst_kind() { return InstKind::PtrOffset; }
};

/// Phi instruction. `$dest = phi(...$vars)`
class PhiInst final : public Inst {
 public:
  std::vector<Variable> vars;

  virtual InstKind inst_kind() { return InstKind::Phi; }
};

}  // namespace mir::inst

namespace mir::types {

const int INT_SIZE = 4;
const int PTR_SIZE = 4;

typedef std::shared_ptr<Ty> SharedTyPtr;

enum class TyKind { Int, Void, Array, Ptr };

/// Base class for types
class Ty {
 public:
  virtual TyKind kind() const = 0;
  virtual bool is_value_type() const = 0;
  virtual int size() const = 0;
  virtual void display(std::ostream& o) const = 0;
  virtual ~Ty();
};

/// Int type. `i32` or `int32` in some languages.
class IntTy final : public Ty {
 public:
  virtual TyKind kind() const { return TyKind::Int; }
  virtual bool is_value_type() const { return true; }
  virtual int size() { return INT_SIZE; };
  virtual void display(std::ostream& o) const;
};

/// Void or unit type.
class VoidTy final : public Ty {
 public:
  virtual TyKind kind() { return TyKind::Void; }
  virtual bool is_value_type() { return true; }
  virtual int size() { return 0; };
  virtual void display(std::ostream& o) const;
};

/// Array type. `item[len]`
class ArrayTy final : public Ty {
 public:
  ArrayTy(SharedTyPtr item, int len) {
    this->item = std::move(item);
    this->len = len;
  }

  SharedTyPtr item;
  int len;

  virtual TyKind kind() { return TyKind::Array; }
  virtual bool is_value_type() { return true; }
  virtual int size() { return item->size() * len; };
  virtual void display(std::ostream& o) const;
};

/// Pointer type. `item*`
///
/// When using `Array<T, n>` to construct a Ptr, instead of `Ptr<Array<T, n>>`
/// it wil automagically reduce to `Ptr<T>`.
class PtrTy final : public Ty {
 public:
  PtrTy(SharedTyPtr item) {
    this->item = std::move(item);
    // reduce "to" type to array item if it's an array
    reduce_array();
  }

  SharedTyPtr item;
  virtual TyKind kind() const { return TyKind::Ptr; }
  virtual bool is_value_type() const { return false; }
  virtual int size() const { return PTR_SIZE; };
  virtual void display(std::ostream& o) const;

 private:
  void reduce_array();
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
