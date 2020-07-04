#include <vector>
#include <stdint.h>

#include "../mir/mir.hpp"
#include "../arm_code/arm.hpp"
#include "symbol.hpp"

#ifndef COMPILER_FRONT_IR_GENERATOR_H_
#define COMPILER_FRONT_IR_GENERATOR_H_

namespace front::irGenerator
{
    using std::string;
    using std::get;
    using std::variant;
    using std::shared_ptr;
    using std::unique_ptr;
    using std::move;

    using mir::types::SharedTyPtr;
    using mir::types::LabelId;
    using mir::types::IntTy;
    using mir::types::VoidTy;
    using mir::types::ArrayTy;
    using mir::types::PtrTy;
    using mir::types::FunctionTy;
    using mir::types::RestParamTy;
    using mir::types::TyKind;

    using mir::inst::Variable;
    using mir::inst::VarId;
    using mir::inst::Value;

    using arm::ConstValue;
    typedef ConstValue GlobalValue;

    typedef variant<int, LabelId, string> RightVal;
    typedef variant<LabelId, string> LeftVal;

    class WhileLabels
    {
    public:
        LabelId _beginLabel;
        LabelId _endLabel;

        WhileLabels()
        {
            _beginLabel = 0;
            _endLabel = 0;
        }
        WhileLabels(LabelId beginLabel, LabelId endLabel)
            : _beginLabel(beginLabel), _endLabel(endLabel) {}
    };

    class JumpLabelId
    {
    public:
        LabelId _jumpLabelId;

        JumpLabelId(LabelId id)
            : _jumpLabelId(id) {}
    };

    typedef std::variant<shared_ptr<mir::inst::Inst>, shared_ptr<mir::inst::JumpInstruction>, shared_ptr<JumpLabelId>> Instruction;

    class irGenerator
    {
    public:
        irGenerator();

        LabelId getNewLabelId();
        // as of now, the tmp for global scope is inserted into global_values of _package
        string getNewTmpValueName(TyKind kind);

        void ir_declare_value(string name, symbol::SymbolKind kind, int len = 0);
        //set "0" + str as str name in globalNameToId, the prefix is 0
        void ir_declare_string(string str);
        void ir_declare_const(string name, std::uint32_t value, int id);
        void ir_declare_const(string name, std::vector<std::uint32_t> values, int id);
        void ir_declare_function(string name, symbol::SymbolKind kind);
        void ir_leave_function();
        void ir_declare_param(string name, symbol::SymbolKind kind);

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
        void ir_function_call(string retName, symbol::SymbolKind kind, string funcName, std::vector<RightVal> params);
        void ir_jump(mir::inst::JumpInstructionKind kind, LabelId bbTrue, LabelId bbFalse, 
            std::optional<string> condRetName, mir::inst::JumpKind jumpKind);
        void ir_label(LabelId label);

        void pushWhile(WhileLabels wl);
        WhileLabels checkWhile();
        void popWhile();

        string getStringName(string str);
        string getConstName(string name, int id);
        string getTmpName(std::uint32_t id);
        std::uint32_t tmpNameToId(string name);

    private:
        mir::inst::MirPackage _package;
        std::map<string, std::vector<Instruction>> _funcNameToInstructions;

        const LabelId _VoidVarId = (1 << 20);

        const string _GlobalInitFuncName = "@@__Compiler__GlobalInitFunc__Auto__Generated__@@";
        const string _stringNamePrefix = "@@0";
        const string _tmpNamePrefix = "@@1";
        const string _constNamePrefix = "@@2";

        std::map<string, LabelId> _localValueNameToId;

        LabelId _nowLabelId;
        std::uint32_t _nowtmpId;
        std::uint32_t _nowLocalValueId;

        std::vector<WhileLabels> _whileStack;
        std::vector<string> _funcStack;

        // name of {local value, global value}
        LabelId nameToLabelId(string name);
        LabelId LeftValueToLabelId(LeftVal leftVal);
        // insure the rightvalue is {number, local value}
        shared_ptr<Value> rightValueToValue(RightVal& rightValue);

        void insertFunc(shared_ptr<mir::inst::MirFunction> func);
    };
}

#endif // !COMPILER_FRONT_IR_GENERATOR_H_
