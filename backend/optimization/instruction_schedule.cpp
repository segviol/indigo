#include "instruction_schedule.hpp"

namespace backend::instruction_schedule {

arm::Inst* copyInst(arm::Inst* inst) {
  switch (inst->op) {
    case arm::OpCode::Nop: {
      return new arm::PureInst(*((arm::PureInst*)inst));
      break;
    }
    case arm::OpCode::B:
    case arm::OpCode::Bl:
    case arm::OpCode::Bx:
    case arm::OpCode::Cbz:
    case arm::OpCode::Cbnz: {
      return new arm::BrInst(*((arm::BrInst*)inst));
      break;
    }
    case arm::OpCode::Mov:
    case arm::OpCode::MovT:
    case arm::OpCode::Mvn:
    case arm::OpCode::Cmp:
    case arm::OpCode::Cmn: {
      return new arm::Arith2Inst(*((arm::Arith2Inst*)inst));
      break;
    }
    case arm::OpCode::Add:
    case arm::OpCode::Sub:
    case arm::OpCode::Rsb:
    case arm::OpCode::Mul:
    case arm::OpCode::SMMul:
    case arm::OpCode::SDiv:
    case arm::OpCode::And:
    case arm::OpCode::Orr:
    case arm::OpCode::Eor:
    case arm::OpCode::Bic:
    case arm::OpCode::Lsl:
    case arm::OpCode::Lsr:
    case arm::OpCode::Asr: {
      return new arm::Arith3Inst(*((arm::Arith3Inst*)inst));
      break;
    }
    case arm::OpCode::LdR:
    case arm::OpCode::StR: {
      return new arm::LoadStoreInst(*((arm::LoadStoreInst*)inst));
      break;
    }
    case arm::OpCode::LdM:
    case arm::OpCode::StM: {
      return new arm::MultLoadStoreInst(*((arm::MultLoadStoreInst*)inst));
      break;
    }
    case arm::OpCode::Push:
    case arm::OpCode::Pop: {
      return new arm::PushPopInst(*((arm::PushPopInst*)inst));
      break;
    }
    case arm::OpCode::_Label: {
      return new arm::LabelInst(*((arm::LabelInst*)inst));
      break;
    }
    case arm::OpCode::_Ctrl: {
      return new arm::CtrlInst(*((arm::CtrlInst*)inst));
      break;
    }
    case arm::OpCode::_Mod:
    default:
      break;
  }
}

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
  std::set<std::shared_ptr<DependencyDAGNode>> cands;

  buildDependencyDAG(blockInsts);

  for (auto& [index, node] : nodes) {
    if (inDegrees[index] == 0) {
      cands.insert(node);
    }
  }

  for (size_t i = 0; i < blockInsts.size();) {
    for (auto& code : allExePopeCodes) {
      if (exePipeLatency[code] > 0) {
        exePipeLatency[code]--;
      }
    }

    if (emptyExePipeSchedule(newInsts, cands)) {
      i++;
    }
  }
};

bool InstructionScheduler::exePipeConflict(
    const std::shared_ptr<DependencyDAGNode>& cand, ExePipeCode exePipeCode) {
  return (exePipeLatency[exePipeCode] > 0 &&
          nodes[exePipeNode[exePipeCode]]->successors.count(cand) > 0);
}

bool InstructionScheduler::emptyExePipeSchedule(
    std::vector<std::unique_ptr<arm::Inst>>& newInsts,
    std::set<std::shared_ptr<DependencyDAGNode>>& cands) {
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
        if (exePipeLatency[ExePipeCode::Integer0] == 0) {
          if (!exePipeConflict(cand, ExePipeCode::Integer1) &&
              !exePipeConflict(cand, ExePipeCode::IntegerM) &&
              !exePipeConflict(cand, ExePipeCode::Load) &&
              !exePipeConflict(cand, ExePipeCode::Store)) {
            pool.push_back(cand);
          }
        } else if (exePipeLatency[ExePipeCode::Integer1] == 0) {
          if (!exePipeConflict(cand, ExePipeCode::Integer0) &&
              !exePipeConflict(cand, ExePipeCode::IntegerM) &&
              !exePipeConflict(cand, ExePipeCode::Load) &&
              !exePipeConflict(cand, ExePipeCode::Store)) {
            pool.push_back(cand);
          }
        }
        break;
      }
      case InstKind::IntegerM: {
        if (exePipeLatency[ExePipeCode::IntegerM] == 0) {
          if (!exePipeConflict(cand, ExePipeCode::Integer0) &&
              !exePipeConflict(cand, ExePipeCode::Integer1) &&
              !exePipeConflict(cand, ExePipeCode::Load) &&
              !exePipeConflict(cand, ExePipeCode::Store)) {
            pool.push_back(cand);
          }
        }
        break;
      }
      case InstKind::Load: {
        if (exePipeLatency[ExePipeCode::Load] == 0) {
          if (!exePipeConflict(cand, ExePipeCode::Integer0) &&
              !exePipeConflict(cand, ExePipeCode::Integer1) &&
              !exePipeConflict(cand, ExePipeCode::IntegerM) &&
              !exePipeConflict(cand, ExePipeCode::Store)) {
            pool.push_back(cand);
          }
        }
        break;
      }
      case InstKind::Store: {
        if (exePipeLatency[ExePipeCode::Store] == 0) {
          if (!exePipeConflict(cand, ExePipeCode::Integer0) &&
              !exePipeConflict(cand, ExePipeCode::Integer1) &&
              !exePipeConflict(cand, ExePipeCode::IntegerM) &&
              !exePipeConflict(cand, ExePipeCode::Load)) {
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
    if (pool.size() > 1) {
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

    newInsts.push_back(
        std::unique_ptr<arm::Inst>(copyInst(pool.front()->inst)));
    updateCands(pool.front()->originIndex, cands);
  }
  return r;
};

void InstructionScheduler::updateCands(
    uint32_t index, std::set<std::shared_ptr<DependencyDAGNode>>& cands) {
  for (auto& successor : nodes[index]->successors) {
    if (inDegrees[successor->originIndex] == 1) {
      cands.insert(successor);
    }
    inDegrees[successor->originIndex]--;
  }
  cands.erase(nodes[index]);
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
        for (size_t j = 0; j < i; j++) {
          addSuccessor(j, i);
        }

        break;
      }
      case arm::OpCode::Bl: {
        if (lastMem != NullIndex) {
          addSuccessor(lastMem, i);
        }
        lastMem = i;

        addCmpReadDependency(i);
        addCmpWriteDependency(i);

        if (lastCall != NullIndex) {
          addSuccessor(lastCall, i);
        }
        lastCall = i;

        for (auto tmpReg : arm::TEMP_REGS) {
          addRegReadDependency(i, tmpReg);
        }
        for (auto& tmpReg : arm::TEMP_REGS) {
          addRegWriteDependency(i, tmpReg);
        }

        setCmpReadNode(i);
        setCmpWriteNode(i);
        for (auto tmpReg : arm::TEMP_REGS) {
          setRegReadNode(i, tmpReg);
        }
        for (auto& tmpReg : arm::TEMP_REGS) {
          setRegWriteNode(i, tmpReg);
        }
        break;
      }
      case arm::OpCode::Mov:
      case arm::OpCode::Mvn: {
        arm::Arith2Inst* movInst = (arm::Arith2Inst*)inst;

        if (movInst->cond != arm::ConditionCode::Always) {
          addCmpReadDependency(i);
        }

        addRegReadDependency(i, movInst->r2);
        addRegWriteDependency(i, movInst->r1);

        if (movInst->cond != arm::ConditionCode::Always) {
          setCmpReadNode(i);
        }

        setRegReadNode(i, movInst->r2);
        setRegWriteNode(i, movInst->r1);
        break;
      }
      case arm::OpCode::MovT: {
        arm::Arith2Inst* movtInst = (arm::Arith2Inst*)inst;

        if (movtInst->cond != arm::ConditionCode::Always) {
          addCmpReadDependency(i);
        }

        addRegWriteDependency(i, movtInst->r1);

        if (movtInst->cond != arm::ConditionCode::Always) {
          setCmpReadNode(i);
        }

        setRegWriteNode(i, movtInst->r1);
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

        if (aluInst->cond != arm::ConditionCode::Always) {
          addCmpReadDependency(i);
        }

        addRegReadDependency(i, aluInst->r1);
        addRegReadDependency(i, aluInst->r2);
        addRegWriteDependency(i, aluInst->rd);

        if (aluInst->cond != arm::ConditionCode::Always) {
          setCmpReadNode(i);
        }

        setRegReadNode(i, aluInst->r1);
        setRegReadNode(i, aluInst->r2);
        setRegWriteNode(i, aluInst->rd);
        break;
      }
      case arm::OpCode::Cmp:
      case arm::OpCode::Cmn: {
        arm::Arith2Inst* cmpInst = (arm::Arith2Inst*)inst;

        if (cmpInst->cond != arm::ConditionCode::Always) {
          addCmpReadDependency(i);
        }
        addCmpWriteDependency(i);

        addRegReadDependency(i, cmpInst->r1);
        addRegReadDependency(i, cmpInst->r2);

        if (cmpInst->cond != arm::ConditionCode::Always) {
          setCmpReadNode(i);
        }
        setCmpWriteNode(i);

        setRegReadNode(i, cmpInst->r1);
        setRegReadNode(i, cmpInst->r2);
        break;
      }
      case arm::OpCode::LdR: {
        arm::LoadStoreInst* ldInst = (arm::LoadStoreInst*)inst;

        if (ldInst->cond != arm::ConditionCode::Always) {
          addCmpReadDependency(i);
        }

        if (lastMem != NullIndex) {
          addSuccessor(lastMem, i);
        }
        lastMem = i;

        if (std::holds_alternative<arm::MemoryOperand>(ldInst->mem)) {
          addRegReadDependency(i, std::get<arm::MemoryOperand>(ldInst->mem));
        }
        if (std::holds_alternative<std::string>(ldInst->mem)) {
          for (auto& tmpReg : arm::TEMP_REGS) {
            addRegWriteDependency(i, tmpReg);
          }
        }
        addRegWriteDependency(i, ldInst->rd);

        if (ldInst->cond != arm::ConditionCode::Always) {
          setCmpReadNode(i);
        }
        if (std::holds_alternative<arm::MemoryOperand>(ldInst->mem)) {
          setRegReadNode(i, std::get<arm::MemoryOperand>(ldInst->mem));
        }
        if (std::holds_alternative<std::string>(ldInst->mem)) {
          for (auto& reg : arm::TEMP_REGS) {
            setRegWriteNode(i, reg);
          }
        }
        setRegWriteNode(i, ldInst->rd);
        break;
      }
      case arm::OpCode::StR: {
        arm::LoadStoreInst* stInst = (arm::LoadStoreInst*)inst;

        if (stInst->cond != arm::ConditionCode::Always) {
          addCmpReadDependency(i);
        }

        if (lastMem != NullIndex) {
          addSuccessor(lastMem, i);
        }
        lastMem = i;

        if (std::holds_alternative<arm::MemoryOperand>(stInst->mem)) {
          addRegReadDependency(i, std::get<arm::MemoryOperand>(stInst->mem));
        }
        addRegReadDependency(i, stInst->rd);

        if (stInst->cond != arm::ConditionCode::Always) {
          setCmpReadNode(i);
        }
        if (std::holds_alternative<arm::MemoryOperand>(stInst->mem)) {
          setRegReadNode(i, std::get<arm::MemoryOperand>(stInst->mem));
        }
        setRegReadNode(i, stInst->rd);
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
};

void InstructionScheduler::addCmpReadDependency(uint32_t successor) {
  if (lastCmp != NullIndex && lastCmp != successor) {
    addSuccessor(lastCmp, successor);
  }
};

void InstructionScheduler::addCmpWriteDependency(uint32_t successor) {
  if (lastCmp != NullIndex && lastCmp != successor) {
    addSuccessor(lastCmp, successor);
  }
  for (auto& node : readCmpNodes) {
    if (node != successor) {
      addSuccessor(node, successor);
    }
  }
};

void InstructionScheduler::setCmpReadNode(uint32_t successor) {
  readCmpNodes.insert(successor);
};

void InstructionScheduler::setCmpWriteNode(uint32_t successor) {
  lastCmp = successor;
  readCmpNodes.clear();
};

void InstructionScheduler::setRegReadNode(uint32_t successor,
                                          arm::Operand2& operand2) {
  if (std::holds_alternative<arm::RegisterOperand>(operand2)) {
    arm::Reg& r2 = std::get<arm::RegisterOperand>(operand2).reg;

    setRegReadNode(successor, r2);
  }
};

void InstructionScheduler::setRegReadNode(uint32_t successor, arm::Reg& reg) {
  regReadNodes[reg].insert(successor);
};

void InstructionScheduler::setRegReadNode(uint32_t successor,
                                          arm::MemoryOperand& mem) {
  setRegReadNode(successor, mem.r1);
  if (std::holds_alternative<arm::RegisterOperand>(mem.offset)) {
    arm::Reg& reg = std::get<arm::RegisterOperand>(mem.offset).reg;

    setRegReadNode(successor, reg);
  }
};

void InstructionScheduler::setRegWriteNode(uint32_t successor, arm::Reg reg) {
  regDefNodes[reg] = successor;
  regReadNodes[reg].clear();
};

void InstructionScheduler::addRegReadDependency(uint32_t successor,
                                                arm::Operand2& operand2) {
  if (std::holds_alternative<arm::RegisterOperand>(operand2)) {
    arm::Reg& r2 = std::get<arm::RegisterOperand>(operand2).reg;

    addRegReadDependency(successor, r2);
  }
};

void InstructionScheduler::addRegReadDependency(uint32_t successor,
                                                arm::Reg& reg) {
  if (regDefNodes.count(reg) > 0 && regDefNodes[reg] != successor) {
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

void InstructionScheduler::addRegWriteDependency(uint32_t successor,
                                                 arm::Reg reg) {
  if (regDefNodes.count(reg) > 0 && regDefNodes[reg] != successor) {
    addSuccessor(regDefNodes[reg], successor);
  }
  if (regReadNodes.count(reg) > 0) {
    std::set<uint32_t>& readNodes = regReadNodes.at(reg);
    for (auto readNode = readNodes.begin(); readNode != readNodes.end();
         readNode++) {
      if (*readNode != successor) {
        addSuccessor(*readNode, successor);
      }
    }
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
      case arm::OpCode::B:
      default: {
        if (blockInsts.empty()) {
          inst_new.push_back(
              std::unique_ptr<arm::Inst>(instruction_schedule::copyInst(inst)));
        } else if (blockInsts.size() < 2) {
          for (auto& blockInst : blockInsts) {
            inst_new.push_back(std::unique_ptr<arm::Inst>(
                instruction_schedule::copyInst(blockInst)));
          }
          inst_new.push_back(
              std::unique_ptr<arm::Inst>(instruction_schedule::copyInst(inst)));
          blockInsts.clear();
        } else {
          instruction_schedule::InstructionScheduler scheduler =
              instruction_schedule::InstructionScheduler();
          scheduler.scheduleBaseBlock(blockInsts, inst_new);
          inst_new.push_back(
              std::unique_ptr<arm::Inst>(instruction_schedule::copyInst(inst)));
          blockInsts.clear();
        }
      }
    }
  }
  f.inst = std::move(inst_new);
};
};  // namespace backend::optimization