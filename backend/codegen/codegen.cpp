#include "codegen.hpp"

#include <exception>
#include <memory>

namespace backend::codegen {

arm::ArmCode Codegen::translate() { throw new std::exception(); }

arm::Function Codegen::translate_function(mir::inst::MirFunction& f) {
  return arm::Function();
}

void Codegen::translate_basic_block(mir::inst::BasicBlk& blk) {
  for (auto& inst : blk.inst) {
    auto& i = *inst;
    if (auto x = dynamic_cast<mir::inst::OpInst*>(&i)) {
    } else if (auto x = dynamic_cast<mir::inst::CallInst*>(&i)) {
    } else if (auto x = dynamic_cast<mir::inst::AssignInst*>(&i)) {
    } else if (auto x = dynamic_cast<mir::inst::LoadInst*>(&i)) {
    } else if (auto x = dynamic_cast<mir::inst::StoreInst*>(&i)) {
    } else if (auto x = dynamic_cast<mir::inst::RefInst*>(&i)) {
    } else if (auto x = dynamic_cast<mir::inst::PhiInst*>(&i)) {
    } else if (auto x = dynamic_cast<mir::inst::PtrOffsetInst*>(&i)) {
    }
  }
}

}  // namespace backend::codegen
