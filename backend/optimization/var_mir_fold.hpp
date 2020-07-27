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
      for (auto instIter = blksIter->second.inst.begin();
           instIter != blksIter->second.inst.end(); instIter++) {
        switch (instIter->get()->inst_kind()) {
          case mir::inst::InstKind::Op: {
            std::shared_ptr<mir::inst::OpInst> opInst =
                std::make_shared<mir::inst::OpInst>(*((mir::inst::OpInst*)instIter->get()));
            switch (opInst->op) {
              case mir::inst::Op::Add: {
                if (opInst->lhs.is_immediate() &&
                    *opInst->lhs.get_if<int32_t>() == 0) {
                  blksIter->second.inst.insert(
                      instIter + 1, std::make_unique<mir::inst::AssignInst>(
                                        opInst->dest, opInst->rhs));
                  blksIter->second.inst.erase(instIter);
                } else if (opInst->rhs.is_immediate() &&
                           *opInst->rhs.get_if<int32_t>() == 0) {
                  blksIter->second.inst.insert(
                      instIter + 1, std::make_unique<mir::inst::AssignInst>(
                                        opInst->dest, opInst->lhs));
                  blksIter->second.inst.erase(instIter);
                }
                break;
              }
              case mir::inst::Op::Sub: {
                if (opInst->rhs.is_immediate() &&
                    *opInst->rhs.get_if<int32_t>() == 0) {
                  blksIter->second.inst.insert(
                      instIter + 1, std::make_unique<mir::inst::AssignInst>(
                                        opInst->dest, opInst->lhs));
                  blksIter->second.inst.erase(instIter);
                }
                break;
              }
              case mir::inst::Op::Mul: {
                if (opInst->lhs.is_immediate() &&
                        *opInst->lhs.get_if<int32_t>() == 0 ||
                    opInst->rhs.is_immediate() &&
                        *opInst->rhs.get_if<int32_t>() == 0) {
                  blksIter->second.inst.insert(
                      instIter + 1, std::make_unique<mir::inst::AssignInst>(
                                        opInst->dest, mir::inst::Value(0)));
                  blksIter->second.inst.erase(instIter);
                } else if (opInst->lhs.is_immediate() &&
                           *opInst->lhs.get_if<int32_t>() == 1) {
                  blksIter->second.inst.insert(
                      instIter + 1, std::make_unique<mir::inst::AssignInst>(
                                        opInst->dest, opInst->rhs));
                  blksIter->second.inst.erase(instIter);
                } else if (opInst->rhs.is_immediate() &&
                           *opInst->rhs.get_if<int32_t>() == 1) {
                  blksIter->second.inst.insert(
                      instIter + 1, std::make_unique<mir::inst::AssignInst>(
                                        opInst->dest, opInst->lhs));
                  blksIter->second.inst.erase(instIter);
                }
                break;
              }
              case mir::inst::Op::Div: {
                if (opInst->lhs.is_immediate() &&
                    *opInst->lhs.get_if<int32_t>() == 0) {
                  blksIter->second.inst.insert(
                      instIter + 1, std::make_unique<mir::inst::AssignInst>(
                                        opInst->dest, mir::inst::Value(0)));
                  blksIter->second.inst.erase(instIter);
                } else if (opInst->rhs.is_immediate() &&
                           *opInst->rhs.get_if<int32_t>() == 1) {
                  blksIter->second.inst.insert(
                      instIter + 1, std::make_unique<mir::inst::AssignInst>(
                                        opInst->dest, opInst->lhs));
                  blksIter->second.inst.erase(instIter);
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
};

}  // namespace optimization::var_mir_fold