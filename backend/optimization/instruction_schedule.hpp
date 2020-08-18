#pragma once

#include "../backend.hpp"
#include "algorithm"

namespace backend::instruction_schedule {
extern const std::string WrongInstExceptionMsg;

arm::Inst* copyInst(arm::Inst* inst);

enum class ExePipeCode { Branch, Integer0, Integer1, IntegerM, Load, Store };

extern std::vector<ExePipeCode> allExePopeCodes;

enum class InstKind { Branch, Call, Integer, IntegerM, Load, Store };

InstKind getInstKind(arm::Inst* inst);
uint32_t getInstExeLatency(arm::Inst* inst);

bool shiftByImmed(arm::Operand2& r2);

class DependencyDAGNode {
 public:
  uint32_t originIndex;
  arm::Inst* inst;
  InstKind instKind;
  uint32_t latency;
  std::set<std::shared_ptr<DependencyDAGNode>> successors;

  bool operator<(const DependencyDAGNode& other) const {
    bool r;
    if (latency < other.latency) {
      r = false;
    } else if (latency > other.latency) {
      r = true;
    } else {
      if (originIndex < other.originIndex) {
        r = true;
      } else {
        r = false;
      }
    }
    return r;
  }

 private:
};

bool cmpNodeSharedPtr(const std::shared_ptr<DependencyDAGNode>& a,
                      const std::shared_ptr<DependencyDAGNode>& b);

class InstructionScheduler {
 public:
  InstructionScheduler() {
    lastMem = NullIndex;
    lastCmp = NullIndex;
    lastCall = NullIndex;

    for (auto& code : allExePopeCodes) {
      exePipeLatency[code] = 0;
    }
  };

  const uint32_t NullIndex = (1 << 20);

  // begin with label inst, end with B inst, only legel instruction included
  // the blockInsts will include B inst in the end and won`t include label inst
  void scheduleBaseBlock(std::vector<arm::Inst*>& blockInsts,
                         std::vector<std::unique_ptr<arm::Inst>>& newInsts);

 private:
  std::map<uint32_t, std::shared_ptr<DependencyDAGNode>> nodes;

  std::map<arm::Reg, uint32_t> regDefNodes;
  uint32_t lastMem;
  uint32_t lastCmp;
  uint32_t lastCall;

  std::map<uint32_t, uint32_t> inDegrees;
  std::map<ExePipeCode, uint32_t> exePipeLatency;
  std::map<ExePipeCode, uint32_t> exePipeNode;

  void buildDependencyDAG(std::vector<arm::Inst*>& blockInsts);

  bool emptyExePipeSchedule(
      std::vector<std::unique_ptr<arm::Inst>>& newInsts,
      std::set<std::shared_ptr<DependencyDAGNode>>& cands);

  void updateCands(uint32_t index,
                   std::set<std::shared_ptr<DependencyDAGNode>>& cands);

  void addSuccessor(uint32_t father, uint32_t successor);
  void addRegReadDependency(uint32_t successor, arm::Operand2& operand2);
  void addRegReadDependency(uint32_t successor, arm::Reg& reg);
  void addRegReadDependency(uint32_t successor, arm::MemoryOperand& mem);
  void addRegWriteDependency(uint32_t successor, arm::Reg reg);
};
};  // namespace backend::instruction_schedule

namespace backend::optimization {
class InstructionSchedule final : public backend::ArmOptimizePass {
 public:
  std::string pass_name() const override { return "instruction_schedule"; };
  void optimize_arm(arm::ArmCode& armCode,
                    std::map<std::string, std::any>& extraDataRepo) override;

 private:
  void optimize_func(arm::Function& f,
                     std::map<std::string, std::any>& extraDataRepo);
};
};  // namespace backend::optimization