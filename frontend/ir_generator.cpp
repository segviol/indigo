#include "ir_generator.hpp"

using namespace front::irGenerator;

std::vector<string> externalFuncName = {
    "getint",
    "getch",
    "getarray",
    "putint",
    "putch",
    "putarray",
    "putf",
    "starttime",
    "stoptime"
};

void irGenerator::outputInstructions(std::ostream& out)
{
    out << (string)">====== global_var ======<" << std::endl;
    for (auto i : _package.global_values)
    {
        out << i.first << (string)" : ";
        i.second.display(out);
        out << std::endl;
    }


    for (auto i : _funcNameToInstructions)
    {
        out << ">====== function name : " + i.first + "======<" << std::endl;
        out << ">====== vars : ======<" << std::endl;
        for (auto j : _package.functions.at(i.first).variables)
        {
            out << j.first << (string)" : ";
            j.second.display(out);
            out << std::endl;
        }

        out << ">====== instructions : ======<" << std::endl;

        for (auto j : i.second)
        {
            out << "  ";
            if (j.index() == 0)
            {
                get<0>(j)->display(out);
            }
            else if (j.index() == 1)
            {
                get<1>(j)->display(out);
            }
            else
            {
                out << "label " << get<2>(j)->_jumpLabelId;
            }
            out << std::endl;
        }
        out << std::endl;
    }
}

string irGenerator::getStringName(string str)
{
    return _stringNamePrefix + "_" + str;
}

string irGenerator::getConstName(string name, int id)
{
    return _constNamePrefix + "_" + std::to_string(id) + "_" + name;
}

string irGenerator::getTmpName(std::uint32_t id)
{
    return _tmpNamePrefix + "_" + std::to_string(id);
}

string irGenerator::getVarName(string name, std::uint32_t id)
{
    return _varNamePrefix + "_" + std::to_string(id);
}

std::uint32_t irGenerator::tmpNameToId(string name)
{
    return std::stoi(name.substr(3));
}

void irGenerator::insertLocalValue(string name, std::uint32_t id, Variable& variable)
{
    _package.functions.find(_funcStack.back())->second.variables.insert(std::pair(id, variable));
    _localValueNameToId[name] = id;
}

void irGenerator::insertFunc(shared_ptr<mir::inst::MirFunction> func)
{
    _package.functions.insert({
        func->name,
        mir::inst::MirFunction(func->name, func->type)
        });
}

irGenerator::irGenerator()
{
    _nowLabelId = 0;
    _nowtmpId = 0;
    _nowLocalValueId = 0;

    shared_ptr<mir::inst::MirFunction> func = shared_ptr<mir::inst::MirFunction>(new mir::inst::MirFunction(
       _GlobalInitFuncName,
        shared_ptr<FunctionTy>(new FunctionTy(SharedTyPtr(new VoidTy()), {}, false))
        ));
    insertFunc(func);
    _funcStack.push_back(func->name);
    
    for (string funcName : externalFuncName)
    {
        shared_ptr<mir::inst::MirFunction> func;
        shared_ptr<FunctionTy> type;

        if (funcName == "getint")
        {
           type = shared_ptr<FunctionTy>(
                new FunctionTy(SharedTyPtr(new IntTy()), {}, true)
            );
        }
        else if (funcName == "getch")
        {
            type = shared_ptr<FunctionTy>(
                new FunctionTy(SharedTyPtr(new IntTy()), {}, true)
            );
        }
        else if (funcName == "getarray")
        {
            type = shared_ptr<FunctionTy>(
                new FunctionTy(SharedTyPtr(new IntTy()), { SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy()))) }, true)
            );
        }
        else if (funcName == "putint")
        {
            type = shared_ptr<FunctionTy>(
                new FunctionTy(SharedTyPtr(new VoidTy()), {SharedTyPtr(new IntTy())}, true)
            );
        }
        else if (funcName == "putarray")
        {
            type = shared_ptr<FunctionTy>(
                new FunctionTy(
                    SharedTyPtr(new VoidTy()),
                    {
                        SharedTyPtr(new IntTy()),
                        SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy)))
                    },
                    true
                )
            );
        }
        else if (funcName == "putf")
        {
            type = shared_ptr<FunctionTy>(
                new FunctionTy(
                    SharedTyPtr(new VoidTy()),
                    {
                        SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy))),
                        SharedTyPtr(new RestParamTy())
                    },
                    true
                )
            );
        }
        else if (funcName == "starttime")
        {
            type = shared_ptr<FunctionTy>(new FunctionTy(SharedTyPtr(new VoidTy()), {}, true));
        }
        else if (funcName == "stoptime")
        {
            type = shared_ptr<FunctionTy>(new FunctionTy(SharedTyPtr(new VoidTy()), {}, true));
        }

        func = shared_ptr<mir::inst::MirFunction>(new mir::inst::MirFunction(funcName, type));

        if (funcName == "getarray")
        {
            func->variables[1] = Variable(SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy()))), true, false);
        }
        else if (funcName == "putint")
        {
            func->variables[1] = Variable(SharedTyPtr(new IntTy()), true, false);
        }
        else if (funcName == "putf")
        {
            func->variables[1] = Variable(SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy))), true, false);
            func->variables[2] = Variable(SharedTyPtr(new RestParamTy()), true, false);
        }

        if (funcName == "putf")
        {
            string trueName = "printf";
            insertFunc(func);
            func->name = trueName;
        }
        else if (funcName == "starttime")
        {
            string trueName = "_sysy_starttime";
            insertFunc(func);
            _package.functions.find(func->name)->second.variables = func->variables;
            func->name = trueName;
        }
        else if (funcName == "stoptime")
        {
            string trueName = "_sysy_stoptime";
            insertFunc(func);
            _package.functions.find(func->name)->second.variables = func->variables;
            func->name = trueName;
        }

        insertFunc(func);
        _package.functions.find(func->name)->second.variables = func->variables;
    }
}

LabelId irGenerator::getNewLabelId()
{
    return _nowLabelId++;
}


string irGenerator::getNewTmpValueName(TyKind kind)
{
    SharedTyPtr ty;
    string name;
    Variable variable;

    switch (kind)
    {
    case mir::types::TyKind::Int:
        ty = shared_ptr<IntTy>(new IntTy());
        break;
    case mir::types::TyKind::Ptr:
        ty = shared_ptr<PtrTy>(new PtrTy(shared_ptr<IntTy>(new IntTy())));
        break;
    default:
        break;
    }
    
    name = getTmpName(_nowLocalValueId);
    variable = Variable(ty, false, true);
    insertLocalValue(name, _nowLocalValueId, variable);
    _nowLocalValueId++;;
    return name;
}

void irGenerator::pushWhile(WhileLabels wl)
{
    _whileStack.push_back(wl);
}

WhileLabels irGenerator::checkWhile()
{
    return _whileStack.back();
}

void irGenerator::popWhile()
{
    _whileStack.pop_back();
}

void irGenerator::ir_op(LeftVal dest, RightVal op1, RightVal op2, mir::inst::Op op)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<Value> op1Value;
    shared_ptr<Value> op2Value;
    shared_ptr<mir::inst::OpInst> opInst;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    op1Value = rightValueToValue(op1);
    op2Value = rightValueToValue(op2);
     
    opInst = shared_ptr<mir::inst::OpInst>(new mir::inst::OpInst(*destVarId, *op1Value, *op2Value, op));

    _funcNameToInstructions[_funcStack.back()].push_back(opInst);
}

void irGenerator::ir_declare_string(string str)
{
    if (_package.global_values.count(getStringName(str)) == 0)
    {
        GlobalValue globalValue = GlobalValue(str);
        _package.global_values[getStringName(str)] = globalValue;
    }
}

void irGenerator::ir_declare_const(string name, std::uint32_t value, int id)
{
    GlobalValue globalValue = GlobalValue(value);
    _package.global_values[getConstName(name, id)] = globalValue;
}

void irGenerator::ir_declare_const(string name, std::vector<std::uint32_t> values, int id)
{
    GlobalValue globalValue = GlobalValue(values);
    _package.global_values[getConstName(name, id)] = globalValue;
}

void irGenerator::ir_declare_value(string name, symbol::SymbolKind kind, int id, int len)
{
    if (_funcStack.back() == _GlobalInitFuncName)
    {
        GlobalValue globalValue;
        if (len == 0)
        {
            globalValue = GlobalValue(0);
        }
        else
        {
            std::vector<std::uint32_t> inits;
            for (int i = 0; i < len; i++)
            {
                inits.push_back(0);
            }
            globalValue = GlobalValue(inits);
        }
        _package.global_values[getVarName(name, id)] = globalValue;
    }
    else
    {
        SharedTyPtr ty;
        switch (kind)
        {
        case front::symbol::SymbolKind::INT:
            ty = SharedTyPtr(new IntTy());
            break;
        case front::symbol::SymbolKind::Array:
            ty = SharedTyPtr(new ArrayTy(SharedTyPtr(new IntTy()), len));
            break;
        case front::symbol::SymbolKind::Ptr:
            ty = SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy())));
            break;
        default:
            break;
        }
        Variable variable(ty, true, false);
        insertLocalValue(getVarName(name, id), _nowLocalValueId, variable);
        _nowLocalValueId++;
    }
}

void irGenerator::ir_declare_param(string name, symbol::SymbolKind kind, int id)
{
    SharedTyPtr ty;

    switch (kind)
    {
    case front::symbol::SymbolKind::INT:
    {
        ir_declare_value(name, kind, id);
        ty = SharedTyPtr(new IntTy());
        break;
    }
    case front::symbol::SymbolKind::Ptr:
    {
        ir_declare_value(name, kind, id);
        ty = SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy())));
        break;
    }
    default:
        break;
    }
    _package.functions.find(_funcStack.back())->second.type->params.push_back(ty);
}

void irGenerator::ir_declare_function(string name, symbol::SymbolKind kind)
{
    shared_ptr<mir::inst::MirFunction> func;
    shared_ptr<FunctionTy> type;
    SharedTyPtr ret;
    std::vector<SharedTyPtr> params;

    switch (kind)
    {
    case front::symbol::SymbolKind::INT:
        ret = SharedTyPtr(new IntTy());
        break;
    case front::symbol::SymbolKind::VOID:
        ret = SharedTyPtr(new VoidTy());
        break;
    default:
        break;
    }
    type = shared_ptr<FunctionTy>(new FunctionTy(ret, params));
    func = shared_ptr<mir::inst::MirFunction>(new mir::inst::MirFunction(name, type));

    insertFunc(func);
    _funcStack.push_back(func->name);

    _package.functions.find(_funcStack.back())->second.variables.insert(
        std::pair(_VoidVarId, Variable(SharedTyPtr(new VoidTy()), false, false))
    );

    _nowLocalValueId = 1;
    _localValueNameToId.clear();
}

void irGenerator::ir_leave_function()
{
    _funcStack.pop_back();
}


void irGenerator::ir_ref(LeftVal dest, LeftVal src)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<std::variant<VarId, std::string>> val;
    LabelId id;
    shared_ptr<mir::inst::Inst> refInst;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    if ((id = nameToLabelId(get<1>(src))) >= 0)
    {
        val = shared_ptr<std::variant<VarId, std::string>>(new std::variant<VarId, std::string>(VarId(id)));
    }
    else
    {
        val = shared_ptr<std::variant<VarId, std::string>>(new std::variant<VarId, std::string>(get<1>(src)));
    }

    refInst = shared_ptr<mir::inst::RefInst>(new mir::inst::RefInst(*destVarId, *val));
     
    _funcNameToInstructions[_funcStack.back()].push_back(refInst);
}

void irGenerator::ir_offset(LeftVal dest, LeftVal ptr, RightVal offset)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<VarId> ptrVarId;
    shared_ptr<Value> offsetValue;
    shared_ptr<mir::inst::PtrOffsetInst> offInst;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    ptrVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(ptr)));
    offsetValue = rightValueToValue(offset);

    offInst = shared_ptr<mir::inst::PtrOffsetInst>(new mir::inst::PtrOffsetInst(*destVarId, *ptrVarId, *offsetValue));

    _funcNameToInstructions[_funcStack.back()].push_back(offInst);
}

void irGenerator::ir_assign(LeftVal dest, RightVal src)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<Value> srcValue;
    shared_ptr<mir::inst::AssignInst> assginInst;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    srcValue = rightValueToValue(src);

    assginInst = shared_ptr<mir::inst::AssignInst>(new mir::inst::AssignInst(*destVarId, *srcValue));

    _funcNameToInstructions[_funcStack.back()].push_back(assginInst);
}

void irGenerator::ir_load(LeftVal dest, RightVal src)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<Value> srcValue;
    shared_ptr<mir::inst::LoadInst> loadInt;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    srcValue = rightValueToValue(src);

    loadInt = shared_ptr<mir::inst::LoadInst>(new mir::inst::LoadInst(*srcValue, *destVarId));

    _funcNameToInstructions[_funcStack.back()].push_back(loadInt);
}

void irGenerator::ir_store(LeftVal dest, RightVal src)
{
    shared_ptr<VarId> destVarId;
    shared_ptr<Value> srcValue;
    shared_ptr<mir::inst::StoreInst> storeInst;

    destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(dest)));
    srcValue = rightValueToValue(src);

    storeInst = shared_ptr<mir::inst::StoreInst>(new mir::inst::StoreInst(*srcValue, *destVarId));

    _funcNameToInstructions[_funcStack.back()].push_back(storeInst);
}

void irGenerator::ir_function_call(string retName, symbol::SymbolKind kind, string funcName, std::vector<RightVal> params)
{
    shared_ptr<VarId> destVarId;
    std::vector<Value> paramValues;
    shared_ptr<mir::inst::CallInst> callInst;

    switch (kind)
    {
    case front::symbol::SymbolKind::INT:
        destVarId = shared_ptr<VarId>(new VarId(LeftValueToLabelId(retName)));
        break;
    case front::symbol::SymbolKind::VOID:
        destVarId = shared_ptr<VarId>(new VarId(_VoidVarId));
        break;
    default:
        break;
    }

    for (auto var : params)
    {
        paramValues.push_back(*rightValueToValue(var));
    }

    callInst = shared_ptr<mir::inst::CallInst>(new mir::inst::CallInst(*destVarId, funcName, paramValues));

    _funcNameToInstructions[_funcStack.back()].push_back(callInst);
}

void irGenerator::ir_jump(mir::inst::JumpInstructionKind kind, LabelId bbTrue, LabelId bbFalse,
    std::optional<string> condRetName, mir::inst::JumpKind jumpKind)
{
    std::optional<VarId> crn;
    shared_ptr<mir::inst::JumpInstruction> jumpInst;

    if (condRetName.has_value())
    {
        crn = VarId(LeftValueToLabelId(condRetName.value()));
    }
    else
    {
        crn = std::nullopt;
    }

    jumpInst = shared_ptr<mir::inst::JumpInstruction>(new mir::inst::JumpInstruction(kind, bbTrue, bbFalse, crn, jumpKind));

    _funcNameToInstructions[_funcStack.back()].push_back(jumpInst);
}

void irGenerator::ir_label(LabelId label)
{
    shared_ptr<JumpLabelId> jumpLabelId;

    jumpLabelId = shared_ptr<JumpLabelId>(new JumpLabelId(label));

    _funcNameToInstructions[_funcStack.back()].push_back(jumpLabelId);
}

LabelId irGenerator::LeftValueToLabelId(LeftVal leftVal)
{
    LabelId id;
    switch (leftVal.index())
    {
    case 0:
         id = get<LabelId>(leftVal);
        break;
    case 1:
        id = nameToLabelId(get<string>(leftVal));
        break;
    }
    return id;
}

shared_ptr<Value> irGenerator::rightValueToValue(RightVal& rightValue)
{
    shared_ptr<Value> value;

    switch (rightValue.index())
    {
    case 0:
        value = shared_ptr<Value>(new Value(get<0>(rightValue)));
        break;
    case 2:
    {
        value = shared_ptr<Value>(new Value(VarId(nameToLabelId(get<2>(rightValue)))));
    }
        break;
    default:
        break;
    }

    return value;
}


LabelId irGenerator::nameToLabelId(string name)
{
    LabelId id = -1;
    if (_localValueNameToId.count(name) == 1)
    {
        id = _localValueNameToId[name];
    }
    return id;
}