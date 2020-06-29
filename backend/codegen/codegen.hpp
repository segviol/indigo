#pragma once
#include <sstream>

#include "../../arm_code/arm.hpp"
#include "../../prelude/prelude.hpp"
#include "../backend.hpp"

namespace backend::codegen {

std::string format_label(std::string_view function_name, uint32_t label_id) {
  auto s = std::stringstream();
  s << "_CONST_" << function_name << "__" << label_id;
  return s.str();
}

class Codegen final {
 public:
  Codegen(mir::inst::MirFunction& func,

          std::map<std::string, std::any>& extra_data)
      : func(func), extra_data(extra_data), inst() {}

  /// Translate MirPackage into ArmCode
  arm::Function translate_function();

 private:
  mir::inst::MirFunction& func;
  std::map<std::string, std::any>& extra_data;

  std::vector<std::unique_ptr<arm::Inst>> inst;
  std::map<mir::inst::VarId, arm::Reg> reg_map;
  std::map<arm::Label, arm::ConstValue> consts;

  uint32_t vreg_gp_counter = 0;
  uint32_t vreg_vd_counter = 0;
  uint32_t vreg_vq_counter = 0;
  uint32_t const_counter = 0;

  arm::Reg get_or_alloc_vgp(mir::inst::VarId v);
  arm::Reg get_or_alloc_vd(mir::inst::VarId v);
  arm::Reg get_or_alloc_vq(mir::inst::VarId v);
  arm::Reg alloc_vgp();
  arm::Reg alloc_vd();
  arm::Reg alloc_vq();

  arm::Operand2 translate_value_to_operand2(mir::inst::Value& v);
  arm::Reg translate_value_to_reg(mir::inst::Value& v);
  arm::Reg translate_var_reg(mir::inst::VarId v);

  // arm::Function translate_function(mir::inst::MirFunction& f);
  void translate_basic_block(mir::inst::BasicBlk& blk);
  void translate_inst(mir::inst::AssignInst& i);
  void translate_inst(mir::inst::PhiInst& i);
  void translate_inst(mir::inst::CallInst& i);
  void translate_inst(mir::inst::StoreInst& i);
  void translate_inst(mir::inst::LoadInst& i);
  void translate_inst(mir::inst::RefInst& i);
  void translate_inst(mir::inst::PtrOffsetInst& i);
  void translate_inst(mir::inst::OpInst& i);

  void emit_compare(mir::inst::VarId& dest, mir::inst::Value& lhs,
                    mir::inst::Value& rhs, arm::ConditionCode cond,
                    bool reversed);
};
}  // namespace backend::codegen
