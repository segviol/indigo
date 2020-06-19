#pragma once
#include <memory>
#include <optional>
#include <vector>

namespace mir::inst {

enum class InstKind { Assign, Op, Ref, Load, Store, PtrOffset, Call, Phi };

enum class Op { Add, Sub, Mul, Div, Gt, Lt, Gte, Lte, Eq, Neq, And, Or };

// TODO: how are variable and values typed?
class Variable {};

class Value {};

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

/// Dereference instruction. `$dest = *$val`
class LoadInst final : public Inst {
 public:
  Value val;

  virtual InstKind inst_kind() { return InstKind::Load; }
};

class StoreInst final : public Inst {
 public:
  Variable dest;
  Value val;

  virtual InstKind inst_kind() { return InstKind::Store; }
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

enum class TyKind { Int, Void, Array, Ptr };

/// Base class for types
class Ty {
 public:
  virtual TyKind kind() const = 0;
  virtual bool is_value_type() const = 0;
  virtual int size() const = 0;
  virtual ~Ty();
};

/// Int type. `i32` or `int32` in some languages.
class IntTy final : public Ty {
 public:
  virtual TyKind kind() const { return TyKind::Int; }
  virtual bool is_value_type() const { return true; }
  virtual int size() { return INT_SIZE; };
};

/// Void or unit type.
class VoidTy final : public Ty {
 public:
  virtual TyKind kind() { return TyKind::Void; }
  virtual bool is_value_type() { return true; }
  virtual int size() { return 0; };
};

/// Array type. `item[len]`
class ArrayTy final : public Ty {
 public:
  ArrayTy(std::shared_ptr<Ty> item, int len) {
    this->item = std::move(item);
    this->len = len;
  }

  std::shared_ptr<Ty> item;
  int len;

  virtual TyKind kind() { return TyKind::Array; }
  virtual bool is_value_type() { return true; }
  virtual int size() { return item->size() * len; };
};

/// Pointer type. `item*`
///
/// When using `Array<T, n>` to construct a Ptr, instead of `Ptr<Array<T, n>>`
/// it wil automagically reduce to `Ptr<T>`.
class PtrTy final : public Ty {
 public:
  PtrTy(std::shared_ptr<Ty> item) {
    this->item = std::move(item);
    // reduce "to" type to array item if it's an array
    reduce_array();
  }

  std::shared_ptr<Ty> item;
  virtual TyKind kind() const { return TyKind::Ptr; }
  virtual bool is_value_type() const { return false; }
  virtual int size() const { return PTR_SIZE; };

 private:
  void reduce_array();
};

}  // namespace mir::types
