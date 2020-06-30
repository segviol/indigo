#pragma once

#include <string>
#include <vector>
#include <memory>

#ifndef COMPILER_FRONT_EXPRESS_NODE_H_
#define COMPILER_FRONT_EXPRESS_NODE_H_

namespace front::express
{
    using std::string;
    using std::vector;
    using std::shared_ptr;

    enum class NodeType
    {
        CONST,
        VAR
    };

    enum class OperationType
    {
        RETURN_FUNC_CALL,

        VAR,
        ARR,
        ARR_NAME,
        NUMBER,

        PLUS,
        MINU,
        MUL,
        DIV,
        MOD,
        OR,
        AND,
        EQL,
        NEQ,
        LSS,
        LEQ,
        GEQ,
        GRE,

        UN_PLUS,
        UN_MINU,
        UN_NOT
    };

    class ExpressNode
    {
    public:
        NodeType _type;
        OperationType _operation;
        int _value;
        string _name;
        vector<shared_ptr<ExpressNode>> _children;

        ExpressNode();

        bool isBinaryOperation();
        bool isUnaryOperation();

        void addChild(shared_ptr<ExpressNode> child);
    };

    typedef shared_ptr<ExpressNode> SharedExNdPtr;
}

#endif // !COMPILER_FRONT_EXPRESS_NODE_H_
