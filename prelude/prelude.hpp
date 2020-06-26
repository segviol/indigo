#pragma once
#include <iostream>

namespace prelude {
class Displayable {
 public:
  virtual void display(std::ostream& o) const = 0;
  friend std::ostream& operator<<(std::ostream& o, const Displayable& val) {
    val.display(o);
    return o;
  }
};
}  // namespace prelude
