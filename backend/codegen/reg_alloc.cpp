#include "reg_alloc.hpp"

#include <cmath>
#include <vector>

#include "../../arm_code/regmanager.hpp"

using namespace arm;
namespace backend::codegen {

const std::set<Reg> GP_REGS = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

/// An interval represented by this struct is a semi-open interval
/// [start, end) where start means this value is first written and end means
/// this value is last read.
struct Interval {
  Interval(unsigned int point) : start(point), end(point) {}

  Interval(unsigned int start, unsigned int end) : start(start), end(end) {
    if (end < start) end = start;
  }

  unsigned int start;
  unsigned int end;

  void add_point(unsigned int pt) {
    update_start(pt);
    update_end(pt);
  }
  void update_start(unsigned int start_) {
    if (start_ < start) start = start_;
  }
  void update_end(unsigned int end_) {
    if (end_ > end) end = end_;
  }
  unsigned int length() { return end - start; }
  bool overlaps(const Interval other) {
    return end > other.start && start < other.end;
  }
};

struct SpillOperation {
  unsigned int index;
  bool is_store;
  Reg reg;

  // Sort by index, store<read and register number
  bool operator<(const SpillOperation &other) const {
    if (index != other.index) return index < other.index;
    if (is_store != other.is_store) return is_store;
    return reg < other.reg;
  }
};

/// A linear-scan register allocator. Performs register allocation using
/// second-chance binpacking algorithm.
class RegAllocator {
 public:
  RegAllocator(arm::Function &f)
      : f(f), live_intervals(), active_regs(), stack_size(f.stack_size) {}
  arm::Function &f;

  std::map<arm::Reg, Interval> live_intervals;
  std::map<arm::Reg, Interval> active_regs;
  std::map<arm::Reg, Interval> spilled_regs;
  std::map<arm::Reg, int> spill_positions;

  std::multimap<int, SpillOperation> spill_operatons;

  int stack_size;

#pragma region Read Write Stuff
  void add_reg_read(Operand2 &reg, unsigned int point) {
    if (auto x = std::get_if<RegisterOperand>(&reg)) {
      add_reg_read(x->reg, point);
    }
  }

  void add_reg_write(Operand2 &reg, unsigned int point) {
    if (auto x = std::get_if<RegisterOperand>(&reg)) {
      add_reg_write(x->reg, point);
    }
  }

  void add_reg_read(MemoryOperand &reg, unsigned int point) {
    add_reg_read(reg.r1, point);
    if (auto x = std::get_if<RegisterOperand>(&reg.offset)) {
      add_reg_read(x->reg, point);
    }
  }

  void add_reg_read(Reg reg, unsigned int point) {
    if (auto r = live_intervals.find(reg); r != live_intervals.end()) {
      r->second.update_end(point);
    } else {
      live_intervals.insert({reg, Interval(point)});
    }
  }

  void add_reg_write(Reg reg, unsigned int point) {
    if (auto r = live_intervals.find(reg); r != live_intervals.end()) {
      r->second.update_start(point);
    } else {
      live_intervals.insert({reg, Interval(point)});
    }
  }
#pragma endregion

  void calc_live_intervals();
  void alloc_regs();
  std::vector<std::pair<Reg, Interval>> sort_intervals();
  void generate_load_store_positions(std::vector<std::pair<Reg, Interval>>);
  void perform_load_stores();
};

void RegAllocator::alloc_regs() {
  calc_live_intervals();
  // alloc regs based on interval
  auto intervals = sort_intervals();
  generate_load_store_positions(std::move(intervals));
  perform_load_stores();
}

void RegAllocator::calc_live_intervals() {
  for (int i = 0; i < f.inst.size(); i++) {
    auto inst_ = &*f.inst[i];
    if (auto x = dynamic_cast<PureInst *>(inst_)) {
      //   noop
    } else if (auto x = dynamic_cast<Arith3Inst *>(inst_)) {
      add_reg_read(x->r1, i);
      add_reg_read(x->r2, i);
      add_reg_write(x->rd, i);
    } else if (auto x = dynamic_cast<Arith2Inst *>(inst_)) {
      if (x->op == arm::OpCode::Mov || x->op == arm::OpCode::MovT) {
        add_reg_write(x->r1, i);
      } else {
        add_reg_read(x->r1, i);
      }
      add_reg_read(x->r2, i);
    } else if (auto x = dynamic_cast<BrInst *>(inst_)) {
      //   noop
    } else if (auto x = dynamic_cast<LoadStoreInst *>(inst_)) {
      if (x->op == arm::OpCode::LdR) {
        add_reg_write(x->rd, i);
      } else {
        // StR
        add_reg_read(x->rd, i);
      }
      if (auto mem = std::get_if<MemoryOperand>(&x->mem)) add_reg_read(*mem, i);
    } else if (auto x = dynamic_cast<MultLoadStoreInst *>(inst_)) {
      if (x->op == arm::OpCode::LdM) {
        for (auto rd : x->rd) add_reg_write(rd, i);
      } else {
        // StM
        for (auto rd : x->rd) add_reg_read(rd, i);
      }
      add_reg_read(x->rn, i);
    } else if (auto x = dynamic_cast<PushPopInst *>(inst_)) {
      if (x->op == arm::OpCode::Push) {
        for (auto rd : x->regs) add_reg_write(rd, i);
      } else {
        // pop
        for (auto rd : x->regs) add_reg_read(rd, i);
      }
    } else if (auto x = dynamic_cast<LabelInst *>(inst_)) {
      //   noop
    } else
      //   noop
      ;
  }
}

std::vector<std::pair<Reg, Interval>> RegAllocator::sort_intervals() {
  //   Initialize vector
  std::vector<std::pair<Reg, Interval>> intervals;
  intervals.reserve(live_intervals.size());
  for (auto interval_pair : live_intervals) {
    intervals.push_back(interval_pair);
  }
  //   sort by interval starting point
  std::sort(intervals.begin(), intervals.end(), [](auto &first, auto &second) {
    return first.second.start < second.second.start;
  });

  return std::move(intervals);
}

void RegAllocator::generate_load_store_positions(
    std::vector<std::pair<Reg, Interval>> unhandled) {
  for (auto interval : unhandled) {
    // todo
  }
}

void RegAllocator::perform_load_stores() {}

void RegAllocatePass::optimize_arm(
    arm::ArmCode &arm_code, std::map<std::string, std::any> &extra_data_repo) {
  for (auto &f : arm_code.functions) {
    optimize_func(*f);
  }
}

void RegAllocatePass::optimize_func(arm::Function &f) {
  RegAllocator fal(f);
  fal.alloc_regs();
}

}  // namespace backend::codegen
