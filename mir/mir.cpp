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
