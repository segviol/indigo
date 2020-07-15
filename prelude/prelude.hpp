#pragma once

#include <climits>
#include <exception>
#include <iostream>
#include <sstream>

#include "../spdlog/fmt/bundled/core.h"

namespace prelude {

static inline uint32_t rotl32(uint32_t n, unsigned int c) {
  const unsigned int mask =
      (CHAR_BIT * sizeof(n));  // assumes width is a power of 2.

  // assert ( (c<=mask) &&"rotate by type width or more");
  c &= mask;
  return (n << c) | (n >> (mask - c));
}

static inline uint32_t rotr32(uint32_t n, unsigned int c) {
  const unsigned int mask = (CHAR_BIT * sizeof(n));

  // assert ( (c<=mask) &&"rotate by type width or more");
  c &= mask;
  return (n >> c) | (n << (mask - c));
}

class Displayable {
 public:
  /// Output a text representation of this class using provided output stream
  /// `o`
  virtual void display(std::ostream& o) const = 0;
  friend std::ostream& operator<<(std::ostream& o, const Displayable& val) {
    val.display(o);
    return o;
  }
};

class UnreachableException : public std::exception {};
class NotImplementedException : public std::exception {};

}  // namespace prelude
