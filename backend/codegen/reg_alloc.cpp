#include "reg_alloc.hpp"

#include <any>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "../optimization/graph_color.hpp"
#include "../optimization/optimization.hpp"
#include "aixlog.hpp"

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

using ColorMap = ::optimization::graph_color::Color_Map;

const std::set<Reg> temp_regs = {0, 1, 2, 3, 12};

class RegAllocator {
 public:
  RegAllocator(arm::Function &f, ColorMap &color_map,
               std::map<mir::inst::VarId, Reg> &mir_to_arm)
      : f(f),
        live_intervals(),
        reg_map(),
        stack_size(f.stack_size),
        spilled_regs(),
        spill_positions(),
        color_map(color_map),
        mir_to_arm(mir_to_arm),
        inst_sink() {}
  arm::Function &f;
  ColorMap &color_map;
  optimization::MirVariableToArmVRegType::mapped_type &mir_to_arm;

  std::set<Reg> used_regs = {};

  std::unordered_map<arm::Reg, Interval> live_intervals;
  std::unordered_map<arm::Reg, Reg> reg_map;
  // key: physical register; value: allocation interval
  std::unordered_map<arm::Reg, Interval> active;
  // key: virtual register; value: physical register
  std::unordered_map<arm::Reg, Reg> active_reg_map;
  std::unordered_map<arm::Reg, Interval> spilled_regs;
  std::unordered_map<arm::Reg, int> spill_positions;

  //   std::multimap<int, SpillOperation> spill_operatons;
  std::vector<std::unique_ptr<arm::Inst>> inst_sink;

  int stack_size;
  int stack_offset = 0;
  std::optional<std::pair<Reg, Reg>> delayed_store;

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
  void construct_reg_map();
  std::vector<std::pair<Reg, Interval>> sort_intervals();
  //   void generate_load_store_positions(std::vector<std::pair<Reg,
  //   Interval>>);
  void replace_read(Reg &r, int i);
  void replace_write(Reg &r, int i);
  void replace_read(Operand2 &r, int i);
  void replace_read(MemoryOperand &r, int i);
  void invalidate_read(int pos) {
    auto it = active.begin();
    while (it != active.end()) {
      if (it->second.end <= pos) {
        // The register is no longer to be read from, thus is freed
        for (auto r : active_reg_map) {
          if (r.second == it->first) {
            active_reg_map.erase(r.first);
            break;
          }
        }
        it = active.erase(it);
      } else {
        it++;
      }
    }
  }
  Reg alloc_transient_reg(Interval i, std::optional<Reg> orig);
  Reg make_space(Reg r, Interval i);
  Reg alloc_read(Reg r);
  Reg alloc_write(Reg r);
  void force_free(Reg r);
  int get_or_alloc_spill_pos(Reg r) {
    int pos;
    display_reg_name(LOG(TRACE), r);
    if (auto p = spill_positions.find(r); p != spill_positions.end()) {
      pos = p->second;
    } else {
      LOG(TRACE) << " set";
      pos = stack_size;
      stack_size += 4;
      spill_positions.insert({r, pos + stack_offset});
    }
    LOG(TRACE) << " " << pos << std::endl;
    return pos;
  }
  void perform_load_stores();
};

void RegAllocator::alloc_regs() {
  calc_live_intervals();
  // for (auto &r : live_intervals) {
  //   // Live interval is just used as a virtual register map for now.
  //   // This assigns a spill position for EVERY virtual register, which is
  //   very
  //   // very very very inefficient.
  //   spill_positions.insert({r.first, stack_size});
  //   stack_size += 4;
  // }

  LOG(TRACE, "color_map") << "Color map:" << std::endl;
  for (auto x : color_map) {
    auto mapped_reg = mir_to_arm.at(x.first);
    LOG(TRACE, "color_map") << x.first << " -> ";
    display_reg_name(LOG(TRACE, "color_map"), mapped_reg);
    LOG(TRACE, "color_map") << ": " << x.second << std::endl;
  }

  construct_reg_map();
  perform_load_stores();
  f.inst = std::move(inst_sink);

  {
    // Add used reg stuff
    auto &first = f.inst.front();
    auto first_ = static_cast<PushPopInst *>(&*first);
    for (auto r : used_regs) first_->regs.insert(r);
    auto &last = f.inst.back();
    auto last_ = static_cast<PushPopInst *>(&*last);
    for (auto r : used_regs) last_->regs.insert(r);
    f.inst.insert(f.inst.begin() + 2,
                  std::make_unique<Arith3Inst>(OpCode::Sub, REG_SP, REG_SP,
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
      if (x->op == arm::OpCode::Mov || x->op == arm::OpCode::MovT ||
          x->op == arm::OpCode::Mvn) {
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

void RegAllocator::construct_reg_map() {
  for (auto item : mir_to_arm) {
    // for every mir variable:
    auto [var_id, vreg_id] = item;
    auto color = color_map.find(var_id);
    if (color != color_map.end()) {
      if (color->second != -1) {
        // Global register id starts with r4;
        auto reg = Reg(color->second + 4);
        reg_map.insert({vreg_id, reg});
        used_regs.insert(reg);
        {
          auto &trace = LOG(TRACE);
          trace << var_id << " <- ";
          display_reg_name(trace, vreg_id);
          trace << " <- ";
          display_reg_name(trace, reg);
          trace << std::endl;
        }
      } else {
        spill_positions.insert({vreg_id, stack_size});
        {
          auto &trace = LOG(TRACE);
          trace << "$" << var_id << " <- ";
          display_reg_name(trace, vreg_id);
          trace << " <- sp + " << stack_size << std::endl;
        }
        stack_size += 4;
      }
    } else {
      // local variable
      {
        auto &trace = LOG(TRACE);
        trace << "$" << var_id << " <- ";
        display_reg_name(trace, vreg_id);
        trace << " <- local ";
        trace << std::endl;
      }
    }
  }
}

void RegAllocator::replace_read(Operand2 &r, int i) {
  if (auto rop = std::get_if<RegisterOperand>(&r)) {
    replace_read(rop->reg, i);
  }
}

void RegAllocator::replace_read(MemoryOperand &r, int i) {
  replace_read(r.r1, i);
  if (auto rop = std::get_if<RegisterOperand>(&r.offset)) {
    replace_read(rop->reg, i);
  }
}

Reg RegAllocator::alloc_transient_reg(Interval i, std::optional<Reg> orig) {
  Reg r = -1;
  if (orig) {
    auto a = active_reg_map.find(orig.value());
    if (a != active_reg_map.end()) {
      return a->second;
    }
  }
  for (auto reg : temp_regs) {
    if (active.find(reg) == active.end()) {
      r = reg;
      break;
    }
  }
  if (r == -1) {
    // Choose a value in active to spill.
    // NOTE: the current algorithm chooses the first non-temporary register to
    // spill.
    if (active_reg_map.size() == 0) {
      std::stringstream ss;
      ss << "Failed to allocate: all active registers are "
            "temporary!"
         << std::endl
         << "Dump: " << std::endl;

      for (auto [k, v] : active) {
        display_reg_name(ss, k);
        ss << ": [" << v.start << ", " << v.end << "]" << std::endl;
      }

      throw std::runtime_error(ss.str());
    }
    auto [virt_reg, phys_reg] = *active_reg_map.begin();
    auto interval = active.at(phys_reg);
    interval.start = i.start;

    int spill_pos = get_or_alloc_spill_pos(virt_reg);
    // TODO: move this into a separate function
    inst_sink.push_back(std::make_unique<LoadStoreInst>(
        OpCode::StR, phys_reg,
        MemoryOperand(REG_SP, spill_pos + stack_offset)));
    r = phys_reg;
    spilled_regs.insert({virt_reg, interval});
    active_reg_map.erase(virt_reg);
    active.erase(phys_reg);
  }
  this->active.insert({r, i});
  if (orig) {
    this->active_reg_map.insert({orig.value(), r});
  }
  return r;
}

/// Replace virtual register r with real register in-place
void RegAllocator::replace_read(Reg &r, int i) {
  auto disp_reg = [&]() {
    display_reg_name(LOG(TRACE), r);
    LOG(TRACE) << " at: " << i << " ";
  };
  if (!is_virtual_register(r)) {
    disp_reg();
    LOG(TRACE) << "phys" << std::endl;
    return;
  } else if (auto reg_map_r = reg_map.find(r); reg_map_r != reg_map.end()) {
    // This register is allocated with graph-coloring
    disp_reg();
    LOG(TRACE) << "graph " << reg_map_r->second << std::endl;
    r = reg_map_r->second;
    return;
  } else if (auto spill_r = spill_positions.find(r);
             spill_r != spill_positions.end()) {
    // this register is allocated in stack
    bool del = false;
    Reg rd = alloc_transient_reg(Interval(i), {});
    auto spill_pos = get_or_alloc_spill_pos(r);
    if (inst_sink.size() > 0) {
      auto &x = inst_sink.back();
      if (auto x_ = dynamic_cast<LoadStoreInst *>(&*x)) {
        if (auto x__ = std::get_if<MemoryOperand>(&x_->mem);
            x_->op == arm::OpCode::StR && x_->rd == rd &&
            (*x__) == MemoryOperand(REG_SP, spill_pos + stack_offset)) {
          del = true;
        }
      }
    }
    if (del) {
      inst_sink.pop_back();
      delayed_store = {{r, rd}};
    } else {
      inst_sink.push_back(std::make_unique<LoadStoreInst>(
          OpCode::LdR, rd, MemoryOperand(REG_SP, spill_pos + stack_offset)));
    }
    disp_reg();
    LOG(TRACE) << "spill " << spill_pos << std::endl;
    r = rd;
    return;
  } else {
    // is temporary register
    auto live_interval = live_intervals.at(r);
    r = alloc_transient_reg(live_interval, r);
    disp_reg();
    LOG(TRACE) << "transient ";
    display_reg_name(LOG(TRACE), r);
    LOG(TRACE) << std::endl;
    return;
  }
}

void RegAllocator::replace_write(Reg &r, int i) {
  if (!is_virtual_register(r)) {
    // is physical register; mark as occupied
    force_free(r);
    active.insert({r, Interval(i, UINT32_MAX)});
    return;
  } else if (auto reg_map_r = reg_map.find(r); reg_map_r != reg_map.end()) {
    // This register is allocated with graph-coloring
    r = reg_map_r->second;
  } else if (auto spill_r = spill_positions.find(r);
             spill_r != spill_positions.end()) {
    // this register is allocated in stack
    Reg rd = alloc_transient_reg(Interval(i), {});
    int pos = get_or_alloc_spill_pos(r);

    bool del = false;
    if (inst_sink.size() > 0) {
      auto &x = inst_sink.back();
      if (auto x_ = dynamic_cast<LoadStoreInst *>(&*x)) {
        if (auto x__ = std::get_if<MemoryOperand>(&x_->mem);
            x_->op == arm::OpCode::StR &&
            (*x__) == MemoryOperand(REG_SP, pos + stack_offset)) {
          del = true;
        }
      }
    }
    if (!del) {
      inst_sink.push_back(std::make_unique<LoadStoreInst>(
          OpCode::StR, rd, MemoryOperand(REG_SP, pos + stack_offset)));
    }
    r = rd;
  } else {
    // Is temporary register
    // the register should already be written to or read from
    auto live_interval = live_intervals.at(r);
    r = alloc_transient_reg(live_interval, r);

    // throw new std::logic_error("Writing to transient register");
  }
}

void RegAllocator::force_free(Reg r) {
  if (auto x = active.find(r); x != active.end()) {
    for (auto y : active_reg_map) {
      if (y.second == r) {
        // Spill to stack
        int stack_pos = get_or_alloc_spill_pos(y.first);
        inst_sink.push_back(std::make_unique<LoadStoreInst>(
            OpCode::StR, r, MemoryOperand(REG_SP, stack_pos + stack_offset)));
        break;
      }
    }
    active.erase(x);
  }
}

Reg RegAllocator::make_space(Reg r, Interval i) {
  throw prelude::NotImplementedException();
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

    if (auto x = dynamic_cast<Arith3Inst *>(inst_)) {
      replace_read(x->r1, i);
      replace_read(x->r2, i);
      invalidate_read(i);
      inst_sink.push_back(std::move(f.inst[i]));
      replace_write(x->rd, i);
    } else if (auto x = dynamic_cast<Arith2Inst *>(inst_)) {
      if (x->op == arm::OpCode::Mov || x->op == arm::OpCode::Mvn) {
        replace_read(x->r2, i);
        invalidate_read(i);
        inst_sink.push_back(std::move(f.inst[i]));
        replace_write(x->r1, i);
      } else if (x->op == arm::OpCode::MovT) {
        replace_read(x->r2, i);
        replace_read(x->r1, i);
        invalidate_read(i);
        inst_sink.push_back(std::move(f.inst[i]));
        replace_write(x->r1, i);
      } else {
        replace_read(x->r1, i);
        replace_read(x->r2, i);
        invalidate_read(i);
        inst_sink.push_back(std::move(f.inst[i]));
      }
    } else if (auto x = dynamic_cast<LoadStoreInst *>(inst_)) {
      if (auto mem = std::get_if<MemoryOperand>(&x->mem)) {
        replace_read(*mem, i);
      }
      if (x->op == arm::OpCode::LdR) {
        invalidate_read(i);
        inst_sink.push_back(std::move(f.inst[i]));
        replace_write(x->rd, i);
      } else {
        // StR
        replace_read(x->rd, i);
        invalidate_read(i);
        inst_sink.push_back(std::move(f.inst[i]));
      }
    } else if (auto x = dynamic_cast<MultLoadStoreInst *>(inst_)) {
      throw prelude::NotImplementedException();
      if (x->op == arm::OpCode::LdM) {
        for (auto rd : x->rd) add_reg_write(rd, i);
      } else {
        // StM
        for (auto rd : x->rd) add_reg_read(rd, i);
      }
      invalidate_read(i);
      add_reg_read(x->rn, i);
    } else if (auto x = dynamic_cast<PushPopInst *>(inst_)) {
      // push pop only use gpr
      invalidate_read(i);
      inst_sink.push_back(std::move(f.inst[i]));
    } else if (auto x = dynamic_cast<LabelInst *>(inst_)) {
      invalidate_read(i);
      inst_sink.push_back(std::move(f.inst[i]));

      // HACK: If it's load_pc label, delay store once more
      if (x->label.find("_$ld_pc") == 0 && inst_sink.size() >= 2 &&
          dynamic_cast<LoadStoreInst *>(&**(inst_sink.end() - 2))) {
        std::swap(*(inst_sink.end() - 2), *(inst_sink.end() - 1));
      }
    } else if (auto x = dynamic_cast<BrInst *>(inst_)) {
      invalidate_read(i);
      if (x->op == arm::OpCode::Bl) {
        auto &label = x->l;
        int param_cnt = x->param_cnt;
        int reg_cnt = std::min(param_cnt, 4);
        for (int i = 0; i < reg_cnt; i++) active.erase(Reg(i));
        for (int i = reg_cnt; i < 4; i++) force_free(Reg(i));
        inst_sink.push_back(std::move(f.inst[i]));
        for (int i = reg_cnt; i < 4; i++) active.erase(Reg(i));
      } else {
        inst_sink.push_back(std::move(f.inst[i]));
      }

    } else {
      invalidate_read(i);
      inst_sink.push_back(std::move(f.inst[i]));
    }
    if (delayed_store) {
      // TODO: check if this is right
      auto [r, rd] = delayed_store.value();
      replace_write(r, i);
      delayed_store = {};
    }
  }
}

void RegAllocatePass::optimize_arm(
    arm::ArmCode &arm_code, std::map<std::string, std::any> &extra_data_repo) {
  for (auto &f : arm_code.functions) {
    optimize_func(*f, extra_data_repo);
  }
}

void RegAllocatePass::optimize_func(
    arm::Function &f, std::map<std::string, std::any> &extra_data_repo) {
  auto &var_mapping_data =
      std::any_cast<optimization::MirVariableToArmVRegType &>(
          extra_data_repo.at(optimization::MIR_VARIABLE_TO_ARM_VREG_DATA_NAME));

  auto &coloring_data = std::any_cast<
      std::unordered_map<std::string, std::shared_ptr<ColorMap>> &>(
      extra_data_repo.at("graph_color"));

  auto f_coloring_data = coloring_data.find(f.name);
  auto var_mapping = var_mapping_data.find(f.name);

  RegAllocator fal(f, *f_coloring_data->second, var_mapping->second);
  fal.alloc_regs();
}

}  // namespace backend::codegen
