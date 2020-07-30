#pragma once

#include <string>

#include "../backend.hpp"

namespace optimization::algebraic_simplification {
class AlgebraicSimplification : public backend::MirOptimizePass {
 public:
  std::string name = "AlgebraicSimplification";

  std::string pass_name() const { return name; }

  void optimize_func(std::string name, mir::inst::MirFunction& mirFunction) {
    for (auto blksIter = mirFunction.basic_blks.begin();
         blksIter != mirFunction.basic_blks.end(); blksIter++) {
      for (auto instIter = blksIter->second.inst.begin();
           instIter != blksIter->second.inst.end(); instIter++) {
        switch (instIter->get()->inst_kind()) {
          case mir::inst::InstKind::Op: {
            mir::inst::OpInst* opPtr = (mir::inst::OpInst*)instIter->get();
            switch (opPtr->op) {
              case mir::inst::Op::Mul: {
                if (opPtr->lhs.is_immediate() && !opPtr->rhs.is_immediate()) {
                  mir::inst::Value tmp = opPtr->lhs;
                  opPtr->lhs = opPtr->rhs;
                  opPtr->rhs = tmp;
                }
                if (!opPtr->lhs.is_immediate() && opPtr->rhs.is_immediate()) {
                  int32_t num = *(opPtr->rhs.get_if<int32_t>());
                  if (num != 0 && (std::abs(num) & (std::abs(num) - 1)) == 0) {
                    uint32_t index;
                    uint32_t mul;

                    mul = std::abs(num);
                    for (index = 0; (mul & 1) == 0; mul >>= 1, index++)
                      ;
                    opPtr->op = mir::inst::Op::Shl;
                    opPtr->rhs = mir::inst::Value(index);

                    if (num < 0) {
                      blksIter->second.inst.insert(
                          instIter + 1, std::make_unique<mir::inst::OpInst>(
                                            opPtr->dest, mir::inst::Value(0),
                                            mir::inst::Value(opPtr->dest),
                                            mir::inst::Op::Sub));
                    }
                  }
                }
                break;
              }
              case mir::inst::Op::Div: {
                if (!opPtr->lhs.is_immediate() && opPtr->rhs.is_immediate()) {
                  int32_t num = *(opPtr->rhs.get_if<int32_t>());
                  if (num != 0 && (std::abs(num) & (std::abs(num) - 1)) == 0) {
                    uint32_t index;
                    uint32_t mul;

                    mul = std::abs(num);
                    for (index = 0; (mul & 1) == 0; mul >>= 1, index++)
                      ;
                    opPtr->op = mir::inst::Op::ShrA;
                    opPtr->rhs = mir::inst::Value(index);

                    if (num < 0) {
                      blksIter->second.inst.insert(
                          instIter + 1, std::make_unique<mir::inst::OpInst>(
                                            opPtr->dest, mir::inst::Value(0),
                                            mir::inst::Value(opPtr->dest),
                                            mir::inst::Op::Sub));
                    }
                  }
                }
                break;
              }
              case mir::inst::Op::Rem: {
                if (!opPtr->lhs.is_immediate() && opPtr->rhs.is_immediate()) {
                  int32_t num = *(opPtr->rhs.get_if<int32_t>());
                  if (num != 0 && (std::abs(num) & (std::abs(num) - 1)) == 0) {
                    uint32_t index;
                    uint32_t rem;

                    rem = std::abs(num);
                    for (index = 0; (rem & 1) == 0; rem >>= 1, index++)
                      ;
                    opPtr->op = mir::inst::Op::And;
                    opPtr->rhs = mir::inst::Value(((uint32_t)1 << index) - 1);
                    // TODO: deal the problem of negative lhs
                  }
                }
                break;
              }
              default:
                break;
            }
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

}  // namespace optimization::algebraic_simplification