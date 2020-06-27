#pragma once
#include "../../arm_code/arm.hpp"
#include "../../prelude/prelude.hpp"
#include "../backend.hpp"

namespace backend::codegen {

class Codegen final {
 public:
  Codegen(mir::inst::MirPackage& pkg) : package(pkg) {}

  /// Translate MirPackage into ArmCode
  arm::ArmCode translate();

 private:
  mir::inst::MirPackage& package;
  std::vector<std::unique_ptr<arm::Inst>> inst;

  arm::Function translate_function(mir::inst::MirFunction& f);
  void translate_basic_block(mir::inst::BasicBlk& blk);
  void translate_inst(mir::inst::AssignInst& i);
  void translate_inst(mir::inst::PhiInst& i);
  void translate_inst(mir::inst::CallInst& i);
  void translate_inst(mir::inst::StoreInst& i);
  void translate_inst(mir::inst::LoadInst& i);
  void translate_inst(mir::inst::RefInst& i);
  void translate_inst(mir::inst::PtrOffsetInst& i);
  void translate_inst(mir::inst::OpInst& i);
};
}  // namespace backend::codegen
