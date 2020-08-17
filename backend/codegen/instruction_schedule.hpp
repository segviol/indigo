#pragma once

#include "../backend.hpp"

namespace backend::instruction_schedule {
  extern const char WrongInstExceptionMsg[];

  enum class ExePipeCode {
    Branch,
    Integer0,
    Integer1,
    IntegerM,
    Load,
    Store
  };

  enum class InstKind {
    Branch,
    Call,
    Integer,
    IntegerM,
    Load,
    Store
  };

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

    private:
  };

  class InstructionScheduler {
    public:
    InstructionScheduler() 
    {
      lastMem = NullIndex;
      lastCmp = NullIndex;
      lastCall = NullIndex;
    };

    const uint32_t NullIndex = (1 << 20);

    // begin with label inst, end with B inst, only legel instruction included
    // the blockInsts will include B inst in the end and won`t include label inst
    void scheduleBaseBlock(std::vector<arm::Inst*> &blockInsts, std::vector<std::unique_ptr<arm::Inst>> &newInsts);

    private:
    std::map<uint32_t, std::shared_ptr<DependencyDAGNode>> nodes;

    std::map<arm::Reg, uint32_t> regDefNodes;
    uint32_t lastMem;
    uint32_t lastCmp;
    uint32_t lastCall;

    std::map<uint32_t, uint32_t> inDegrees;

    void buildDependencyDAG(std::vector<arm::Inst*> &blockInsts);

    void addSuccessor(uint32_t father, uint32_t successor);
    void addRegReadDependency(uint32_t successor, arm::Operand2& operand2);
    void addRegReadDependency(uint32_t successor, arm::Reg& reg);
    void addRegReadDependency(uint32_t successor, arm::MemoryOperand& mem);
  };
};

namespace backend::codegen {
  class InstructionSchedule final : public backend::ArmOptimizePass {
    public:
    std::string pass_name() const override { return "instruction_schedule"; };
    void optimize_arm(
      arm::ArmCode &armCode, std::map<std::string, std::any> &extraDataRepo
    ) override;
    
    private:
    void optimize_func(arm::Function &f, std::map<std::string, std::any> &extraDataRepo);
  };
};