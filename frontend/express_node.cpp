#include "express_node.hpp"

using namespace front::express;

ExpressNode::ExpressNode()
{
    _type = NodeType::CONST;
    _operation = OperationType::AND;
    _value = 0;
    _name = "";
}

bool ExpressNode::isBinaryOperation()
{
    return _operation == OperationType::PLUS || _operation == OperationType::MINU || _operation == OperationType::MUL
        || _operation == OperationType::DIV || _operation == OperationType::MOD || _operation == OperationType::OR
        || _operation == OperationType::AND || _operation == OperationType::EQL || _operation == OperationType::NEQ
        || _operation == OperationType::LSS || _operation == OperationType::LEQ || _operation == OperationType::GEQ
        || _operation == OperationType::GRE;
}

bool ExpressNode::isUnaryOperation()
{
    return _operation == OperationType::UN_PLUS || _operation == OperationType::UN_MINU || _operation == OperationType::UN_NOT;
}

void ExpressNode::addChild(SharedExNdPtr child)
{
    _children.push_back(child);
}