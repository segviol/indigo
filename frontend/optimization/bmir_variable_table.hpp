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

class VarSingle {
 public:
  bool changed;
  bool isConst;
  int32_t initValue;

  VarSingle(bool isConst) : changed(false), isConst(isConst) {}
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

  void insertVarSingle(std::string name,
                       std::shared_ptr<VarSingle>& varSingle) {
    name2VarSingle[name] = varSingle;
  }

  void insertVarSingle(std::string name,
                       std::shared_ptr<VarSingle>&& varSingle) {
    name2VarSingle[name] = varSingle;
  }

  std::shared_ptr<VarSingle>& getVarSingle(std::string name) {
    return name2VarSingle.at(name);
  }

  bool hasVarSingle(std::string name) { return name2VarSingle.count(name) > 0; }

 private:
  std::map<std::string, std::shared_ptr<VarArray>> name2VarArray;
  std::map<std::string, std::shared_ptr<VarSingle>> name2VarSingle;
};
}  // namespace front::optimization::bmir_variable_table