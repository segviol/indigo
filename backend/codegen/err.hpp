#pragma once

#include <cstdint>
#include <exception>
#include <string>

namespace backend::codegen {
class FunctionNotFoundError {
 public:
  std::string fn_id;
};
}  // namespace backend::codegen
