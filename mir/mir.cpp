#include "mir.hpp"

#include <iostream>
#include <memory>
#include <typeinfo>
#include <vector>

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
    case Op::Rem:
      o << "%";
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

// void Variable::display(std::ostream& o) const { o << id << ": " << *ty; }

// void VarId::display(std::ostream& o) const { o << "$" << id; }

void Value::display(std::ostream& o) const {
  if (auto x = std::get_if<VarId>(this)) {
    o << *x;
  } else if (auto x = std::get_if<int32_t>(this)) {
    o << *x;
  }
}

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

void RefInst::display(std::ostream& o) const {
  o << dest << " = &";
  if (auto x = std::get_if<VarId>(&val)) {
    o << *x;
  } else if (auto x = std::get_if<types::LabelId>(&val)) {
    o << "@" << *x;
  }
}

void LoadInst::display(std::ostream& o) const {
  o << dest << " = load " << src;
}

void StoreInst::display(std::ostream& o) const {
  o << "store " << val << " to "
    << "dest";
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
      o << "undefined_jump!";
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

    case JumpInstructionKind::Unreachable:
      o << "unreachable!";
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

  for (auto i = inst.begin(); i != inst.end(); i++) {
    o << '\t' << **i << std::endl;
  }

  o << jump << std::endl;
}

void MirFunction::display(std::ostream& o) const {
  auto& return_ty = type->ret;
  auto& params = type->params;
  o << "fn " << name << "(";
  for (auto i = params.begin(); i != params.end(); i++) {
    if (i != params.begin()) o << ", ";
    o << *i;
  }
  o << ") ->" << return_ty << "{" << std::endl;
  for (auto i = basic_blks.begin(); i != basic_blks.end(); i++) {
    o << i->second;
  }
  o << "}" << std::endl;
}

void MirPackage::display(std::ostream& o) const {}

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

void FunctionTy::display(std::ostream& o) const {
  o << "Fn(";
  for (auto i = params.begin(); i != params.end(); i++) {
    if (i != params.begin()) o << ", ";
    o << *i;
  }
  o << ") -> " << *ret;
}

void RestParamTy::display(std::ostream& o) const { o << "...Rest"; }

std::shared_ptr<IntTy> new_int_ty() { return std::make_shared<IntTy>(); }

std::shared_ptr<VoidTy> new_void_ty() { return std::make_shared<VoidTy>(); }

std::shared_ptr<ArrayTy> new_array_ty(SharedTyPtr item, int len) {
  return std::make_shared<ArrayTy>(item, len);
}

std::shared_ptr<PtrTy> new_ptr_ty(SharedTyPtr item) {
  return std::make_shared<PtrTy>(item);
}

std::shared_ptr<FunctionTy> new_function_ty(SharedTyPtr ret,
                                            std::vector<SharedTyPtr> params,
                                            bool is_extern = false) {
  return std::make_shared<FunctionTy>(ret, params, is_extern);
}

}  // namespace mir::types
