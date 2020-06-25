#include "mir.hpp"

#include <iostream>
#include <memory>
#include <typeinfo>

namespace mir::inst {

void display_op(std::ostream& o, Op val) {
  switch (val) {
    case Op::Add:
      o << "+";
      break;
    case Op::Sub:
      o << "-";
      break;
    case Op::Mul:
      o << "*";
      break;
    case Op::Div:
      o << "/";
      break;
    case Op::Gt:
      o << ">";
      break;
    case Op::Lt:
      o << "<";
      break;
    case Op::Gte:
      o << ">=";
      break;
    case Op::Lte:
      o << "<=";
      break;
    case Op::Eq:
      o << "==";
      break;
    case Op::Neq:
      o << "!=";
      break;
    case Op::And:
      o << "&&";
      break;
    case Op::Or:
      o << "||";
      break;
  }
}

void Variable::display(std::ostream& o) const { o << "TODO:VARIABLE"; }
void Value::display(std::ostream& o) const { o << "TODO:VALUE"; }

void AssignInst::display(std::ostream& o) const { o << dest << " = " << src; }

void OpInst::display(std::ostream& o) const {
  o << dest << " = " << lhs << " ";
  display_op(o, op);
  o << " " << rhs;
}

void CallInst::display(std::ostream& o) const {
  o << dest << " = " << func << "(";
  for (int i = 0; i < params.size(); i++) {
    if (i != 0) o << ", ";
    o << params[i];
  }
  o << ")";
}

void RefInst::display(std::ostream& o) const { o << dest << " = &" << val; }

void LoadInst::display(std::ostream& o) const {
  o << dest << " = load " << val;
}

void StoreInst::display(std::ostream& o) const {
  o << dest << " = store " << val;
}

void PtrOffsetInst::display(std::ostream& o) const {
  o << dest << " = offset " << ptr << " by " << offset;
}

void PhiInst::display(std::ostream& o) const {
  o << dest << " = phi [";
  for (int i = 0; i < vars.size(); i++) {
    if (i != 0) o << ", ";
    o << vars[i];
  }
  o << "]";
}

void JumpInstruction::display(std::ostream& o) const {
  switch (kind) {
    case JumpInstructionKind::Undefined:
      o << "UNDEFINED_JUMP!";
      break;

    case JumpInstructionKind::Br:
      o << "br " << bb_true;
      break;

    case JumpInstructionKind::BrCond:
      o << "br " << cond_or_ret.value() << ", " << bb_true << ", " << bb_false;
      if (jump_kind == JumpKind::Branch)
        o << " if_branch";
      else if (jump_kind == JumpKind::Loop)
        o << " loop";

      break;

    case JumpInstructionKind::Return:
      if (cond_or_ret.has_value())
        o << "ret " << cond_or_ret.value();
      else
        o << "ret void";
      break;

    default:
      break;
  }
}

void BasicBlk::display(std::ostream& o) const {
  o << "bb" << id << ":" << std::endl;

  // print preceding
  o << "// preceding: ";
  for (auto i = preceding.begin(); i != preceding.end(); i++) {
    if (i != preceding.begin()) o << ", ";
    o << *i;
  }
  o << std::endl;

  for (auto& i : inst) {
    o << *i << std::endl;
  }

  o << jump << std::endl;
}

}  // namespace mir::inst

namespace mir::types {

void PtrTy::reduce_array() {
  if (this->item->kind() == TyKind::Array) {
    auto arr = std::dynamic_pointer_cast<ArrayTy>(this->item);
    this->item = arr->item;
  }
  // else noop
}

void IntTy::display(std::ostream& o) const { o << "i32"; }

void VoidTy::display(std::ostream& o) const { o << "void"; }

void PtrTy::display(std::ostream& o) const {
  item->display(o);
  o << "*";
}

void ArrayTy::display(std::ostream& o) const {
  o << "[";
  item->display(o);
  o << " x " << len << "]";
}

void RestParamTy::display(std::ostream& o) const { o << "..."; }

std::shared_ptr<IntTy> new_int_ty() { return std::make_shared<IntTy>(); }

std::shared_ptr<VoidTy> new_void_ty() { return std::make_shared<VoidTy>(); }

std::shared_ptr<ArrayTy> new_array_ty(SharedTyPtr item, int len) {
  return std::make_shared<ArrayTy>(item, len);
}

std::shared_ptr<PtrTy> new_ptr_ty(SharedTyPtr item) {
  return std::make_shared<PtrTy>(item);
}

}  // namespace mir::types
