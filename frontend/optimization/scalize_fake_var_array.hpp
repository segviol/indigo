#pragma once

#include "bmir_optimization.hpp"

namespace front::optimization::scalize_fake_var_array {
class ScalizeFakeVarArray final
    : public bmir_optimization::BmirOptimizationPass {
 public:
  std::string name = "scalize fake var array";
  virtual std::string pass_name() const { return name; }

  virtual void optimize_bmir(
      mir::inst::MirPackage& package,
      bmir_variable_table::BmirVariableTable& bmirVariableTable,
      std::map<std::string, std::vector<irGenerator::Instruction>>& funcInsts,
      std::map<std::string, std::any>& messages) {
    for (auto& func : funcInsts) {
      optimize_functiom(func.first, package, bmirVariableTable, func.second);
    }
  }

 private:
  void optimize_functiom(
      std::string funcName, mir::inst::MirPackage& package,
      bmir_variable_table::BmirVariableTable& bmirVariableTable,
      std::vector<irGenerator::Instruction>& insts) {
    std::map<uint32_t, std::pair<std::string, uint32_t>> tmpVarId2Offset;
    for (auto instructionIter = insts.begin(); instructionIter != insts.end();
         instructionIter++) {
      if (instructionIter->index() == 0) {
        std::shared_ptr<mir::inst::Inst> mirInst =
            std::get<std::shared_ptr<mir::inst::Inst>>(*instructionIter);
        switch (mirInst->inst_kind()) {
          case mir::inst::InstKind::Ref: {
            mir::inst::RefInst* refInst = (mir::inst::RefInst*)mirInst.get();
            if (std::holds_alternative<std::string>(refInst->val)) {
              std::string& varName = std::get<std::string>(refInst->val);
              if (bmirVariableTable.hasNameKey(varName) &&
                  !bmirVariableTable.getVarArray(varName)->changed) {
                tmpVarId2Offset[refInst->dest] = {varName, 0};
              }
            }
            break;
          }
          case mir::inst::InstKind::Load: {
            mir::inst::LoadInst* loadInst = (mir::inst::LoadInst*)mirInst.get();
            mir::inst::VarId& src = std::get<mir::inst::VarId>(loadInst->src);
            if (tmpVarId2Offset.count(src) > 0) {
              size_t index = instructionIter - insts.begin();
              insts.insert(
                  insts.begin() + index,
                  std::shared_ptr<mir::inst::AssignInst>(
                      new mir::inst::AssignInst(
                          loadInst->dest,
                          mir::inst::Value(
                              bmirVariableTable
                                  .getVarArray(tmpVarId2Offset[src].first)
                                  ->initValues.at(tmpVarId2Offset[src].second /
                                                  mir::types::INT_SIZE)))));
              insts.erase(insts.begin() + index + 1);
            }
            break;
          }
          case mir::inst::InstKind::Op: {
            mir::inst::OpInst* opInst = (mir::inst::OpInst*)mirInst.get();
            if (tmpVarId2Offset.count(opInst->dest) > 0) {
              if (std::holds_alternative<int32_t>(opInst->rhs)) {
                tmpVarId2Offset[opInst->dest].second +=
                    std::get<int32_t>(opInst->rhs);
              } else {
                tmpVarId2Offset.erase(opInst->dest);
              }
            }
            break;
          }
          default:
            break;
        }
      } else if (instructionIter->index() == 2) {
        tmpVarId2Offset.clear();
      }
    }
  }
};
}  // namespace front::optimization::scalize_fake_var_array