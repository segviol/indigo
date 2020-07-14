#include <stdint.h>
#include <vector>
#include <algorithm>

#include "../arm_code/arm.hpp"
#include "../mir/mir.hpp"
#include "symbol.hpp"

#ifndef COMPILER_FRONT_IR_GENERATOR_H_
#define COMPILER_FRONT_IR_GENERATOR_H_

namespace front::irGenerator {
using std::get;
using std::move;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::variant;

using mir::types::ArrayTy;
using mir::types::FunctionTy;
using mir::types::IntTy;
using mir::types::LabelId;
using mir::types::PtrTy;
using mir::types::RestParamTy;
using mir::types::SharedTyPtr;
using mir::types::TyKind;
using mir::types::VoidTy;

using mir::inst::Value;
using mir::inst::Variable;
using mir::inst::VarId;

using arm::ConstValue;
typedef ConstValue GlobalValue;

typedef variant<int, LabelId, string> RightVal;
typedef variant<LabelId, string> LeftVal;

class WhileLabels {
public:
  LabelId _beginLabel;
  LabelId _endLabel;

  WhileLabels() {
    _beginLabel = 0;
    _endLabel = 0;
  }
  WhileLabels(LabelId beginLabel, LabelId endLabel)
      : _beginLabel(beginLabel), _endLabel(endLabel) {}
};

class FunctionData {
public:
    std::map<string, LabelId> _localValueNameToId;
    std::uint32_t _nowTmpId;
    std::uint32_t _nowLocalValueId;
};

class JumpLabelId {
public:
  LabelId _jumpLabelId;

  JumpLabelId(LabelId id) : _jumpLabelId(id) {}
};

typedef std::variant<shared_ptr<mir::inst::Inst>,
                     shared_ptr<mir::inst::JumpInstruction>,
                     shared_ptr<JumpLabelId>>
    Instruction;

extern std::vector<string> externalFuncName;

class irGenerator {
public:
  irGenerator();

  void outputInstructions(std::ostream &out);

  LabelId getNewLabelId();
  // as of now, the tmp for global scope is inserted into global init
  // function.variables
  string getNewTmpValueName(TyKind kind);

  void ir_declare_value(string name, symbol::SymbolKind kind, int id,
                        int len = 0);
  void ir_declare_string(string str);
  void ir_declare_const(string name, std::uint32_t value, int id);
  void ir_declare_const(string name, std::vector<std::uint32_t> values, int id);
  void ir_declare_function(string _name, symbol::SymbolKind kind);
  void ir_leave_function();
  void ir_declare_param(string name, symbol::SymbolKind kind, int id);
  void ir_finish_param_declare(std::vector<string>& paramsName);
  void ir_end_of_program();
  void ir_begin_of_program();

  /* src type mean (src.index())
   * 0: jumplabel
   * 1: {global_values, local_values}
   */
  void ir_ref(LeftVal dest, LeftVal src);
  void ir_offset(LeftVal dest, LeftVal ptr, RightVal offset);
  void ir_load(LeftVal dest, RightVal src);
  void ir_store(LeftVal dest, RightVal src);
  void ir_op(LeftVal dest, RightVal op1, RightVal op2, mir::inst::Op op);
  void ir_assign(LeftVal dest, RightVal src);
  void ir_assign(LeftVal dest, uint32_t sourceId);
  void ir_function_call(string retName, symbol::SymbolKind kind,
                        string funcName, std::vector<RightVal> params);
  void ir_jump(mir::inst::JumpInstructionKind kind, LabelId bbTrue,
               LabelId bbFalse, std::optional<string> condRetName,
               mir::inst::JumpKind jumpKind);
  void ir_label(LabelId label);

  void pushWhile(WhileLabels wl);
  WhileLabels checkWhile();
  void popWhile();

  string getStringName(string str);
  string getConstName(string name, int id);
  string getTmpName(std::uint32_t id);
  string getVarName(string name, std::uint32_t id);
  string getFunctionName(string name);

  std::map<string, std::vector<Instruction>> &getfuncNameToInstructions() {
    return _funcNameToInstructions;
  }

  mir::inst::MirPackage &getPackage() { return _package; }

private:
  mir::inst::MirPackage _package;
  std::map<string, std::vector<Instruction>> _funcNameToInstructions;

  const LabelId _InitLocalVarId = 1;
  const LabelId _InitTmpVarId = (1 << 15);
  const LabelId _VoidVarId = (1 << 20);
  const LabelId _ReturnBlockLabelId = (1 << 20);
  const LabelId _ReturnVarId = 0;

  const string _GlobalInitFuncName = "main";
  const string _VoidVarName = "$$_Compiler_Void_Var_Name_$$";
  const string _ReturnVarName = "$$__Compiler_Retuen_Var_Name__$$";
  const string _StringNamePrefix = "$$0";
  const string _TmpNamePrefix = "$$1";
  const string _ConstNamePrefix = "$$2";
  const string _VarNamePrefix = "$$3";
  const string _GenSaveParamVarNamePrefix = "$$4";
  const string _FunctionNamePrefix = "$$5";


  LabelId _nowLabelId;

  std::vector<WhileLabels> _whileStack;
  std::vector<string> _funcStack;
  std::map<string, FunctionData> _funcNameToFuncData;

  // name of {local value, global value}
  LabelId nameToLabelId(string name);
  LabelId LeftValueToLabelId(LeftVal leftVal);
  // insure the rightvalue is {number, local value}
  shared_ptr<Value> rightValueToValue(RightVal &rightValue);

  void insertFunc(string key, shared_ptr<mir::inst::MirFunction> func);
  void insertLocalValue(string name, std::uint32_t id, Variable &variable);
  void changeLocalValueId(std::uint32_t destId, std::uint32_t sourceId, string name);

  string getGenSaveParamVarName(uint32_t id);

};
} // namespace front::irGenerator

#endif // !COMPILER_FRONT_IR_GENERATOR_H_
