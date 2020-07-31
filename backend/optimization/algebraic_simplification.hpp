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
      for (size_t i = 0; i < blksIter->second.inst.size(); i++) {
        mir::inst::Inst* instIter = blksIter->second.inst.at(i).get();
        switch (instIter->inst_kind()) {
          case mir::inst::InstKind::Op: {
            mir::inst::OpInst* opPtr = (mir::inst::OpInst*)instIter;
            switch (opPtr->op) {
              case mir::inst::Op::Mul: {
                if (opPtr->lhs.is_immediate() && !opPtr->rhs.is_immediate()) {
                  mir::inst::Value tmp = opPtr->lhs;
                  opPtr->lhs = opPtr->rhs;
                  opPtr->rhs = tmp;
                }
                if (!opPtr->lhs.is_immediate() && opPtr->rhs.is_immediate()) {
                  int32_t num = *(opPtr->rhs.get_if<int32_t>());
                  if (num != 0) {
                    if (num == -1) {
                      opPtr->op = mir::inst::Op::Sub;
                      opPtr->rhs = opPtr->lhs;
                      opPtr->lhs = mir::inst::Value(0);
                    } else if ((std::abs(num) & (std::abs(num) - 1)) == 0) {
                      uint32_t index;
                      uint32_t mul;

                      mul = std::abs(num);
                      for (index = 0; (mul & 1) == 0; mul >>= 1, index++)
                        ;
                      opPtr->op = mir::inst::Op::Shl;
                      opPtr->rhs = mir::inst::Value(index);

                      if (num < 0) {
                        insertInst(blksIter->second.inst, i + 1,
                                   std::make_unique<mir::inst::OpInst>(
                                       opPtr->dest, mir::inst::Value(0),
                                       mir::inst::Value(opPtr->dest),
                                       mir::inst::Op::Sub));
                      }
                    }
                  }
                }
                break;
              }
              case mir::inst::Op::Div: {
                if (!opPtr->lhs.is_immediate() && opPtr->rhs.is_immediate()) {
                  int32_t num = *(opPtr->rhs.get_if<int32_t>());
                  if (num != 0) {
                    if (num == -1) {
                      opPtr->op = mir::inst::Op::Sub;
                      opPtr->rhs = opPtr->lhs;
                      opPtr->lhs = mir::inst::Value(0);
                    } else if ((std::abs(num) & (std::abs(num) - 1)) == 0) {
                      uint32_t index;
                      uint32_t mul;

                      mul = std::abs(num);
                      for (index = 0; (mul & 1) == 0; mul >>= 1, index++)
                        ;
                      opPtr->op = mir::inst::Op::ShrA;
                      opPtr->rhs = mir::inst::Value(index);

                      if (num < 0) {
                        insertInst(blksIter->second.inst, i + 1,
                                   std::make_unique<mir::inst::OpInst>(
                                       opPtr->dest, mir::inst::Value(0),
                                       mir::inst::Value(opPtr->dest),
                                       mir::inst::Op::Sub));
                      }
                    } else {
                      uint32_t numAbs;
                      int32_t l;
                      uint64_t m;
                      int32_t m1;
                      int32_t dSign;
                      int32_t shPost;

                      numAbs = std::abs(num);
                      if ((l = upLog2(numAbs)) < 1) {
                        l = 1;
                      }
                      m = 1 + ((uint64_t)1 << (32 + l - 1)) / numAbs;
                      m1 = m - ((uint64_t)1 << 32);
                      dSign = (num >= 0 ? 0 : -1);
                      shPost = l - 1;

                      uint32_t tmp1;
                      uint32_t tmp2;
                      uint32_t tmp3;

                      tmp1 = getNewVar(mirFunction);
                      insertInst(
                          blksIter->second.inst, i + 1,
                          std::unique_ptr<mir::inst::Inst>(
                              new mir::inst::OpInst(
                                  mir::inst::VarId(tmp1), mir::inst::Value(m1),
                                  opPtr->lhs, mir::inst::Op::MulSh)));
                      tmp2 = getNewVar(mirFunction);
                      insertInst(
                          blksIter->second.inst, i + 2,
                          std::unique_ptr<mir::inst::Inst>(
                              new mir::inst::OpInst(
                                  mir::inst::VarId(tmp2),
                                  mir::inst::Value(mir::inst::VarId(tmp1)),
                                  opPtr->lhs, mir::inst::Op::Add)));
                      tmp1 = getNewVar(mirFunction);
                      insertInst(
                          blksIter->second.inst, i + 3,
                          std::unique_ptr<mir::inst::Inst>(
                              new mir::inst::OpInst(mir::inst::VarId(tmp1),
                                                    mir::inst::Value(tmp2),
                                                    mir::inst::Value(shPost),
                                                    mir::inst::Op::ShrA)));
                      tmp2 = getNewVar(mirFunction);
                      insertInst(
                          blksIter->second.inst, i + 4,
                          std::unique_ptr<mir::inst::Inst>(
                              new mir::inst::OpInst(
                                  mir::inst::VarId(tmp2), opPtr->lhs,
                                  mir::inst::Value(31), mir::inst::Op::ShrA)));
                      tmp3 = getNewVar(mirFunction);
                      insertInst(
                          blksIter->second.inst, i + 5,
                          std::unique_ptr<mir::inst::Inst>(
                              new mir::inst::OpInst(mir::inst::VarId(tmp3),
                                                    mir::inst::Value(tmp1),
                                                    mir::inst::Value(tmp2),
                                                    mir::inst::Op::Sub)));
                      tmp1 = getNewVar(mirFunction);
                      insertInst(
                          blksIter->second.inst, i + 6,
                          std::unique_ptr<mir::inst::Inst>(
                              new mir::inst::OpInst(mir::inst::VarId(tmp1),
                                                    mir::inst::Value(tmp3),
                                                    mir::inst::Value(dSign),
                                                    mir::inst::Op::Xor)));
                      insertInst(blksIter->second.inst, i + 7,
                                 std::unique_ptr<mir::inst::Inst>(
                                     new mir::inst::OpInst(
                                         opPtr->dest, mir::inst::Value(tmp1),
                                         mir::inst::Value(dSign),
                                         mir::inst::Op::Sub)));
                      blksIter->second.inst.erase(
                          blksIter->second.inst.begin() + i);
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
  // num != 0
  uint32_t upLog2(uint32_t num) {
    uint32_t leadZero;
    uint32_t r;

    for (leadZero = 0;
         leadZero <= 31 && (num & ((uint32_t)1 << (31 - leadZero))) == 0;
         leadZero++)
      ;
    if ((num & (num - 1)) == 0) {
      r = 31 - leadZero;
    } else {
      r = 32 - leadZero;
    }
    return r;
  }

  uint32_t getNewVar(mir::inst::MirFunction& func) {
    uint32_t maxId = 1;
    for (auto& var : func.variables) {
      if (var.first > maxId) {
        maxId = var.first;
      }
    }
    func.variables.insert(std::pair<uint32_t, mir::inst::Variable>(
        ++maxId, mir::inst::Variable(mir::types::SharedTyPtr(
                                         std::make_shared<mir::types::IntTy>()),
                                     false, true)));
    return maxId;
  }

  void insertInst(std::vector<std::unique_ptr<mir::inst::Inst>>& insts,
                  size_t i, std::unique_ptr<mir::inst::Inst>&& inst) {
    if (i < insts.size()) {
      insts.insert(insts.begin() + i, std::move(inst));
    } else {
      insts.push_back(std::move(inst));
    }
  }
};

}  // namespace optimization::algebraic_simplification