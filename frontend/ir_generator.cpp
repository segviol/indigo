#include "ir_generator.hpp"

using namespace front::irGenerator;

std::vector<string> front::irGenerator::externalFuncName = {
    "getint", "getch",     "getarray", "putint", "putch", "putarray",
    "putf",   "starttime", "stoptime", "malloc", "free"};

void irGenerator::outputInstructions(std::ostream &out) {
  out << (string) ">====== global_var ======<" << std::endl;
  for (auto i : _package.global_values) {
    out << i.first << (string) " : ";
    i.second.display(out);
    out << std::endl;
  }

  for (auto i : _funcNameToInstructions) {
    out << ">====== function name : " + i.first + "======<" << std::endl;
    out << ">====== vars : ======<" << std::endl;
    for (auto j : _package.functions.at(i.first).variables) {
      out << j.first << (string) " : ";
      j.second.display(out);
      out << std::endl;
    }

    out << ">====== instructions : ======<" << std::endl;

    for (auto j : i.second) {
      out << "  ";
      if (j.index() == 0) {
        get<0>(j)->display(out);
      } else if (j.index() == 1) {
        get<1>(j)->display(out);
      } else {
        out << "label " << get<2>(j)->_jumpLabelId;
      }
      out << std::endl;
    }
    out << std::endl;
  }
}

string irGenerator::getStringName(string str) {
  return _StringNamePrefix + "_" + str;
}

string irGenerator::getConstName(string name, int id) {
  return _ConstNamePrefix + "_" + std::to_string(id) + "_" + name;
}

string irGenerator::getTmpName(std::uint32_t id) {
  return _TmpNamePrefix + "_" + std::to_string(id);
}

string irGenerator::getVarName(string name, std::uint32_t id) {
  return _VarNamePrefix + "_" + std::to_string(id);
}

string irGenerator::getGenSaveParamVarName(uint32_t id) {
  return _GenSaveParamVarNamePrefix + "_" + std::to_string(id);
}

string irGenerator::getFunctionName(string name) {
  if (find(externalFuncName.begin(), externalFuncName.end(), name) !=
      externalFuncName.end()) {
    return name;
  }
  return _FunctionNamePrefix + "_" + name;
}

void irGenerator::insertLocalValue(string name, std::uint32_t id,
                                   Variable &variable) {
  _package.functions.find(_funcStack.back())
      ->second.variables.insert(std::pair(id, variable));
  _funcNameToFuncData[_funcStack.back()]._localValueNameToId[name] = id;
}

void irGenerator::changeLocalValueId(std::uint32_t destId,
                                     std::uint32_t sourceId, string name) {
  Variable variable;

  _funcNameToFuncData[_funcStack.back()]._localValueNameToId.erase(name);
  variable = _package.functions.at(_funcStack.back()).variables.at(sourceId);
  _package.functions.at(_funcStack.back()).variables.erase(sourceId);

  insertLocalValue(name, destId, variable);
}

void irGenerator::insertFunc(string key,
                             shared_ptr<mir::inst::MirFunction> func) {
  _package.functions.insert(
      {key, mir::inst::MirFunction(func->name, func->type)});
}

void irGenerator::ir_begin_of_program() {
  for (string funcName : externalFuncName) {
    shared_ptr<mir::inst::MirFunction> func;
    shared_ptr<FunctionTy> type;

    if (funcName == "getint") {
      type = shared_ptr<FunctionTy>(
          new FunctionTy(SharedTyPtr(new IntTy()), {}, true));
    } else if (funcName == "getch") {
      type = shared_ptr<FunctionTy>(
          new FunctionTy(SharedTyPtr(new IntTy()), {}, true));
    } else if (funcName == "getarray") {
      type = shared_ptr<FunctionTy>(new FunctionTy(
          SharedTyPtr(new IntTy()),
          {SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy())))}, true));
    } else if (funcName == "putint") {
      type = shared_ptr<FunctionTy>(new FunctionTy(
          SharedTyPtr(new VoidTy()), {SharedTyPtr(new IntTy())}, true));
    } else if (funcName == "putch") {
      type = shared_ptr<FunctionTy>(new FunctionTy(
          SharedTyPtr(new VoidTy()), {SharedTyPtr(new IntTy())}, true));
    } else if (funcName == "putarray") {
      type = shared_ptr<FunctionTy>(
          new FunctionTy(SharedTyPtr(new VoidTy()),
                         {SharedTyPtr(new IntTy()),
                          SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy)))},
                         true));
    } else if (funcName == "putf") {
      type = shared_ptr<FunctionTy>(
          new FunctionTy(SharedTyPtr(new VoidTy()),
                         {SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy))),
                          SharedTyPtr(new RestParamTy())},
                         true));
    } else if (funcName == "starttime") {
      type = shared_ptr<FunctionTy>(
          new FunctionTy(SharedTyPtr(new VoidTy()), {}, true));
    } else if (funcName == "stoptime") {
      type = shared_ptr<FunctionTy>(
          new FunctionTy(SharedTyPtr(new VoidTy()), {}, true));
    } else if (funcName == "malloc") {
      type = shared_ptr<FunctionTy>(
          new FunctionTy(SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy))),
                         {SharedTyPtr(new IntTy())}, true));
    } else if (funcName == "free") {
      type = shared_ptr<FunctionTy>(new FunctionTy(
          SharedTyPtr(new VoidTy()),
          {SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy)))}, true));
    }

    func = shared_ptr<mir::inst::MirFunction>(
        new mir::inst::MirFunction(funcName, type));

    if (funcName == "getarray") {
      func->variables[1] = Variable(
          SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy()))), true, false);
    } else if (funcName == "putint") {
      func->variables[1] = Variable(SharedTyPtr(new IntTy()), true, false);
    } else if (funcName == "putch") {
      func->variables[1] = Variable(SharedTyPtr(new IntTy()), true, false);
    } else if (funcName == "putarray") {
      func->variables[1] = Variable(SharedTyPtr(new IntTy()), true, false);
      func->variables[2] =
          Variable(SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy))), true, false);
    } else if (funcName == "putf") {
      func->variables[1] =
          Variable(SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy))), true, false);
      func->variables[2] =
          Variable(SharedTyPtr(new RestParamTy()), true, false);
    } else if (funcName == "malloc") {
      func->variables[1] = Variable(SharedTyPtr(new IntTy()), true, false);
    } else if (funcName == "free") {
      func->variables[1] =
          Variable(SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy))), true, false);
    }

    if (funcName == "putf") {
      string trueName = "printf";
      func->name = trueName;
      insertFunc(getFunctionName(trueName), func);
      _package.functions.find(getFunctionName(trueName))->second.variables =
          func->variables;
    } else if (funcName == "starttime") {
      string trueName = "_sysy_starttime";
      func->name = trueName;
      insertFunc(getFunctionName(trueName), func);
      _package.functions.find(getFunctionName(trueName))->second.variables =
          func->variables;
    } else if (funcName == "stoptime") {
      string trueName = "_sysy_stoptime";
      func->name = trueName;
      insertFunc(getFunctionName(trueName), func);
      _package.functions.find(getFunctionName(trueName))->second.variables =
          func->variables;
    }

    insertFunc(getFunctionName(funcName), func);
    _package.functions.find(getFunctionName(funcName))->second.variables =
        func->variables;
  }

  _nowLabelId = 0;

  ir_declare_function(_GlobalInitFuncName, symbol::SymbolKind::INT);
}

irGenerator::irGenerator() {}

LabelId irGenerator::getNewLabelId() { return _nowLabelId++; }

string irGenerator::getNewTmpValueName(TyKind kind) {
  SharedTyPtr ty;
  string name;
  Variable variable;

  switch (kind) {
  case mir::types::TyKind::Int:
    ty = shared_ptr<IntTy>(new IntTy());
    break;
  case mir::types::TyKind::Ptr:
    ty = shared_ptr<PtrTy>(new PtrTy(shared_ptr<IntTy>(new IntTy())));
    break;
  default:
    break;
  }

  name = getTmpName(_funcNameToFuncData[_funcStack.back()]._nowTmpId);
  variable = Variable(ty, false, true);
  insertLocalValue(name, _funcNameToFuncData[_funcStack.back()]._nowTmpId,
                   variable);
  _funcNameToFuncData[_funcStack.back()]._nowTmpId++;
  ;
  return name;
}

void irGenerator::pushWhile(WhileLabels wl) { _whileStack.push_back(wl); }

WhileLabels irGenerator::checkWhile() { return _whileStack.back(); }

void irGenerator::popWhile() { _whileStack.pop_back(); }

void irGenerator::ir_op(LeftVal dest, RightVal op1, RightVal op2,
                        mir::inst::Op op) {
  shared_ptr<VarId> destVarId;
  shared_ptr<Value> op1Value;
  shared_ptr<Value> op2Value;
  shared_ptr<mir::inst::OpInst> opInst;

  destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
  op1Value = rightValueToValue(op1);
  op2Value = rightValueToValue(op2);

  opInst = shared_ptr<mir::inst::OpInst>(
      new mir::inst::OpInst(*destVarId, *op1Value, *op2Value, op));

  _funcNameToInstructions[_funcStack.back()].push_back(opInst);
}

void irGenerator::ir_declare_string(string str) {
  if (_package.global_values.count(getStringName(str)) == 0) {
    GlobalValue globalValue = GlobalValue(str);
    _package.global_values[getStringName(str)] = globalValue;
  }
}

void irGenerator::ir_declare_const(string name, std::uint32_t value, int id) {
  GlobalValue globalValue = GlobalValue(value);
  _package.global_values[getConstName(name, id)] = globalValue;
}

void irGenerator::ir_declare_const(string name,
                                   std::vector<std::uint32_t> values, int id) {
  GlobalValue globalValue = GlobalValue(values);
  _package.global_values[getConstName(name, id)] = globalValue;
}

void irGenerator::ir_declare_value(string name, symbol::SymbolKind kind, int id,
                                   int len) {
  if (_funcStack.back() == _GlobalInitFuncName) {
    GlobalValue globalValue;
    if (len == 0) {
      globalValue = GlobalValue(0);
    } else {
      std::vector<std::uint32_t> inits;
      for (int i = 0; i < len; i++) {
        inits.push_back(0);
      }
      globalValue = GlobalValue(inits);
    }
    _package.global_values[getVarName(name, id)] = globalValue;
  } else {
    SharedTyPtr ty;
    bool is_memory;
    string varName;

    switch (kind) {
    case front::symbol::SymbolKind::INT:
      ty = SharedTyPtr(new IntTy());
      is_memory = false;
      break;
    case front::symbol::SymbolKind::Array:
      ty = SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy())));
      is_memory = false;
      break;
    case front::symbol::SymbolKind::Ptr:
      ty = SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy())));
      is_memory = false;
      break;
    default:
      break;
    }
    Variable variable(ty, is_memory, false);
    varName = getVarName(name, id);

    insertLocalValue(varName,
                     _funcNameToFuncData[_funcStack.back()]._nowLocalValueId,
                     variable);
    _funcNameToFuncData[_funcStack.back()]._nowLocalValueId++;

    if (kind == symbol::SymbolKind::Array) {
      RightVal right;
      right.emplace<0>(len);
      ir_function_call(varName, symbol::SymbolKind::Ptr, "malloc", {right});
      _funcNameToFuncData[_funcStack.back()]._freeList.push_back(varName);
    }
  }
}

void irGenerator::ir_declare_param(string name, symbol::SymbolKind kind,
                                   int id) {
  SharedTyPtr ty;

  ir_declare_value(name, kind, id);

  switch (kind) {
  case front::symbol::SymbolKind::INT: {
    ty = SharedTyPtr(new IntTy());
    break;
  }
  case front::symbol::SymbolKind::Ptr: {
    ty = SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy())));
    break;
  }
  default:
    break;
  }
  _package.functions.find(_funcStack.back())->second.type->params.push_back(ty);
}

void irGenerator::ir_finish_param_declare(std::vector<string> &paramsName) {
  std::vector<SharedTyPtr> &params =
      _package.functions.at(_funcStack.back()).type->params;

  for (int i = 0; i < params.size(); i++) {
    Variable &variable =
        _package.functions.at(_funcStack.back()).variables.at(i + 1);
    Variable saveVar(variable.ty, variable.is_memory_var, variable.is_temp_var);
    insertLocalValue(getGenSaveParamVarName(i + 1),
                     _funcNameToFuncData[_funcStack.back()]._nowLocalValueId++,
                     saveVar);
    ir_assign(getGenSaveParamVarName(i + 1), i + 1);
    _funcNameToFuncData[_funcStack.back()]
        ._localValueNameToId[paramsName.at(i)] =
        _funcNameToFuncData[_funcStack.back()]
            ._localValueNameToId[getGenSaveParamVarName(i + 1)];
  }
}

void irGenerator::ir_declare_function(string _name, symbol::SymbolKind kind) {
  string name;
  shared_ptr<mir::inst::MirFunction> func;
  shared_ptr<FunctionTy> type;
  SharedTyPtr ret;
  std::vector<SharedTyPtr> params;
  FunctionData functionData;

  functionData._nowLocalValueId = _InitLocalVarId;
  functionData._nowTmpId = _InitTmpVarId;
  _funcNameToFuncData[_name] = functionData;

  switch (kind) {
  case front::symbol::SymbolKind::INT:
    ret = SharedTyPtr(new IntTy());
    break;
  case front::symbol::SymbolKind::VOID:
    ret = SharedTyPtr(new VoidTy());
    break;
  default:
    break;
  }
  name = _name;
  type = shared_ptr<FunctionTy>(new FunctionTy(ret, params, false));
  func = shared_ptr<mir::inst::MirFunction>(
      new mir::inst::MirFunction(name, type));

  insertFunc(name, func);
  _funcStack.push_back(func->name);

  _package.functions.find(_funcStack.back())
      ->second.variables.insert(std::pair(
          _VoidVarId, Variable(SharedTyPtr(new VoidTy()), false, false)));
  _funcNameToFuncData[_funcStack.back()]._localValueNameToId[_VoidVarName] =
      _VoidVarId;
  if (kind == front::symbol::SymbolKind::INT) {
    _package.functions.find(_funcStack.back())
        ->second.variables.insert(std::pair(
            _ReturnVarId, Variable(SharedTyPtr(new IntTy()), false, false)));
    _funcNameToFuncData[_funcStack.back()]._localValueNameToId[_ReturnVarName] =
        _ReturnVarId;
  }
  _funcNameToInstructions[_funcStack.back()].push_back(
      shared_ptr<JumpLabelId>(new JumpLabelId(getNewLabelId())));
}

void irGenerator::ir_leave_function() {
  shared_ptr<mir::inst::JumpInstruction> jumpInst;

  ir_label(_ReturnBlockLabelId);

  for (string var : _funcNameToFuncData[_funcStack.back()]._freeList) {
    ir_function_call("void", symbol::SymbolKind::VOID, "free", {var});
  }

  switch (_package.functions.at(_funcStack.back()).type->ret->kind()) {
  case mir::types::TyKind::Int: {
    jumpInst =
        shared_ptr<mir::inst::JumpInstruction>(new mir::inst::JumpInstruction(
            mir::inst::JumpInstructionKind::Return, -1, -1, VarId(_ReturnVarId),
            mir::inst::JumpKind::Undefined));
    break;
  }
  case mir::types::TyKind::Void: {
    jumpInst =
        shared_ptr<mir::inst::JumpInstruction>(new mir::inst::JumpInstruction(
            mir::inst::JumpInstructionKind::Return, -1, -1, std::nullopt,
            mir::inst::JumpKind::Undefined));
    break;
  }
  default:
    break;
  }
  _funcNameToInstructions[_funcStack.back()].push_back(jumpInst);
  _funcStack.pop_back();
}

void irGenerator::ir_ref(LeftVal dest, LeftVal src) {
  shared_ptr<VarId> destVarId;
  shared_ptr<std::variant<VarId, std::string>> val;
  LabelId id;
  shared_ptr<mir::inst::Inst> refInst;

  destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
  if ((id = nameToLabelId(get<1>(src))) >= 0) {
    val = shared_ptr<std::variant<VarId, std::string>>(
        new std::variant<VarId, std::string>(VarId(id)));
  } else {
    val = shared_ptr<std::variant<VarId, std::string>>(
        new std::variant<VarId, std::string>(get<1>(src)));
  }

  refInst =
      shared_ptr<mir::inst::RefInst>(new mir::inst::RefInst(*destVarId, *val));

  _funcNameToInstructions[_funcStack.back()].push_back(refInst);
}

void irGenerator::ir_offset(LeftVal dest, LeftVal ptr, RightVal offset) {
  shared_ptr<VarId> destVarId;
  shared_ptr<VarId> ptrVarId;
  shared_ptr<Value> offsetValue;
  shared_ptr<mir::inst::PtrOffsetInst> offInst;

  destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
  ptrVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(ptr)));
  offsetValue = rightValueToValue(offset);

  offInst = shared_ptr<mir::inst::PtrOffsetInst>(
      new mir::inst::PtrOffsetInst(*destVarId, *ptrVarId, *offsetValue));

  _funcNameToInstructions[_funcStack.back()].push_back(offInst);
}

void irGenerator::ir_assign(LeftVal dest, RightVal src) {
  shared_ptr<VarId> destVarId;
  shared_ptr<Value> srcValue;
  shared_ptr<mir::inst::AssignInst> assginInst;

  destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
  srcValue = rightValueToValue(src);

  assginInst = shared_ptr<mir::inst::AssignInst>(
      new mir::inst::AssignInst(*destVarId, *srcValue));

  _funcNameToInstructions[_funcStack.back()].push_back(assginInst);
}

void irGenerator::ir_assign(LeftVal dest, uint32_t sourceId) {
  shared_ptr<VarId> destVarId;
  shared_ptr<Value> srcValue;
  shared_ptr<mir::inst::AssignInst> assginInst;

  destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
  srcValue = shared_ptr<Value>(new Value(VarId(sourceId)));

  assginInst = shared_ptr<mir::inst::AssignInst>(
      new mir::inst::AssignInst(*destVarId, *srcValue));

  _funcNameToInstructions[_funcStack.back()].push_back(assginInst);
}

void irGenerator::ir_load(LeftVal dest, RightVal src) {
  shared_ptr<VarId> destVarId;
  shared_ptr<Value> srcValue;
  shared_ptr<mir::inst::LoadInst> loadInt;

  destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
  srcValue = rightValueToValue(src);

  loadInt = shared_ptr<mir::inst::LoadInst>(
      new mir::inst::LoadInst(*srcValue, *destVarId));

  _funcNameToInstructions[_funcStack.back()].push_back(loadInt);
}

void irGenerator::ir_store(LeftVal dest, RightVal src) {
  shared_ptr<VarId> destVarId;
  shared_ptr<Value> srcValue;
  shared_ptr<mir::inst::StoreInst> storeInst;

  destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
  srcValue = rightValueToValue(src);

  storeInst = shared_ptr<mir::inst::StoreInst>(
      new mir::inst::StoreInst(*srcValue, *destVarId));

  _funcNameToInstructions[_funcStack.back()].push_back(storeInst);
}

void irGenerator::ir_function_call(string retName, symbol::SymbolKind kind,
                                   string funcName,
                                   std::vector<RightVal> params) {
  shared_ptr<VarId> destVarId;
  std::vector<Value> paramValues;
  shared_ptr<mir::inst::CallInst> callInst;

  switch (kind) {
  case front::symbol::SymbolKind::Ptr:
  case front::symbol::SymbolKind::INT: {
    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(retName)));
    break;
  }

  case front::symbol::SymbolKind::VOID: {
    destVarId = shared_ptr<VarId>(new VarId(_VoidVarId));
    break;
  }
  default:
    break;
  }

  for (auto var : params) {
    paramValues.push_back(*rightValueToValue(var));
  }

  callInst = shared_ptr<mir::inst::CallInst>(
      new mir::inst::CallInst(*destVarId, funcName, paramValues));

  _funcNameToInstructions[_funcStack.back()].push_back(callInst);
}

void irGenerator::ir_end_of_program() {
  string returnTmp = getNewTmpValueName(mir::types::TyKind::Int);
  ir_function_call(returnTmp, front::symbol::SymbolKind::INT,
                   getFunctionName(_GlobalInitFuncName), {});
  ir_jump(mir::inst::JumpInstructionKind::Return, -1, -1, returnTmp,
          mir::inst::JumpKind::Undefined);
  ir_leave_function();
}

void irGenerator::ir_jump(mir::inst::JumpInstructionKind kind, LabelId bbTrue,
                          LabelId bbFalse, std::optional<string> condRetName,
                          mir::inst::JumpKind jumpKind) {
  std::optional<VarId> crn;
  shared_ptr<mir::inst::JumpInstruction> jumpInst;

  if (condRetName.has_value()) {
    crn = VarId(LeftValueToLabelId(condRetName.value()));
  } else {
    crn = std::nullopt;
  }

  if (kind == mir::inst::JumpInstructionKind::Return) {
    kind = mir::inst::JumpInstructionKind::Br;
    bbTrue = _ReturnBlockLabelId;
    bbFalse = -1;
    if (crn.has_value()) {
      ir_assign(_ReturnVarName, condRetName.value());
      crn.reset();
    }
    jumpKind = mir::inst::JumpKind::Undefined;
  }

  jumpInst = shared_ptr<mir::inst::JumpInstruction>(
      new mir::inst::JumpInstruction(kind, bbTrue, bbFalse, crn, jumpKind));

  _funcNameToInstructions[_funcStack.back()].push_back(jumpInst);
}

void irGenerator::ir_label(LabelId label) {
  shared_ptr<JumpLabelId> jumpLabelId;

  jumpLabelId = shared_ptr<JumpLabelId>(new JumpLabelId(label));

  _funcNameToInstructions[_funcStack.back()].push_back(jumpLabelId);
}

LabelId irGenerator::LeftValueToLabelId(LeftVal leftVal) {
  LabelId id;
  switch (leftVal.index()) {
  case 0:
    id = get<LabelId>(leftVal);
    break;
  case 1:
    id = nameToLabelId(get<string>(leftVal));
    break;
  }
  return id;
}

shared_ptr<Value> irGenerator::rightValueToValue(RightVal &rightValue) {
  shared_ptr<Value> value;

  switch (rightValue.index()) {
  case 0:
    value = shared_ptr<Value>(new Value(get<0>(rightValue)));
    break;
  case 2: {
    value =
        shared_ptr<Value>(new Value(VarId(nameToLabelId(get<2>(rightValue)))));
  } break;
  default:
    break;
  }

  return value;
}

LabelId irGenerator::nameToLabelId(string name) {
  LabelId id = -1;
  if (_funcNameToFuncData[_funcStack.back()]._localValueNameToId.count(name) ==
      1) {
    id = _funcNameToFuncData[_funcStack.back()]._localValueNameToId[name];
  }
  return id;
}
