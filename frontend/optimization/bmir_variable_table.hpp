#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace front::optimization::bmir_variable_table {
class VarArray {
 public:
  bool changed;
  bool isConst;
  std::vector<int32_t> initValues;

  VarArray(bool isConst) : changed(false), isConst(isConst) {}
};

class BmirVariableTable {
 public:
  void insertVarArray(std::string name, std::shared_ptr<VarArray>& varArray) {
    name2VarArray[name] = varArray;
  }

  void insertVarArray(std::string name, std::shared_ptr<VarArray>&& varArray) {
    name2VarArray[name] = varArray;
  }

  std::shared_ptr<VarArray>& getVarArray(std::string name) {
    return name2VarArray.at(name);
  }

  bool hasNameKey(std::string name) { return name2VarArray.count(name) > 0; }

 private:
  std::map<std::string, std::shared_ptr<VarArray>> name2VarArray;
};
}  // namespace front::optimization::bmir_variable_table