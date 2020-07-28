#pragma once

#include <string>

#include "../backend.hpp"

namespace optimization::var_mir_fold {
class VarMirFold final : public backend::MirOptimizePass {
 public:
  std::string name = "VarMirFold";

  std::string pass_name() const { return name; }

  void optimize_func(const std::string& name,
                     mir::inst::MirFunction& mirFunction) {
    for (auto blksIter = mirFunction.basic_blks.begin();
         blksIter != mirFunction.basic_blks.end(); blksIter++) {
      for (size_t i = 0; i < blksIter->second.inst.size(); i++) {
        mir::inst::Inst* inst =
            blksIter->second.inst.at(i).get();
        switch (inst->inst_kind()) {
          case mir::inst::InstKind::Op: {
            mir::inst::OpInst* opInst = (mir::inst::OpInst*) inst;
            switch (opInst->op) {
              case mir::inst::Op::Add: {
                if (isSpecifyImmediate(opInst->lhs, 0)) {
                  replaceInst(blksIter->second.inst, i,
                              std::make_unique<mir::inst::AssignInst>(
                                  opInst->dest, opInst->rhs));
                } else if (isSpecifyImmediate(opInst->rhs, 0)) {
                  replaceInst(blksIter->second.inst, i,
                              std::make_unique<mir::inst::AssignInst>(
                                  opInst->dest, opInst->lhs));
                }
                break;
              }
              case mir::inst::Op::Sub: {
                if (isSpecifyImmediate(opInst->rhs, 0)) {
                  replaceInst(blksIter->second.inst, i,
                              std::make_unique<mir::inst::AssignInst>(
                                  opInst->dest, opInst->lhs));
                }
                break;
              }
              case mir::inst::Op::Mul: {
                if (isSpecifyImmediate(opInst->lhs, 0) ||
                    isSpecifyImmediate(opInst->rhs, 0)) {
                  replaceInst(blksIter->second.inst, i,
                              std::make_unique<mir::inst::AssignInst>(
                                  opInst->dest, mir::inst::Value(0)));
                } else if (isSpecifyImmediate(opInst->lhs, 1)) {
                  replaceInst(blksIter->second.inst, i,
                              std::make_unique<mir::inst::AssignInst>(
                                  opInst->dest, opInst->rhs));
                } else if (isSpecifyImmediate(opInst->rhs, 1)) {
                  replaceInst(blksIter->second.inst, i,
                              std::make_unique<mir::inst::AssignInst>(
                                  opInst->dest, opInst->lhs));
                }
                break;
              }
              case mir::inst::Op::Div: {
                if (isSpecifyImmediate(opInst->lhs, 0)) {
                  replaceInst(blksIter->second.inst, i,
                              std::make_unique<mir::inst::AssignInst>(
                                  opInst->dest, mir::inst::Value(0)));
                } else if (isSpecifyImmediate(opInst->rhs, 1)) {
                  replaceInst(blksIter->second.inst, i,
                              std::make_unique<mir::inst::AssignInst>(
                                  opInst->dest, opInst->lhs));
                }
                break;
              }
              case mir::inst::Op::Rem: {
                break;
              }
              default:
                break;
            }
            break;
          }
          case mir::inst::InstKind::PtrOffset: {
            break;
          }
          default:
            break;
        }
      }
    }
  }

  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto& i : package.functions) {
      if (!i.second.type->is_extern) {
        optimize_func(i.first, i.second);
      }
    }
  }

 private:
  bool isSpecifyImmediate(mir::inst::Value& value, int32_t number) {
    return (value.is_immediate() && *(value.get_if<int32_t>()) == number);
  }

  void replaceInst(std::vector<std::unique_ptr<mir::inst::Inst>>& insts,
                   size_t index, std::unique_ptr<mir::inst::Inst>&& inst) {
    insts.insert(insts.begin() + index + 1, std::move(inst));
    insts.erase(insts.begin() + index);
  }
};

}  // namespace optimization::var_mir_fold