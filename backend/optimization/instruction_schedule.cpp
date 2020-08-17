#include "instruction_schedule.hpp"

namespace backend::instruction_schedule {

std::vector<ExePipeCode> allExePopeCodes = {
    ExePipeCode::Branch,   ExePipeCode::Integer0, ExePipeCode::Integer1,
    ExePipeCode::IntegerM, ExePipeCode::Load,     ExePipeCode::Store};

const std::string WrongInstExceptionMsg =
    "non-supported arm instrution for instruction schedule";

bool cmpNodeSharedPtr(const std::shared_ptr<DependencyDAGNode>& a,
                      const std::shared_ptr<DependencyDAGNode>& b) {
  return (*a < *b);
};

InstKind getInstKind(arm::Inst* inst) {
  switch (inst->op) {
    case arm::OpCode::B: {
      return InstKind::Branch;
      break;
    }
    case arm::OpCode::Bl: {
      return InstKind::Call;
      break;
    }
    case arm::OpCode::Mov:
    case arm::OpCode::MovT:
    case arm::OpCode::Mvn:
    case arm::OpCode::Lsl:
    case arm::OpCode::Lsr:
    case arm::OpCode::Asr: {
      return InstKind::Integer;
      break;
    }
    case arm::OpCode::Add:
    case arm::OpCode::Sub:
    case arm::OpCode::And:
    case arm::OpCode::Orr:
    case arm::OpCode::Eor:
    case arm::OpCode::Bic: {
      if (shiftByImmed(((arm::Arith3Inst*)inst)->r2)) {
        return InstKind::IntegerM;
      } else {
        return InstKind::Integer;
      }
      break;
    }
    case arm::OpCode::Mul:
    case arm::OpCode::SMMul: {
      return InstKind::IntegerM;
      break;
    }
    case arm::OpCode::Cmp:
    case arm::OpCode::Cmn: {
      if (shiftByImmed(((arm::Arith2Inst*)inst)->r2)) {
        return InstKind::IntegerM;
      } else {
        return InstKind::Integer;
      }
      break;
    }
    case arm::OpCode::LdR: {
      return InstKind::Load;
      break;
    }
    case arm::OpCode::StR: {
      return InstKind::Store;
      break;
    }
    default: {
      throw WrongInstExceptionMsg;
    }
  }
};

uint32_t getInstExeLatency(arm::Inst* inst) {
  switch (inst->op) {
    case arm::OpCode::B: {
      return 1;
      break;
    }
    case arm::OpCode::Bl: {
      return 1;
      break;
    }
    case arm::OpCode::Mov:
    case arm::OpCode::MovT:
    case arm::OpCode::Mvn:
    case arm::OpCode::Lsl:
    case arm::OpCode::Lsr:
    case arm::OpCode::Asr: {
      return 1;
      break;
    }
    case arm::OpCode::Add:
    case arm::OpCode::Sub:
    case arm::OpCode::And:
    case arm::OpCode::Orr:
    case arm::OpCode::Eor:
    case arm::OpCode::Bic: {
      if (shiftByImmed(((arm::Arith3Inst*)inst)->r2)) {
        return 2;
      } else {
        return 1;
      }
      break;
    }
    case arm::OpCode::Mul:
    case arm::OpCode::SMMul: {
      return 3;
      break;
    }
    case arm::OpCode::Cmp:
    case arm::OpCode::Cmn: {
      if (shiftByImmed(((arm::Arith2Inst*)inst)->r2)) {
        return 2;
      } else {
        return 1;
      }
      break;
    }
    case arm::OpCode::LdR: {
      return 4;
      break;
    }
    case arm::OpCode::StR: {
      return 1;
      break;
    }
    default: {
      throw WrongInstExceptionMsg;
    }
  }
};

bool shiftByImmed(arm::Operand2& r2) {
  return (std::holds_alternative<arm::RegisterOperand>(r2) &&
          (std::get<arm::RegisterOperand>(r2).shift !=
               arm::RegisterShiftKind::Lsl ||
           std::get<arm::RegisterOperand>(r2).shift_amount != 0));
};

void InstructionScheduler::scheduleBaseBlock(
    std::vector<arm::Inst*>& blockInsts,
    std::vector<std::unique_ptr<arm::Inst>>& newInsts) {
  std::vector<std::shared_ptr<DependencyDAGNode>> cands;

  buildDependencyDAG(blockInsts);

  for (auto& [index, node] : nodes) {
    if (inDegrees[index] == 0) {
      cands.push_back(node);
    }
  }

  for (size_t i = 0; i < blockInsts.size(); i++) {
    for (auto& code : allExePopeCodes) {
      if (exePipeLatency[code] > 0) {
        exePipeLatency[code]--;
      }
    }

    if (emptyExePipeSchedule(newInsts, cands)) {
    } else {
      i--;
    }
  }
};

bool InstructionScheduler::emptyExePipeSchedule(
    std::vector<std::unique_ptr<arm::Inst>>& newInsts,
    std::vector<std::shared_ptr<DependencyDAGNode>>& cands) {
  std::vector<std::shared_ptr<DependencyDAGNode>> pool;
  bool r;

  for (auto& cand : cands) {
    switch (cand->instKind) {
      case InstKind::Branch:
      case InstKind::Call: {
        if (exePipeLatency[ExePipeCode::Branch] == 0) {
          pool.push_back(cand);
        }
        break;
      }
      case InstKind::Integer: {
        if (exePipeLatency[ExePipeCode::Integer0] == 0 ||
            exePipeLatency[ExePipeCode::Integer1] == 0) {
          if (exePipeLatency[ExePipeCode::IntegerM] == 0 ||
              nodes[exePipeNode[ExePipeCode::IntegerM]]->successors.count(
                  cand) == 0) {
            pool.push_back(cand);
          }
        }
        break;
      }
      case InstKind::IntegerM: {
        if (exePipeLatency[ExePipeCode::IntegerM] == 0) {
          pool.push_back(cand);
        }
        break;
      }
      case InstKind::Load: {
        if (exePipeLatency[ExePipeCode::Load] == 0) {
          if (exePipeLatency[ExePipeCode::IntegerM] == 0 ||
              nodes[exePipeNode[ExePipeCode::IntegerM]]->successors.count(
                  cand) == 0) {
            pool.push_back(cand);
          }
        }
        break;
      }
      case InstKind::Store: {
        if (exePipeLatency[ExePipeCode::Store] == 0) {
          if (exePipeLatency[ExePipeCode::IntegerM] == 0 ||
              nodes[exePipeNode[ExePipeCode::IntegerM]]->successors.count(
                  cand) == 0) {
            pool.push_back(cand);
          }
        }
        break;
      }
    }
  }

  if (pool.empty()) {
    r = false;
  } else {
    r = true;
    if (pool.size() == 1) {
      std::sort(pool.begin(), pool.end(), cmpNodeSharedPtr);
    }

    switch (pool.front()->instKind) {
      case InstKind::Branch:
      case InstKind::Call: {
        exePipeLatency[ExePipeCode::Branch] = pool.front()->latency;
        exePipeNode[ExePipeCode::Branch] = pool.front()->originIndex;
        break;
      }
      case InstKind::Integer: {
        if (exePipeLatency[ExePipeCode::Integer0] == 0) {
          exePipeLatency[ExePipeCode::Integer0] = pool.front()->latency;
          exePipeNode[ExePipeCode::Integer0] = pool.front()->originIndex;
        } else {
          exePipeLatency[ExePipeCode::Integer1] = pool.front()->latency;
          exePipeNode[ExePipeCode::Integer1] = pool.front()->originIndex;
        }
        break;
      }
      case InstKind::IntegerM: {
        exePipeLatency[ExePipeCode::IntegerM] = pool.front()->latency;
        exePipeNode[ExePipeCode::IntegerM] = pool.front()->originIndex;
        break;
      }
      case InstKind::Load: {
        exePipeLatency[ExePipeCode::Load] = pool.front()->latency;
        exePipeNode[ExePipeCode::Load] = pool.front()->originIndex;
        break;
      }
      case InstKind::Store: {
        exePipeLatency[ExePipeCode::Store] = pool.front()->latency;
        exePipeNode[ExePipeCode::Store] = pool.front()->originIndex;
        break;
      }
      default:
        break;
    }

    newInsts.push_back(std::unique_ptr<arm::Inst>(pool.front()->inst));
    updateCands(0, cands);
  }
  return r;
};

void InstructionScheduler::updateCands(
    uint32_t indexOfCands,
    std::vector<std::shared_ptr<DependencyDAGNode>>& cands) {
  for (auto& successor : cands.at(indexOfCands)->successors) {
    if (inDegrees[successor->originIndex] == 1) {
      cands.push_back(successor);
    }
    inDegrees[successor->originIndex]--;
  }
  cands.erase(cands.begin() + indexOfCands);
};

void InstructionScheduler::buildDependencyDAG(
    std::vector<arm::Inst*>& blockInsts) {
  for (size_t i = 0; i < blockInsts.size(); i++) {
    arm::Inst* inst;
    std::shared_ptr<DependencyDAGNode> node;

    inst = blockInsts.at(i);
    node = std::make_shared<DependencyDAGNode>();

    node->originIndex = i;
    node->inst = inst;
    node->instKind = getInstKind(inst);
    node->latency = getInstExeLatency(inst);

    nodes[i] = node;
    inDegrees[i] = 0;

    switch (inst->op) {
      case arm::OpCode::B: {
        for (size_t j = 0; j < i; i++) {
          addSuccessor(j, i);
        }

        break;
      }
      case arm::OpCode::Bl: {
        uint32_t paraNum;

        paraNum = 0;
        for (uint32_t bInstNum = 0; i - 1 - bInstNum >= 0; bInstNum++) {
          if (i - 1 - bInstNum == lastCall) {
            break;
          }
          auto& bInst = blockInsts.at(i - 1 - bInstNum);
          if (paraNum <= 3) {
            if (bInst->op == arm::OpCode::Mov &&
                ((arm::Arith2Inst*)bInst)->r1 == paraNum) {
              addSuccessor(i - 1 - bInstNum, i);
              paraNum++;
              regDefNodes[((arm::Arith2Inst*)bInst)->r1] = i;
            }
            // TODO: deal the movt pass param bug
            else if (bInst->op == arm::OpCode::MovT &&
                     ((arm::Arith2Inst*)bInst)->r1 == paraNum) {
              addSuccessor(i - 1 - bInstNum, i);
            } else {
              break;
            }
          } else {
            if (bInst->op == arm::OpCode::StR) {
              arm::LoadStoreInst* storeInst = (arm::LoadStoreInst*)inst;
              if (std::holds_alternative<arm::MemoryOperand>(storeInst->mem) &&
                  std::get<arm::MemoryOperand>(storeInst->mem).r1 ==
                      arm::REG_SP) {
                arm::MemoryOperand& memOpd =
                    std::get<arm::MemoryOperand>(storeInst->mem);
                if (std::holds_alternative<int16_t>(memOpd.offset) &&
                    std::get<int16_t>(memOpd.offset) == (paraNum - 4) * 4) {
                  addSuccessor(i - 1 - bInstNum, i);
                  paraNum++;
                } else {
                  break;
                }
              } else {
                break;
              }
            } else {
              break;
            }
          }
        }

        if (lastMem != NullIndex) {
          addSuccessor(lastMem, i);
        }
        if (lastCall != NullIndex) {
          addSuccessor(lastCall, i);
        }
        lastMem = i;
        lastCall = i;

        break;
      }
      case arm::OpCode::Mov:
      case arm::OpCode::Mvn: {
        arm::Arith2Inst* movInst = (arm::Arith2Inst*)inst;

        if (movInst->cond != arm::ConditionCode::Always) {
          if (lastCmp != NullIndex) {
            addSuccessor(lastCmp, i);
          }
          lastCmp = i;
        }

        addRegReadDependency(i, movInst->r2);
        regDefNodes[movInst->r1] = i;
        break;
      }
      case arm::OpCode::MovT: {
        arm::Arith2Inst* movtInst = (arm::Arith2Inst*)inst;

        regDefNodes[movtInst->r1] = i;
        break;
      }
      case arm::OpCode::Lsl:
      case arm::OpCode::Lsr:
      case arm::OpCode::Asr:
      case arm::OpCode::Add:
      case arm::OpCode::Sub:
      case arm::OpCode::And:
      case arm::OpCode::Orr:
      case arm::OpCode::Eor:
      case arm::OpCode::Bic:
      case arm::OpCode::Mul:
      case arm::OpCode::SMMul: {
        arm::Arith3Inst* aluInst = (arm::Arith3Inst*)inst;
        if (aluInst->rd == arm::REG_SP && (aluInst->op == arm::OpCode::Add ||
                                           aluInst->op == arm::OpCode::Sub)) {
          if (lastCall != NullIndex) {
            addSuccessor(lastCall, i);
          }
          lastCall = i;
        }

        addRegReadDependency(i, aluInst->r1);
        addRegReadDependency(i, aluInst->r2);
        regDefNodes[aluInst->rd] = i;

        break;
      }
      case arm::OpCode::Cmp:
      case arm::OpCode::Cmn: {
        arm::Arith2Inst* cmpInst = (arm::Arith2Inst*)inst;

        if (lastCmp != NullIndex) {
          addSuccessor(lastCmp, i);
        }
        lastCmp = i;

        addRegReadDependency(i, cmpInst->r1);
        addRegReadDependency(i, cmpInst->r2);

        break;
      }
      case arm::OpCode::LdR: {
        arm::LoadStoreInst* ldInst = (arm::LoadStoreInst*)inst;

        if (lastMem != NullIndex) {
          addSuccessor(lastMem, i);
        }
        lastMem = i;

        if (std::holds_alternative<arm::MemoryOperand>(ldInst->mem)) {
          addRegReadDependency(i, std::get<arm::MemoryOperand>(ldInst->mem));
        }
        regDefNodes[ldInst->rd] = i;

        break;
      }
      case arm::OpCode::StR: {
        arm::LoadStoreInst* stInst = (arm::LoadStoreInst*)inst;

        if (lastMem != NullIndex) {
          addSuccessor(lastMem, i);
        }
        lastMem = i;

        if (std::holds_alternative<arm::MemoryOperand>(stInst->mem)) {
          addRegReadDependency(i, std::get<arm::MemoryOperand>(stInst->mem));
        }
        addRegReadDependency(i, stInst->rd);

        break;
      }
      default: {
        throw WrongInstExceptionMsg;
      }
    }
  }
};

void InstructionScheduler::addSuccessor(uint32_t father, uint32_t successor) {
  if (nodes[father]->successors.count(nodes[successor]) == 0) {
    nodes[father]->successors.insert(nodes[successor]);
    inDegrees[successor]++;
  }
}

void InstructionScheduler::addRegReadDependency(uint32_t successor,
                                                arm::Operand2& operand2) {
  if (std::holds_alternative<arm::RegisterOperand>(operand2)) {
    arm::Reg& r2 = std::get<arm::RegisterOperand>(operand2).reg;

    addRegReadDependency(successor, r2);
  }
};

void InstructionScheduler::addRegReadDependency(uint32_t successor,
                                                arm::Reg& reg) {
  if (regDefNodes.count(reg) > 0) {
    addSuccessor(regDefNodes[reg], successor);
  }
};

void InstructionScheduler::addRegReadDependency(uint32_t successor,
                                                arm::MemoryOperand& mem) {
  addRegReadDependency(successor, mem.r1);
  if (std::holds_alternative<arm::RegisterOperand>(mem.offset)) {
    arm::Reg& reg = std::get<arm::RegisterOperand>(mem.offset).reg;

    addRegReadDependency(successor, reg);
  }
};

};  // namespace backend::instruction_schedule

namespace backend::optimization {
void InstructionSchedule::optimize_arm(
    arm::ArmCode& armCode, std::map<std::string, std::any>& extraDataRepo) {
  for (auto& f : armCode.functions) {
    optimize_func(*f, extraDataRepo);
  }
};

void InstructionSchedule::optimize_func(
    arm::Function& f, std::map<std::string, std::any>& extraDataRepo) {
  std::vector<std::unique_ptr<arm::Inst>> inst_new;
  std::vector<arm::Inst*> blockInsts;
  for (size_t index = 0; index < f.inst.size(); index++) {
    arm::Inst* inst = f.inst.at(index).get();
    switch (inst->op) {
      case arm::OpCode::B: {
        if (blockInsts.empty()) {
          inst_new.push_back(std::unique_ptr<arm::Inst>(inst));
        } else if (blockInsts.size() < 2) {
          for (auto& blockInst : blockInsts) {
            inst_new.push_back(std::unique_ptr<arm::Inst>(blockInst));
          }
          inst_new.push_back(std::unique_ptr<arm::Inst>(inst));
          blockInsts.clear();
        } else {
          blockInsts.push_back(inst);
          instruction_schedule::InstructionScheduler scheduler =
              instruction_schedule::InstructionScheduler();
          scheduler.scheduleBaseBlock(blockInsts, inst_new);
          blockInsts.clear();
        }
        break;
      }
      case arm::OpCode::Bl:
      case arm::OpCode::Mov:
      case arm::OpCode::MovT:
      case arm::OpCode::Mvn:
      case arm::OpCode::Lsl:
      case arm::OpCode::Lsr:
      case arm::OpCode::Asr:
      case arm::OpCode::Add:
      case arm::OpCode::Sub:
      case arm::OpCode::And:
      case arm::OpCode::Orr:
      case arm::OpCode::Eor:
      case arm::OpCode::Bic:
      case arm::OpCode::Mul:
      case arm::OpCode::SMMul:
      case arm::OpCode::Cmp:
      case arm::OpCode::Cmn:
      case arm::OpCode::LdR:
      case arm::OpCode::StR: {
        blockInsts.push_back(inst);
        break;
      }
      default: {
        for (auto& blockInst : blockInsts) {
          inst_new.push_back(std::unique_ptr<arm::Inst>(blockInst));
        }
        inst_new.push_back(std::unique_ptr<arm::Inst>(inst));
        blockInsts.clear();
      }
    }
  }
  f.inst = std::move(inst_new);
};
};  // namespace backend::optimization