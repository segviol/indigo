

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../arm_code/arm.hpp"
#include "../mir/mir.hpp"

#ifndef COMPILER_FRONT_CONST_PROP_H_
#define COMPILER_FRONT_CONST_PROP_H_

namespace front::const_prop {

class GlobalVar {
 public:
  std::string reg;
  int32_t value;
  bool useable;
  std::vector<mir::inst::VarId> treg;
  GlobalVar(std::string _reg);
};
}  // namespace front::const_prop

void const_propagation(mir::inst::MirPackage& p);

#endif