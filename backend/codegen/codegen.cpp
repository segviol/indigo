#include "codegen.hpp"

#include <exception>

namespace backend::codegen {

arm::ArmCode Codegen::translate() { throw new std::exception(); }

arm::Function Codegen::translate_function(mir::inst::MirFunction &f) {
  //   return arm::Function();
  throw new std::exception();
}

}  // namespace backend::codegen
