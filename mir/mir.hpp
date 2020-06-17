#pragma once
#include <memory>
#include <optional>
#include <vector>

using namespace std;

namespace mir::inst {

enum class InstKind { Assign, Op, Ref, Load, Store, PtrOffset, Call, Phi };

enum class Op { Add, Sub, Mul, Div, Gt, Lt, Gte, Lte, Eq, Neq, And, Or };

class Variable {};

class Value {};

/// Base class for instruction
class Inst {
 public:
  /// Instruction destination variable
  Variable dest;

  virtual InstKind inst_kind() = 0;
};

/// Assign instruction. `$dest = $src`
class AssignInst : public Inst {
 public:
  Value src;

  virtual InstKind inst_kind() { return InstKind::Assign; }
};

/// Operator instruction. `$dest = $lhs op $rhs`
class OpInst : public Inst {
 public:
  Value lhs;
  Value rhs;
  Op op;

  virtual InstKind inst_kind() { return InstKind::Op; }
};

/// Call instruction. `$dest = call $func(...$params)`
class CallInst : public Inst {
 public:
  Variable func;
  vector<Value> params;

  virtual InstKind inst_kind() { return InstKind::Call; }
};

/// Reference instruction. `$dest = &$val`
class RefInst : public Inst {
 public:
  Variable val;

  virtual InstKind inst_kind() { return InstKind::Ref; }
};

/// Dereference instruction. `$dest = *$val`
class LoadInst : public Inst {
 public:
  Value val;

  virtual InstKind inst_kind() { return InstKind::Load; }
};

class StoreInst : public Inst {
 public:
  Variable dest;
  Value val;

  virtual InstKind inst_kind() { return InstKind::Store; }
};

/// Phi instruction. `$dest = phi(...$vars)`
class PhiInst : public Inst {
 public:
  vector<Variable> vars;

  virtual InstKind inst_kind() { return InstKind::Phi; }
};

}  // namespace mir::inst

namespace mir::types {

const int INT_SIZE = 4;
const int PTR_SIZE = 4;

class Ty {
 public:
  virtual bool is_value_type() = 0;
  virtual int size() = 0;
};

class IntTy : public Ty {
 public:
  virtual bool is_value_type() { return true; }
  virtual int size() { return INT_SIZE; };
};

class VoidTy : public Ty {
 public:
  virtual bool is_value_type() { return true; }
  virtual int size() { return 0; };
};

class ArrayTy : public Ty {
 public:
  ArrayTy(unique_ptr<Ty> item, int len) {
    this->item = move(item);
    this->len = len;
  }

  unique_ptr<Ty> item;
  int len;

  virtual bool is_value_type() { return true; }
  virtual int size() { return item->size() * len; };
};

class PtrTy : public Ty {
 public:
  PtrTy(unique_ptr<Ty> to) { this->to = move(to); }

  unique_ptr<Ty> to;
  virtual bool is_value_type() { return false; }
  virtual int size() { return PTR_SIZE; };

 private:
  void reduce_ty();
};

}  // namespace mir::types
