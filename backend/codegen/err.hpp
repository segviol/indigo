#pragma once

#include <cstdint>
#include <exception>
#include <string>

namespace backend::codegen {
class FunctionNotFoundError : public std::exception {
 public:
  FunctionNotFoundError(std::string fn_id) : std::exception() {
    what_ = "Function not found: " + fn_id;
  }
  std::string what_;
  virtual const char* what() const noexcept override { return what_.c_str(); }
};
}  // namespace backend::codegen
