#pragma once

#include <sys/types.h>

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
namespace RegManager {
using std::list;
using std::vector;
typedef u_int32_t Reg;
typedef u_int32_t VirtualReg;

const static vector<int> allRegs{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
const static vector<int> allGlobalRegs{};
const static vector<int> allLocalRegs{0, 1, 2, 3,  4,  5,  6,
                                      7, 8, 9, 10, 11, 12, 13};

class RegManager {
 public:
  vector<int> globalRegs;
  std::list<int> freeLocalRegs;
  std::list<int> useLocalRegs;  // lru
  std::map<Reg, VirtualReg> reg_var_map;
  std::map<VirtualReg, Reg> var_reg_map;

  Reg allocReg(VirtualReg var) {
    Reg reg;
    if (var_reg_map.count(var)) {
      reg = var_reg_map.at(var);
      useReg(reg);
      return reg;
    } else if (!freeLocalRegs.empty()) {
      reg = freeLocalRegs.front();
      freeLocalRegs.pop_front();
      reg_var_map[reg] = var;
      var_reg_map[var] = reg;
      useReg(reg);
      return reg;
    }
    reg = useLocalRegs.back();
    writeBack(reg);
    reg_var_map[reg] = var;
    var_reg_map[var] = reg;
    useReg(reg);
    return reg;
  }

  void useReg(Reg reg) {
    auto iter = std::find(useLocalRegs.begin(), useLocalRegs.end(), reg);
    if (iter != useLocalRegs.end()) {
      useLocalRegs.erase(iter);
      useLocalRegs.emplace_front(reg);
    } else {
      useLocalRegs.emplace_front(reg);
    }
  }

  void writeBack(Reg reg) {
    // To finish write back insts

    auto var = var_reg_map[reg];
    var_reg_map.erase(var);
    reg_var_map.erase(reg);
    auto iter = std::find(useLocalRegs.begin(), useLocalRegs.end(), reg);
    useLocalRegs.erase(iter);
    freeLocalRegs.push_front(reg);
  }
};
}  // namespace RegManager