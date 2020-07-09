#include "reg_alloc.hpp"

#include <cassert>
#include <cmath>
#include <memory>
#include <vector>

#include "../../arm_code/regmanager.hpp"

namespace backend::codegen {
using namespace arm;

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
    add_starting_point(pt);
    add_ending_point(pt);
  }
  void add_starting_point(unsigned int start_) {
    if (start_ < start) start = start_;
  }
  void add_ending_point(unsigned int end_) {
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

struct Alloc {
  Reg reg;
  Interval interval;
};

class RegAllocator {
 public:
  RegAllocator(arm::Function &f)
      : f(f),
        live_intervals(),
        active(),
        stack_size(f.stack_size),
        spilled_regs(),
        spill_positions(),
        inst_sink() {}
  arm::Function &f;

  std::set<Reg> used_regs = {4, 5, 6, 7};

  std::map<arm::Reg, Interval> live_intervals;
  std::map<arm::Reg, Alloc> active;
  std::map<arm::Reg, Interval> spilled_regs;
  std::map<arm::Reg, int> spill_positions;

  //   std::multimap<int, SpillOperation> spill_operatons;
  std::vector<std::unique_ptr<arm::Inst>> inst_sink;

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
      r->second.add_ending_point(point);
    } else {
      live_intervals.insert({reg, Interval(point)});
    }
  }

  void add_reg_write(Reg reg, unsigned int point) {
    if (auto r = live_intervals.find(reg); r != live_intervals.end()) {
      r->second.add_starting_point(point);
    } else {
      live_intervals.insert({reg, Interval(point)});
    }
  }
#pragma endregion

  void calc_live_intervals();
  void alloc_regs();
  std::vector<std::pair<Reg, Interval>> sort_intervals();
  //   void generate_load_store_positions(std::vector<std::pair<Reg,
  //   Interval>>);
  void write_load(Reg r, Reg rd);
  void write_store(Reg r, Reg rs);
  void invalidate_read(int pos) {
    auto it = active.begin();
    while (it != active.end()) {
      if (it->second.interval.end <= pos)
        // The register is no longer to be read from, thus is freed
        it = active.erase(it);
      else
        it++;
    }
  }
  Reg make_space(Reg r, Interval i);
  Reg alloc_read(Reg r);
  Reg alloc_write(Reg r);
  void perform_load_stores();
};

void RegAllocator::alloc_regs() {
  calc_live_intervals();
  for (auto &r : live_intervals) {
    // Live interval is just used as a virtual register map for now.
    // This assigns a spill position for EVERY virtual register, which is very
    // very very very inefficient.
    spill_positions.insert({r.first, stack_size});
    stack_size += 4;
  }

  // TODO: Implement linear scan; The current method loads and stores every time
  // alloc regs based on interval
  //   auto intervals = sort_intervals();
  //   generate_load_store_positions(std::move(intervals));
  perform_load_stores();
  f.inst = std::move(inst_sink);

  {
    // Add used reg stuff
    auto &first = f.inst.front();
    auto first_ = static_cast<PushPopInst *>(&*first);
    for (auto r : used_regs) first_->regs.push_back(r);
    auto &last = f.inst.back();
    auto last_ = static_cast<PushPopInst *>(&*last);
    for (auto r : used_regs) last_->regs.push_back(r);
    f.inst.insert(f.inst.begin() + 2,
                  std::make_unique<Arith3Inst>(OpCode::Add, REG_SP, REG_SP,
                                               Operand2(stack_size)));
  }
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

void RegAllocator::write_load(Reg r, Reg rd) {
  inst_sink.push_back(std::make_unique<LoadStoreInst>(
      OpCode::LdR, rd, MemoryOperand(REG_SP, spill_positions.at(r))));
}

void RegAllocator::write_store(Reg r, Reg rs) {
  int pos;
  if (auto p = spill_positions.find(r); p != spill_positions.end()) {
    pos = p->second;
  } else {
    pos = stack_size;
    stack_size += 4;
    spill_positions.insert({r, pos});
  }
  inst_sink.push_back(std::make_unique<LoadStoreInst>(
      OpCode::StR, rs, MemoryOperand(REG_SP, pos)));
}

Reg RegAllocator::make_space(Reg r, Interval i) {
  throw new prelude::NotImplementedException();
}

Reg RegAllocator::alloc_read(Reg r) {
  if (!is_virtual_register(r))
    return r;
  else {
    // virtual register
    if (auto reg = active.find(r); reg != active.end()) {
      return reg->second.reg;
    } else if (auto spill = spilled_regs.find(r); spill != spilled_regs.end()) {
      Reg r_ = make_space(r, live_intervals.at(r));
      write_load(r, r_);
      return r_;
    } else {
      Reg r_ = make_space(r, live_intervals.at(r));
      return r_;
    }
  }
}

Reg RegAllocator::alloc_write(Reg r) {
  if (!is_virtual_register(r))
    return r;
  else {
    // virtual register
    if (auto reg = active.find(r); reg != active.end()) {
      return reg->second.reg;
    } else if (auto spill = spilled_regs.find(r); spill != spilled_regs.end()) {
      Reg r_ = make_space(r, live_intervals.at(r));
      write_load(r, r_);
      return r_;
    } else {
      Reg r_ = make_space(r, live_intervals.at(r));
      return r_;
    }
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

void RegAllocator::perform_load_stores() {
  for (int i = 0; i < f.inst.size(); i++) {
    auto inst_ = &*f.inst[i];
    // TODO: F_CK. Just use the plain old method for now.
    /**
     * The curren dirty hack:
     * Replace every register used in 3-operand moves with r4, r5 and r6
     * Replace every register used in 2-operand moves with r4, r5
     */
    if (auto x = dynamic_cast<Arith3Inst *>(inst_)) {
      if (is_virtual_register(x->r1)) {
        write_load(x->r1, 4);
        x->r1 = 4;
      }
      if (x->r2.is_virtual()) write_load(x->r2.get_reg(), 5);
      x->r2.replace_reg_if_virtual(5);
      inst_sink.push_back(std::move(f.inst[i]));
      if (is_virtual_register(x->rd)) {
        x->rd = 6;
        write_store(x->rd, 6);
      }
    } else if (auto x = dynamic_cast<Arith2Inst *>(inst_)) {
      if (x->r2.is_virtual()) write_load(x->r2.get_reg(), 5);
      x->r2.replace_reg_if_virtual(5);
      if (x->op == arm::OpCode::Mov || x->op == arm::OpCode::MovT) {
        Reg r1 = x->r1;
        if (is_virtual_register(r1)) {
          write_load(r1, 4);
        }
        inst_sink.push_back(std::move(f.inst[i]));
        if (is_virtual_register(r1)) {
          write_store(r1, 4);
          x->r1 = 4;
        }
      } else {
        if (is_virtual_register(x->r1)) {
          write_load(x->r1, 4);
          x->r1 = 4;
        }
        inst_sink.push_back(std::move(f.inst[i]));
      }
    } else if (auto x = dynamic_cast<LoadStoreInst *>(inst_)) {
      if (auto mem = std::get_if<MemoryOperand>(&x->mem)) {
        auto [vr1, vr2] = mem->is_virtual();
        if (is_virtual_register(vr1)) write_load(vr1, 4);
        if (vr2 && is_virtual_register(*vr2)) write_load(*vr2, 5);
        mem->replace_reg_if_virtual(4, 5);
      }
      if (x->op == arm::OpCode::LdR) {
        inst_sink.push_back(std::move(f.inst[i]));
        if (is_virtual_register(x->rd)) {
          x->rd = 6;
          write_store(x->rd, 6);
        }
      } else {
        // StR
        if (is_virtual_register(x->rd)) {
          x->rd = 6;
          write_load(x->rd, 6);
        }
        inst_sink.push_back(std::move(f.inst[i]));
      }
    } else if (auto x = dynamic_cast<MultLoadStoreInst *>(inst_)) {
      throw new prelude::NotImplementedException();
      if (x->op == arm::OpCode::LdM) {
        for (auto rd : x->rd) add_reg_write(rd, i);
      } else {
        // StM
        for (auto rd : x->rd) add_reg_read(rd, i);
      }
      add_reg_read(x->rn, i);
    } else if (auto x = dynamic_cast<PushPopInst *>(inst_)) {
      // push pop only use gpr
      inst_sink.push_back(std::move(f.inst[i]));
    } else
      inst_sink.push_back(std::move(f.inst[i]));
  }
}

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
