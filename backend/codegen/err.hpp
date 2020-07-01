#pragma once

#include <cstdint>
#include <exception>

namespace backend::codegen {
class FunctionNotFoundError {
 public:
  uint32_t fn_id;
};
}  // namespace backend::codegen
