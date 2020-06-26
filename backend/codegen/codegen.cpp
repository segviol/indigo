#include "codegen.hpp"

#include <climits>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

namespace backend::codegen {

static inline uint32_t rotl32(uint32_t n, unsigned int c) {
  const unsigned int mask =
      (CHAR_BIT * sizeof(n) - 1);  // assumes width is a power of 2.

  // assert ( (c<=mask) &&"rotate by type width or more");
  c &= mask;
  return (n << c) | (n >> ((-c) & mask));
}

static inline uint32_t rotr32(uint32_t n, unsigned int c) {
  const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);

  // assert ( (c<=mask) &&"rotate by type width or more");
  c &= mask;
  return (n >> c) | (n << ((-c) & mask));
}

void Codegen::do_mir_optimization() {
  for (auto& pass : mir_passes) {
    pass->optimize_mir(package, extra_data);
  }
}

void Codegen::do_arm_optimization() {
  for (auto& pass : arm_passes) {
    pass->optimize_arm(arm_code.value(), extra_data);
  }
}

void Codegen::do_mir_to_arm_transform() {
  // TODO!
}

ArmCode Codegen::generate_code() {
  do_mir_optimization();
  do_mir_to_arm_transform();
  do_arm_optimization();
  return std::move(arm_code.value());
}

}  // namespace backend::codegen
