#include "codegen.hpp"

#include <cassert>
#include <exception>
#include <memory>
#include <typeinfo>

namespace backend::codegen {

using namespace arm;

arm::Function Codegen::translate_function() { return arm::Function(); }

void Codegen::translate_basic_block(mir::inst::BasicBlk& blk) {
  for (auto& inst : blk.inst) {
    auto& i = *inst;
    if (auto x = dynamic_cast<mir::inst::OpInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::RefInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::PhiInst*>(&i)) {
      translate_inst(*x);
    } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst*>(&i)) {
      translate_inst(*x);
    } else {
      throw new std::bad_cast();
    }
  }
}

arm::Reg Codegen::get_or_alloc_vgp(mir::inst::VarId v) {
  auto found = reg_map.find(v);
  if (found != reg_map.end()) {
    assert(arm::register_type(found->second) ==
           arm::RegisterKind::VirtualGeneralPurpose);
    return found->second;
  } else {
    return alloc_vgp();
  }
}

arm::Reg Codegen::get_or_alloc_vd(mir::inst::VarId v) {
  auto found = reg_map.find(v);
  if (found != reg_map.end()) {
    assert(arm::register_type(found->second) ==
           arm::RegisterKind::VirtualDoubleVector);
    return found->second;
  } else {
    return alloc_vd();
  }
}

arm::Reg Codegen::get_or_alloc_vq(mir::inst::VarId v) {
  auto found = reg_map.find(v);
  if (found != reg_map.end()) {
    assert(arm::register_type(found->second) ==
           arm::RegisterKind::VirtualQuadVector);
    return found->second;
  } else {
    return alloc_vq();
  }
}

arm::Reg Codegen::alloc_vgp() {
  return arm::make_register(arm::RegisterKind::VirtualGeneralPurpose,
                            vreg_gp_counter++);
}

arm::Reg Codegen::alloc_vd() {
  return arm::make_register(arm::RegisterKind::VirtualDoubleVector,
                            vreg_gp_counter++);
}

arm::Reg Codegen::alloc_vq() {
  return arm::make_register(arm::RegisterKind::VirtualQuadVector,
                            vreg_gp_counter++);
}

arm::Operand2 Codegen::translate_operand2(mir::inst::Value& v) {
  // TODO
  throw new std::exception();
}

arm::Reg Codegen::translate_operand2_alloc_reg(mir::inst::Value& v) {
  // TODO
  throw new std::exception();
}

arm::Reg Codegen::translate_var_reg(mir::inst::VarId v) {
  // TODO
  throw new std::exception();
}

void Codegen::translate_inst(mir::inst::AssignInst& i) {
  inst.push_back(
      std::make_unique<Arith2Inst>(arm::OpCode::Mov, translate_var_reg(0),
                                   translate_operand2_alloc_reg(i.src)));
}

void Codegen::translate_inst(mir::inst::PhiInst& i) {}

void Codegen::translate_inst(mir::inst::CallInst& i) {}

void Codegen::translate_inst(mir::inst::StoreInst& i) {}

void Codegen::translate_inst(mir::inst::LoadInst& i) {}

void Codegen::translate_inst(mir::inst::RefInst& i) {}

void Codegen::translate_inst(mir::inst::PtrOffsetInst& i) {}

void Codegen::translate_inst(mir::inst::OpInst& i) {}

}  // namespace backend::codegen
