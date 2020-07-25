#include <memory>
#include <string>
#include <vector>

#ifndef COMPILER_FRONT_EXPRESS_NODE_H_
#define COMPILER_FRONT_EXPRESS_NODE_H_

namespace front::express {
using std::shared_ptr;
using std::string;
using std::vector;

enum class NodeType { CNS, VAR };

enum class OperationType {
  RETURN_FUNC_CALL,
  VOID_FUNC_CALL,

  VAR,
  ARR,
  NUMBER,
  STRING,
  PTR,

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
  UN_NOT,

  ASSIGN,
  LOAD
};

class ExpressNode {
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
} // namespace front::express

#endif // !COMPILER_FRONT_EXPRESS_NODE_H_
