#pragma once
#include <vector>
using namespace std;

namespace mir::inst {

enum class InstKind { Assign, Op, Ref, Deref, PtrOffset, Call, Phi };

enum class Op { Add, Sub, Mul, Div, Gt, Lt, Gte, Lte, Eq, Neq, And, Or };

class Variable {};

class Value {};

/// Base class for instruction
class Inst {
 public:
  /// Instruction destination variable
  Variable dest;

  virtual InstKind inst_kind();
};

/// Assign instruction. `$dest = $src`
class AssignInst : Inst {
 public:
  Value src;

  virtual InstKind inst_kind() { return InstKind::Assign; }
};

/// Operator instruction. `$dest = $lhs op $rhs`
class OpInst : Inst {
 public:
  Value lhs;
  Value rhs;
  Op op;

  virtual InstKind inst_kind() { return InstKind::Op; }
};

/// Call instruction. `$dest = call $func(...$params)`
class CallInst : Inst {
 public:
  Variable func;
  vector<Value> params;

  virtual InstKind inst_kind() { return InstKind::Call; }
};

/// Reference instruction. `$dest = &$val`
class RefInst : Inst {
 public:
  Variable val;

  virtual InstKind inst_kind() { return InstKind::Ref; }
};

/// Dereference instruction. `$dest = *$val`
class DerefInst : Inst {
 public:
  Value val;

  virtual InstKind inst_kind() { return InstKind::Deref; }
};

/// Phi instruction. `$dest = phi(...$vars)`
class PhiInst : Inst {
 public:
  vector<Variable> vars;

  virtual InstKind inst_kind() { return InstKind::Phi; }
};

}  // namespace mir::inst
