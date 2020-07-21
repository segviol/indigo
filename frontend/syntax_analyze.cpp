#include "syntax_analyze.hpp"

#include "../include/aixlog.hpp"

using namespace front::syntax;

vector<Word> tmp_vect_word;

void SyntaxAnalyze::hp_init_external_function() {
  static bool inited = false;
  if (inited == false) {
    for (string funcName : irGenerator::externalFuncName) {
      if (funcName == "getint") {
        symbolTable.push_symbol(SharedSyPtr(
            new FunctionSymbol(funcName, SymbolKind::INT, _initLayerNum)));
      } else if (funcName == "getch") {
        symbolTable.push_symbol(SharedSyPtr(
            new FunctionSymbol(funcName, SymbolKind::INT, _initLayerNum)));
      } else if (funcName == "getarray") {
        symbolTable.push_symbol(SharedSyPtr(
            new FunctionSymbol(funcName, SymbolKind::INT, _initLayerNum)));
      } else if (funcName == "putint") {
        symbolTable.push_symbol(SharedSyPtr(
            new FunctionSymbol(funcName, SymbolKind::VOID, _initLayerNum)));
      } else if (funcName == "putch") {
        symbolTable.push_symbol(SharedSyPtr(
            new FunctionSymbol(funcName, SymbolKind::VOID, _initLayerNum)));
      } else if (funcName == "putarray") {
        symbolTable.push_symbol(SharedSyPtr(
            new FunctionSymbol(funcName, SymbolKind::VOID, _initLayerNum)));
      } else if (funcName == "putf") {
        symbolTable.push_symbol(SharedSyPtr(
            new FunctionSymbol(funcName, SymbolKind::VOID, _initLayerNum)));
      } else if (funcName == "starttime") {
        symbolTable.push_symbol(SharedSyPtr(
            new FunctionSymbol(funcName, SymbolKind::VOID, _initLayerNum)));
      } else if (funcName == "stoptime") {
        symbolTable.push_symbol(SharedSyPtr(
            new FunctionSymbol(funcName, SymbolKind::VOID, _initLayerNum)));
      }
    }
    inited = true;
  }
}

SyntaxAnalyze::SyntaxAnalyze() : matched_index(-1), word_list(tmp_vect_word) {}

SyntaxAnalyze::SyntaxAnalyze(vector<Word> &in_word_list)
    : matched_index(-1), word_list(in_word_list) {}

SyntaxAnalyze::~SyntaxAnalyze() {}

void SyntaxAnalyze::gm_comp_unit() {
  hp_init_external_function();

  irGenerator.ir_begin_of_program();

  while (matched_index + 1 < word_list.size()) {
    if (try_word(1, Token::INTTK, Token::VOIDTK) &&
        try_word(2, Token::IDENFR) && try_word(3, Token::LPARENT)) {
      gm_func_def();
    } else if (try_word(1, Token::CONSTTK)) {
      gm_const_decl();
    } else {
      gm_var_decl();
    }
  }

  irGenerator.ir_end_of_program();
}

void SyntaxAnalyze::gm_const_decl() {
  match_one_word(Token::CONSTTK);
  match_one_word(Token::INTTK);
  gm_const_def();
  while (try_word(1, Token::COMMA)) {
    match_one_word(Token::COMMA);
    gm_const_def();
  }
  match_one_word(Token::SEMICN);
}

void SyntaxAnalyze::gm_const_def() {
  string name;
  SymbolKind kind;
  SharedSyPtr symbol;
  SharedExNdPtr dimension;
  vector<SharedExNdPtr> init_values;
  vector<SharedExNdPtr> dimensions;
  vector<std::uint32_t> inits;

  match_one_word(Token::IDENFR);
  name = get_least_matched_word().get_self();

  if (try_word(1, Token::LBRACK)) {
    kind = SymbolKind::Array;
    while (try_word(1, Token::LBRACK)) {
      match_one_word(Token::LBRACK);
      dimension = gm_const_exp();
      match_one_word(Token::RBRACK);

      dimensions.push_back(dimension);
    }
  } else {
    kind = SymbolKind::INT;
  }

  match_one_word(Token::ASSIGN);

  if (try_word(1, Token::LBRACE)) {
    match_one_word(Token::LBRACE);
    gm_const_init_val(init_values, dimensions, 0);
    match_one_word(Token::RBRACE);
  } else {
    gm_const_init_val(init_values, dimensions, 0);
  }

  if (init_values.size() > 0) {
    for (auto i : init_values) {
      inits.push_back(i->_value);
    }
  }

  if (kind == SymbolKind::INT) {
    symbol.reset(
        new IntSymbol(name, layer_num, true, init_values.front()->_value));
    irGenerator.ir_declare_const(name, inits.front(), symbol->getId());
  } else {
    symbol.reset(new ArraySymbol(name, IntSymbol::getHolderIntSymbol(),
                                 layer_num, true, false));
    for (auto var : dimensions) {
      std::static_pointer_cast<ArraySymbol>(symbol)->addDimension(var);
    }
    irGenerator.ir_declare_const(name, inits, symbol->getId());
  }

  symbolTable.push_symbol(symbol);
}

// the caller will depackage one layer of brace before call this function
uint32_t SyntaxAnalyze::gm_const_init_val(vector<SharedExNdPtr> &init_values,
                                          vector<SharedExNdPtr> &dimensions,
                                          uint32_t braceLayerNum) {
  uint32_t totalValueNum;
  uint32_t nowValueNum;

  totalValueNum = 1;
  for (int i = braceLayerNum; i < dimensions.size(); i++) {
    totalValueNum *= dimensions.at(i)->_value;
  }
  nowValueNum = 0;

  while (!try_word(1, Token::RBRACE, Token::COMMA, Token::SEMICN)) {
    if (try_word(1, Token::LBRACE)) {
      match_one_word(Token::LBRACE);

      nowValueNum +=
          gm_const_init_val(init_values, dimensions, braceLayerNum + 1);

      match_one_word(Token::RBRACE);
    } else {
      nowValueNum++;
      SharedExNdPtr value;
      value = gm_const_exp();
      init_values.push_back(value);
      if (dimensions.size() == 0)
      {
          break;
      }
    }
    if (try_word(1, Token::COMMA)) {
      match_one_word(Token::COMMA);
    }
  }

  for (; nowValueNum < totalValueNum; nowValueNum++) {
    SharedExNdPtr value;
    value = SharedExNdPtr(new ExpressNode());
    value->_type = NodeType::CONST;
    value->_operation = OperationType::NUMBER;
    value->_value = 0;
    init_values.push_back(value);
  }

  return nowValueNum;
}

void SyntaxAnalyze::gm_var_decl() {
  match_one_word(Token::INTTK);
  gm_var_def();
  while (try_word(1, Token::COMMA)) {
    match_one_word(Token::COMMA);
    gm_var_def();
  }
  match_one_word(Token::SEMICN);
}

void SyntaxAnalyze::gm_var_def() {
  string name;
  SymbolKind kind;
  SharedExNdPtr dimension;
  vector<SharedExNdPtr> dimensions;
  vector<SharedExNdPtr> init_values;
  SharedSyPtr symbol;

  match_one_word(Token::IDENFR);
  name = get_least_matched_word().get_self();

  if (try_word(1, Token::LBRACK)) {
    kind = SymbolKind::Array;
    while (try_word(1, Token::LBRACK)) {
      match_one_word(Token::LBRACK);
      dimension = gm_const_exp();
      dimensions.push_back(dimension);
      match_one_word(Token::RBRACK);
    }
  } else {
    kind = SymbolKind::INT;
  }

  if (try_word(1, Token::ASSIGN)) {
    match_one_word(Token::ASSIGN);
    if (try_word(1, Token::LBRACE)) {
      match_one_word(Token::LBRACE);
      gm_init_val(init_values, dimensions, 0);
      match_one_word(Token::RBRACE);
    } else {
      gm_init_val(init_values, dimensions, 0);
    }
  }

  if (kind == SymbolKind::INT) {
    symbol.reset(new IntSymbol(name, layer_num, false));
    irGenerator.ir_declare_value(name, kind, symbol->getId());
  } else {
    symbol.reset(new ArraySymbol(name, IntSymbol::getHolderIntSymbol(),
                                 layer_num, false, false));
    for (auto var : dimensions) {
      std::static_pointer_cast<ArraySymbol>(symbol)->addDimension(var);
    }

    irGenerator.ir_declare_value(
        name, kind, symbol->getId(),
        std::static_pointer_cast<ArraySymbol>(symbol)->getLen());
  }

  if (init_values.size() > 0) {
    string valueName = irGenerator.getVarName(name, symbol->getId());
    irGenerator::RightVal rightVal;

    if (symbol->kind() == SymbolKind::Array) {
      string initPtr;
      int offset = 0;

      initPtr = irGenerator.getNewTmpValueName(TyKind::Ptr);

      if (inGlobalLayer()) {
        irGenerator.ir_ref(initPtr, valueName);
      } else {
        irGenerator.ir_assign(initPtr, valueName);
      }

      for (auto var : init_values) {
        if (offset > 0) {
          rightVal.emplace<0>(1);
          irGenerator.ir_offset(initPtr, initPtr, rightVal);
        }

        if (symbol->kind() == SymbolKind::Array) {
          std::static_pointer_cast<ArraySymbol>(symbol)->addValue(var);
        }

        if (var->_type == NodeType::CONST) {
          rightVal.emplace<0>(var->_value);
        } else {
          rightVal.emplace<2>(var->_name);
        }
        irGenerator.ir_store(initPtr, rightVal);
        offset++;
      }
    } else {
      SharedExNdPtr var = init_values.front();

      if (var->_type == NodeType::CONST) {
        rightVal.emplace<0>(var->_value);
      } else {
        rightVal.emplace<2>(var->_name);
      }

      if (inGlobalLayer()) {
        string addrTmp;

        addrTmp = irGenerator.getNewTmpValueName(TyKind::Ptr);
        irGenerator.ir_ref(addrTmp, valueName);
        irGenerator.ir_store(addrTmp, rightVal);
      } else {
        irGenerator.ir_assign(valueName, rightVal);
      }
    }
  }
  symbolTable.push_symbol(symbol);
}

uint32_t SyntaxAnalyze::gm_init_val(vector<SharedExNdPtr> &init_values,
                                    vector<SharedExNdPtr> &dimensions,
                                    uint32_t braceLayerNum) {
  uint32_t totalValueNum;
  uint32_t nowValueNum;

  totalValueNum = 1;
  for (int i = braceLayerNum; i < dimensions.size(); i++) {
    totalValueNum *= dimensions.at(i)->_value;
  }
  nowValueNum = 0;

  while (!try_word(1, Token::RBRACE, Token::COMMA, Token::SEMICN)) {
    if (try_word(1, Token::LBRACE)) {
      match_one_word(Token::LBRACE);

      nowValueNum += gm_init_val(init_values, dimensions, braceLayerNum + 1);

      match_one_word(Token::RBRACE);
    } else {
      nowValueNum++;
      SharedExNdPtr value;
      value = gm_exp();
      init_values.push_back(value);
      if (dimensions.size() == 0)
      {
          break;
      }
    }
    if (try_word(1, Token::COMMA)) {
      match_one_word(Token::COMMA);
    }
  }

  for (; nowValueNum < totalValueNum; nowValueNum++) {
    SharedExNdPtr value;
    value = SharedExNdPtr(new ExpressNode());
    value->_type = NodeType::CONST;
    value->_operation = OperationType::NUMBER;
    value->_value = 0;
    init_values.push_back(value);
  }

  return nowValueNum;
}

void SyntaxAnalyze::hp_gn_binary_mir(string tmpName, SharedExNdPtr first,
                                     SharedExNdPtr second, Op op) {
  RightVal op1;
  RightVal op2;

  if (first->_type == NodeType::CONST) {
    op1.emplace<0>(first->_value);
  } else {
    op1 = first->_name;
  }
  if (second->_type == NodeType::CONST) {
    op2.emplace<0>(second->_value);
  } else {
    op2 = second->_name;
  }
  irGenerator.ir_op(tmpName, op1, op2, op);
}

SharedExNdPtr SyntaxAnalyze::gm_const_exp() { return gm_exp(); }

SharedExNdPtr SyntaxAnalyze::gm_exp() {
  SharedExNdPtr first;

  first = gm_mul_exp();

  while (try_word(1, Token::PLUS, Token::MINU)) {
    SharedExNdPtr second;
    SharedExNdPtr father(new ExpressNode());
    Op op;

    if (try_word(1, Token::PLUS)) {
      match_one_word(Token::PLUS);
      father->_operation = OperationType::PLUS;
      op = Op::Add;
    } else {
      match_one_word(Token::MINU);
      father->_operation = OperationType::MINU;
      op = Op::Sub;
    }

    second = gm_mul_exp();

    if (first->_type == NodeType::CONST && second->_type == NodeType::CONST) {
      father->_type = NodeType::CONST;
      switch (father->_operation) {
      case OperationType::PLUS:
        father->_value = first->_value + second->_value;
        break;
      case OperationType::MINU:
        father->_value = first->_value - second->_value;
        break;
      default:
        break;
      }
    } else {
      string tmpName;

      tmpName = irGenerator.getNewTmpValueName(TyKind::Int);
      father->_type = NodeType::VAR;
      father->_name = tmpName;

      hp_gn_binary_mir(tmpName, first, second, op);
    }
    father->addChild(first);
    father->addChild(second);

    first = father;
  }

  return first;
}

SharedExNdPtr SyntaxAnalyze::gm_mul_exp() {
  SharedExNdPtr first;
  first = gm_unary_exp();

  while (try_word(1, Token::MULT, Token::DIV, Token::MOD)) {
    SharedExNdPtr second;
    SharedExNdPtr father(new ExpressNode());
    Op op;

    if (try_word(1, Token::MULT)) {
      match_one_word(Token::MULT);
      father->_operation = OperationType::MUL;
      op = Op::Mul;
    } else if (try_word(1, Token::DIV)) {
      match_one_word(Token::DIV);
      father->_operation = OperationType::DIV;
      op = Op::Div;
    } else {
      match_one_word(Token::MOD);
      father->_operation = OperationType::MOD;
      op = Op::Rem;
    }

    second = gm_unary_exp();

    if (first->_type == NodeType::CONST && second->_type == NodeType::CONST) {
      father->_type = NodeType::CONST;
      switch (father->_operation) {
      case OperationType::MUL:
        father->_value = first->_value * second->_value;
        break;
      case OperationType::DIV:
        father->_value = first->_value / second->_value;
        break;
      case OperationType::MOD:
        father->_value = first->_value % second->_value;
        break;
      default:
        break;
      }
    } else {
      string tmpName;

      tmpName = irGenerator.getNewTmpValueName(TyKind::Int);
      father->_type = NodeType::VAR;
      father->_name = tmpName;

      hp_gn_binary_mir(tmpName, first, second, op);
    }
    father->addChild(first);
    father->addChild(second);
    first = father;
  }
  return first;
}

SharedExNdPtr SyntaxAnalyze::gm_unary_exp() {
  SharedExNdPtr node;

  if (try_word(1, Token::LPARENT)) {
    match_one_word(Token::LPARENT);

    node = gm_exp();

    match_one_word(Token::RPARENT);
  } else if (try_word(1, Token::IDENFR)) {
    if (try_word(2, Token::LPARENT)) {
      node = gm_func_call();
    } else {
      node = gm_l_val(ValueMode::right);
    }
  } else if (try_word(1, Token::INTCON)) {
    string num;
    match_one_word(Token::INTCON);
    num = get_least_matched_word().get_self();
    node = SharedExNdPtr(new ExpressNode());
    node->_value = front::word::stringToInt(num);
    node->_type = NodeType::CONST;
    node->_operation = OperationType::NUMBER;
  } else {
    node = SharedExNdPtr(new ExpressNode());
    if (try_word(1, Token::PLUS, Token::MINU, Token::NOT)) {
      if (try_word(1, Token::PLUS)) {
        match_one_word(Token::PLUS);
        node->_operation = OperationType::UN_PLUS;
      } else if (try_word(1, Token::MINU)) {
        match_one_word(Token::MINU);
        node->_operation = OperationType::UN_MINU;
      } else {
        match_one_word(Token::NOT);
        node->_operation = OperationType::UN_NOT;
      }
    }

    if (node->_operation == OperationType::UN_PLUS) {
      node = gm_unary_exp();
    } else {
      SharedExNdPtr child = gm_unary_exp();

      if (child->_type == NodeType::CONST) {
        node->_type = NodeType::CONST;
        switch (node->_operation) {
        case OperationType::UN_MINU:
          node->_value = -child->_value;
          break;
        case OperationType::UN_NOT:
          node->_value = !child->_value;
          break;
        default:
          break;
        }
      } else {
        SharedExNdPtr tmp;
        string nodeId;

        nodeId = irGenerator.getNewTmpValueName(TyKind::Int);
        node->_type = NodeType::VAR;
        node->_name = nodeId;

        tmp = SharedExNdPtr(new ExpressNode());
        tmp->_type = NodeType::CONST;
        tmp->_operation = OperationType::NUMBER;
        tmp->_value = 0;

        if (node->_operation == OperationType::UN_MINU) {
          hp_gn_binary_mir(nodeId, tmp, child, Op::Sub);
        } else {
          hp_gn_binary_mir(nodeId, tmp, child, Op::Eq);
        }
      }
      node->addChild(child);
    }
  }
  return node;
}

SharedExNdPtr SyntaxAnalyze::computeIndex(SharedSyPtr arr, SharedExNdPtr node) {
  vector<SharedExNdPtr> &dimesions =
      std::static_pointer_cast<ArraySymbol>(arr)->_dimensions;
  string tmpPtr;
  string tmpOff;
  RightVal rightvalue1;
  RightVal rightvalue2;
  SharedExNdPtr index = node->_children.front();
  size_t i;

  tmpPtr = irGenerator.getNewTmpValueName(TyKind::Ptr);
  if (node->_operation == OperationType::ARR) {
    irGenerator.ir_ref(tmpPtr, node->_name);
  } else {
    irGenerator.ir_assign(tmpPtr, node->_name);
  }

  // in function, there should be a virtual d in dimensions.at(0)
  for (i = 1; i < dimesions.size(); i++) {
    if (i == 1) {
      tmpOff = irGenerator.getNewTmpValueName(TyKind::Int);
      if (index->_type == NodeType::CONST) {
        rightvalue1.emplace<0>(index->_value);
        irGenerator.ir_assign(tmpOff, rightvalue1);
      } else {
        irGenerator.ir_assign(tmpOff, index->_name);
      }
    }

    SharedExNdPtr d = dimesions.at(i);

    SharedExNdPtr mulNode = SharedExNdPtr(new ExpressNode());
    SharedExNdPtr addNode = SharedExNdPtr(new ExpressNode());
    SharedExNdPtr index2 = SharedExNdPtr(new ExpressNode());

    if (i < node->_children.size()) {
      index2 = node->_children.at(i);
    } else {
      index2->_type = NodeType::CONST;
      index2->_operation = OperationType::NUMBER;
      index2->_value = 0;
    }

    mulNode->_operation = OperationType::MUL;
    if (index->_type == NodeType::CONST && d->_type == NodeType::CONST) {
      mulNode->_type = NodeType::CONST;
      mulNode->_value = index->_value * d->_value;
      rightvalue1.emplace<0>(mulNode->_value);
      irGenerator.ir_assign(tmpOff, rightvalue1);
    } else {
      mulNode->_type = NodeType::VAR;
      mulNode->_name = tmpOff;
      rightvalue1.emplace<2>(tmpOff);
      if (d->_type == NodeType::CONST) {
        rightvalue2.emplace<0>(d->_value);
      } else {
        rightvalue2 = d->_name;
      }
      irGenerator.ir_op(tmpOff, rightvalue1, rightvalue2, Op::Mul);
    }
    mulNode->addChild(index);
    mulNode->addChild(d);

    addNode->_operation = OperationType::PLUS;
    if (mulNode->_type == NodeType::CONST && index2->_type == NodeType::CONST) {
      addNode->_type = NodeType::CONST;
      addNode->_value = mulNode->_value + index2->_value;
      rightvalue1.emplace<0>(addNode->_value);
      irGenerator.ir_assign(tmpOff, rightvalue1);
    } else {
      addNode->_type = NodeType::VAR;
      addNode->_name = tmpOff;
      rightvalue1.emplace<2>(tmpOff);
      if (index2->_type == NodeType::CONST) {
        rightvalue2.emplace<0>(index2->_value);
      } else {
        rightvalue2 = index2->_name;
      }
      irGenerator.ir_op(tmpOff, rightvalue1, rightvalue2, Op::Add);
    }
    addNode->addChild(mulNode);
    addNode->addChild(index2);

    index = addNode;
  }

  SharedExNdPtr addr = SharedExNdPtr(new ExpressNode());

  addr->_type = NodeType::VAR;
  addr->_operation = OperationType::PTR;
  addr->_name = tmpPtr;
  addr->addChild(node);
  addr->addChild(index);

  if (index->_type == NodeType::CONST) {
    rightvalue1.emplace<0>(index->_value);
  } else {
    rightvalue1 = index->_name;
  }
  irGenerator.ir_offset(tmpPtr, tmpPtr, rightvalue1);

  return addr;
}

SharedExNdPtr SyntaxAnalyze::gm_l_val(ValueMode mode) {
  SharedExNdPtr node(new ExpressNode());
  string name;

  match_one_word(Token::IDENFR);
  name = get_least_matched_word().get_self();

  if (try_word(1, Token::LBRACK)) {
    SharedSyPtr arr = symbolTable.find_least_layer_symbol(name);
    SharedExNdPtr index;
    SharedExNdPtr child;
    SharedExNdPtr addr;
    node->_type = NodeType::VAR;
    node->_operation =
        (std::static_pointer_cast<ArraySymbol>(arr)->isParam() ||
                 (arr->getLayerNum() != _initLayerNum &&
                  !std::static_pointer_cast<ArraySymbol>(arr)->isConst())
             ? OperationType::PTR
             : OperationType::ARR);
    node->_name = (std::static_pointer_cast<ArraySymbol>(arr)->isConst()
                       ? irGenerator.getConstName(arr->getName(), arr->getId())
                       : irGenerator.getVarName(arr->getName(), arr->getId()));
    while (try_word(1, Token::LBRACK)) {
      match_one_word(Token::LBRACK);
      child = gm_exp();
      node->addChild(child);
      match_one_word(Token::RBRACK);
    }

    addr = computeIndex(arr, node);

    if (mode == ValueMode::left ||
        node->_children.size() <
            std::static_pointer_cast<ArraySymbol>(arr)->_dimensions.size()) {
      node = addr;
    } else {
      SharedExNdPtr loadValue;
      string value;

      value = irGenerator.getNewTmpValueName(TyKind::Int);
      loadValue = SharedExNdPtr(new ExpressNode);
      loadValue->_type = NodeType::VAR;
      loadValue->_operation = OperationType::LOAD;
      loadValue->_name = value;
      loadValue->addChild(addr);
      node = loadValue;

      irGenerator.ir_load(value, addr->_name);
    }

  } else {
    SharedSyPtr var = symbolTable.find_least_layer_symbol(name);

    if (var->kind() == SymbolKind::INT) {
      if (std::static_pointer_cast<IntSymbol>(var)->isConst()) {
        node->_type = NodeType::CONST;
        node->_operation = OperationType::VAR;
        node->_value = std::static_pointer_cast<IntSymbol>(var)->getValue();
      } else {
        node->_type = NodeType::VAR;
        node->_operation = OperationType::VAR;
        node->_name = irGenerator.getVarName(var->getName(), var->getId());
        if (var->getLayerNum() == _initLayerNum) {
          SharedExNdPtr addr;
          string addrTmp;

          addrTmp = irGenerator.getNewTmpValueName(TyKind::Ptr);
          addr = SharedExNdPtr(new ExpressNode());
          addr->_type = NodeType::VAR;
          addr->_operation = OperationType::PTR;
          addr->_name = addrTmp;
          addr->addChild(node);

          irGenerator.ir_ref(addrTmp, node->_name);

          if (mode == ValueMode::left) {
            node = addr;
          } else {
            SharedExNdPtr value;
            string valueTmp;

            valueTmp = irGenerator.getNewTmpValueName(TyKind::Int);
            value = SharedExNdPtr(new ExpressNode());
            value->_type = NodeType::VAR;
            value->_operation = OperationType::LOAD;
            value->_name = valueTmp;
            value->addChild(addr);

            irGenerator.ir_load(value->_name, addr->_name);
            node = value;
          }
        }
      }
    } else if (var->kind() == SymbolKind::Array) {
      string arrPtr;

      // The existing of arr name alone which must be the param to func
      arrPtr = irGenerator.getNewTmpValueName(TyKind::Ptr);
      node->_type = NodeType::VAR;
      node->_operation = OperationType::PTR;
      node->_name = arrPtr;

      if (std::static_pointer_cast<ArraySymbol>(var)->isConst()) {
        irGenerator.ir_ref(
            arrPtr, irGenerator.getConstName(var->getName(), var->getId()));
      } else if (std::static_pointer_cast<ArraySymbol>(var)->isParam()) {
        irGenerator.ir_assign(
            arrPtr, irGenerator.getVarName(var->getName(), var->getId()));
      } else if (var->getLayerNum() != _initLayerNum &&
                 !std::static_pointer_cast<ArraySymbol>(var)->isConst()) {
        irGenerator.ir_assign(
            arrPtr, irGenerator.getVarName(var->getName(), var->getId()));
      } else {
        irGenerator.ir_ref(
            arrPtr, irGenerator.getVarName(var->getName(), var->getId()));
      }
    }
  }

  return node;
}

SharedExNdPtr SyntaxAnalyze::gm_func_call() {
  SharedExNdPtr node(new ExpressNode());
  SharedSyPtr func;
  string name;
  SharedExNdPtr param;
  vector<SharedExNdPtr> params;
  vector<RightVal> rightParams;
  RightVal rightVal;

  match_one_word(Token::IDENFR);
  name = get_least_matched_word().get_self();

  func = symbolTable.find_least_layer_symbol(name);

  match_one_word(Token::LPARENT);
  if (!try_word(1, Token::RPARENT)) {
    if (name == "putf") {
      string strName;

      match_one_word(Token::STRCON);

      strName =
          irGenerator.ir_declare_string(get_least_matched_word().get_self());

      param = SharedExNdPtr(new ExpressNode());
      param->_type = NodeType::VAR;
      param->_operation = OperationType::STRING;
      param->_name = strName;
    } else {
      param = gm_exp();
    }
    params.push_back(param);

    while (try_word(1, Token::COMMA)) {
      match_one_word(Token::COMMA);
      param = gm_exp();
      params.push_back(param);
    }
  }
  match_one_word(Token::RPARENT);

  if (std::static_pointer_cast<FunctionSymbol>(func)->getRet() ==
      SymbolKind::INT) {
    string retValue = irGenerator.getNewTmpValueName(TyKind::Int);
    node->_type = NodeType::VAR;
    node->_operation = OperationType::RETURN_FUNC_CALL;
    node->_name = retValue;
  } else {
    node->_type = NodeType::VAR;
    node->_operation = OperationType::VOID_FUNC_CALL;
    node->_name = name;
  }

  for (auto var : params) {
    if (var->_type == NodeType::CONST) {
      rightVal.emplace<0>(var->_value);
    } else {
      rightVal = var->_name;
    }
    rightParams.push_back(rightVal);
  }
  irGenerator.ir_function_call(
      node->_name, std::static_pointer_cast<FunctionSymbol>(func)->getRet(),
      irGenerator.getFunctionName(
          std::static_pointer_cast<FunctionSymbol>(func)->getName()),
      rightParams);

  return node;
}

void SyntaxAnalyze::gm_func_def() {
  string name;
  SymbolKind ret;
  int funcLayerNum;
  vector<SharedSyPtr> params;
  vector<string> paramsName;
  vector<std::pair<SharedSyPtr, string>> genValues;
  SharedSyPtr func;

  if (try_word(1, Token::INTTK)) {
    match_one_word(Token::INTTK);
    ret = SymbolKind::INT;
  } else {
    match_one_word(Token::VOIDTK);
    ret = SymbolKind::VOID;
  }

  match_one_word(Token::IDENFR);
  name = get_least_matched_word().get_self();
  funcLayerNum = layer_num;

  func.reset(new FunctionSymbol(name, ret, funcLayerNum));
  symbolTable.push_symbol(func);

  irGenerator.ir_declare_function(irGenerator.getFunctionName(name), ret);

  match_one_word(Token::LPARENT);
  if (!try_word(1, Token::RPARENT)) {
    gm_func_param(params, funcLayerNum, genValues);
    while (try_word(1, Token::COMMA)) {
      match_one_word(Token::COMMA);
      gm_func_param(params, funcLayerNum, genValues);
    }
  }

  /* keep the arrage to deal with declaring and initualizing
   * 1. declare params in source code
   * 2. declare and initualize generateed values one by one
   */

  for (auto var : params) {
    std::static_pointer_cast<FunctionSymbol>(func)->getParams().push_back(var);
    paramsName.push_back(irGenerator.getVarName(var->getName(), var->getId()));
  }

  irGenerator.ir_finish_param_declare(paramsName);

  for (auto var : genValues) {
    irGenerator.ir_declare_value(var.first->getName(), var.first->kind(),
                                 var.first->getId());
    irGenerator.ir_assign(var.first->getName(), var.second);
  }

  match_one_word(Token::RPARENT);

  gm_block();

  irGenerator.ir_leave_function();
}

void SyntaxAnalyze::gm_func_param(
    vector<SharedSyPtr> &params, int funcLayerNum,
    vector<std::pair<SharedSyPtr, string>> &genValues) {
  string name;
  SharedSyPtr symbol;

  match_one_word(Token::INTTK);
  match_one_word(Token::IDENFR);
  name = get_least_matched_word().get_self();

  if (try_word(1, Token::LBRACK)) {
    symbol.reset(new ArraySymbol(name, IntSymbol::getHolderIntSymbol(),
                                 funcLayerNum + 1, false, true));
    match_one_word(Token::LBRACK);

    SharedExNdPtr holderDimension = SharedExNdPtr(new ExpressNode());
    holderDimension->_type = NodeType::VAR;
    std::static_pointer_cast<ArraySymbol>(symbol)->addDimension(
        SharedExNdPtr(holderDimension));

    match_one_word(Token::RBRACK);

    while (try_word(1, Token::LBRACK)) {
      SharedExNdPtr dimension;
      match_one_word(Token::LBRACK);
      dimension = gm_exp();
      match_one_word(Token::RBRACK);
      if (dimension->_type == NodeType::CONST) {
        std::static_pointer_cast<ArraySymbol>(symbol)->addDimension(dimension);
      } else {
        SharedSyPtr genValue;
        SharedExNdPtr genNode;

        genValue.reset(
            new IntSymbol(hp_gen_save_value(), funcLayerNum + 1, false));
        genValues.push_back(
            std::pair<SharedSyPtr, string>(genValue, dimension->_name));
        symbolTable.push_symbol(genValue);

        genNode->_type = NodeType::VAR;
        genNode->_operation = OperationType::ASSIGN;
        genNode->addChild(dimension);
        genNode->_name = genValue->_name;
        std::static_pointer_cast<ArraySymbol>(symbol)->addDimension(genNode);
      }
    }
  } else {
    symbol.reset(new IntSymbol(name, funcLayerNum + 1, false));
  }
  params.push_back(symbol);

  symbolTable.push_symbol(symbol);
  switch (symbol->kind()) {
  case SymbolKind::Array:
    irGenerator.ir_declare_param(symbol->getName(), SymbolKind::Ptr,
                                 symbol->getId());
    break;
  case SymbolKind::INT:
    irGenerator.ir_declare_param(symbol->getName(), SymbolKind::INT,
                                 symbol->getId());
    break;
  default:
    break;
  }
}

string SyntaxAnalyze::hp_gen_save_value() {
  string name = "$$";
  name = name + "__Compiler__gen__save__value__" + to_string(_genValueNum) +
         "__$$";
  _genValueNum++;
  return name;
}

void SyntaxAnalyze::gm_block() {
  match_one_word(Token::LBRACE);
  in_layer();

  while (!try_word(1, Token::RBRACE)) {
    gm_block_item();
  }

  out_layer();
  match_one_word(Token::RBRACE);
}

void SyntaxAnalyze::gm_block_item() {
  if (try_word(1, Token::CONSTTK)) {
    gm_const_decl();
  } else if (try_word(1, Token::INTTK)) {
    gm_var_decl();
  } else {
    gm_stmt();
  }
}

void SyntaxAnalyze::gm_stmt() {
  if (try_word(1, Token::LBRACE)) {
    gm_block();
  } else if (try_word(1, Token::IFTK)) {
    gm_if_stmt();
  } else if (try_word(1, Token::WHILETK)) {
    gm_while_stmt();
  } else if (try_word(1, Token::BREAK)) {
    match_one_word(Token::BREAK);
    match_one_word(Token::SEMICN);

    irGenerator.ir_jump(mir::inst::JumpInstructionKind::Br,
                        irGenerator.checkWhile()._endLabel, -1, std::nullopt,
                        mir::inst::JumpKind::Loop);
  } else if (try_word(1, Token::CONTINUETK)) {
    match_one_word(Token::CONTINUETK);
    match_one_word(Token::SEMICN);

    irGenerator.ir_jump(mir::inst::JumpInstructionKind::Br,
                        irGenerator.checkWhile()._beginLabel, -1, std::nullopt,
                        mir::inst::JumpKind::Loop);
  } else if (try_word(1, Token::RETURNTK)) {
    gm_return_stmt();
  } else if (find_word(Token::ASSIGN, Token::SEMICN)) {
    gm_assign_stmt();
  } else if (!try_word(1, Token::SEMICN)) {
    gm_exp();
  } else {
    match_one_word(Token::SEMICN);
  }
}

void SyntaxAnalyze::gm_if_stmt() {
  SharedExNdPtr cond;
  LabelId ifTrue = irGenerator.getNewLabelId();
  LabelId ifFalse = irGenerator.getNewLabelId();
  std::optional<string> condStr;

  match_one_word(Token::IFTK);

  match_one_word(Token::LPARENT);

  cond = gm_cond();

  match_one_word(Token::RPARENT);

  if (cond->_type == NodeType::CONST) {
    string tmpName = irGenerator.getNewTmpValueName(TyKind::Int);
    RightVal right;
    right.emplace<0>(cond->_value);
    irGenerator.ir_assign(tmpName, right);
    condStr = tmpName;
  } else {
    condStr = cond->_name;
  }
  irGenerator.ir_jump(mir::inst::JumpInstructionKind::BrCond, ifTrue, ifFalse,
                      condStr, mir::inst::JumpKind::Branch);
  irGenerator.ir_label(ifTrue);

  gm_stmt();

  if (try_word(1, Token::ELSETK)) {
    LabelId ifEnd = irGenerator.getNewLabelId();
    irGenerator.ir_jump(mir::inst::JumpInstructionKind::Br, ifEnd, -1,
                        std::nullopt, mir::inst::JumpKind::Branch);
    irGenerator.ir_label(ifFalse);

    match_one_word(Token::ELSETK);
    gm_stmt();

    irGenerator.ir_label(ifEnd);
  } else {
    irGenerator.ir_label(ifFalse);
  }
}

void SyntaxAnalyze::gm_while_stmt() {
  SharedExNdPtr cond;
  LabelId whileTrue = irGenerator.getNewLabelId();
  LabelId whileEnd = irGenerator.getNewLabelId();
  LabelId whileBegin = irGenerator.getNewLabelId();
  std::optional<string> condStr;
  irGenerator::WhileLabels whileLabels =
      irGenerator::WhileLabels(whileBegin, whileEnd);

  irGenerator.pushWhile(whileLabels);
  irGenerator.ir_label(whileBegin);

  match_one_word(Token::WHILETK);
  match_one_word(Token::LPARENT);

  cond = gm_cond();

  if (cond->_type == NodeType::CONST) {
    string tmpName = irGenerator.getNewTmpValueName(TyKind::Int);
    RightVal right;
    right.emplace<0>(cond->_value);
    irGenerator.ir_assign(tmpName, right);
    condStr = tmpName;
  } else {
    condStr = cond->_name;
  }
  irGenerator.ir_jump(mir::inst::JumpInstructionKind::BrCond, whileTrue,
                      whileEnd, condStr, mir::inst::JumpKind::Loop);
  irGenerator.ir_label(whileTrue);

  match_one_word(Token::RPARENT);
  gm_stmt();

  irGenerator.ir_jump(mir::inst::JumpInstructionKind::Br, whileBegin, -1,
                      std::nullopt, mir::inst::JumpKind::Loop);
  irGenerator.ir_label(whileEnd);
  irGenerator.popWhile();
}

void SyntaxAnalyze::gm_return_stmt() {
  std::optional<string> retStr;

  match_one_word(Token::RETURNTK);
  if (!try_word(1, Token::SEMICN)) {
    SharedExNdPtr retValue;

    retValue = gm_exp();

    if (retValue->_type == NodeType::CONST) {
      string tmpName = irGenerator.getNewTmpValueName(TyKind::Int);
      RightVal right;
      right.emplace<0>(retValue->_value);
      irGenerator.ir_assign(tmpName, right);
      retStr = tmpName;
    } else {
      retStr = retValue->_name;
    }
  }
  match_one_word(Token::SEMICN);

  irGenerator.ir_jump(mir::inst::JumpInstructionKind::Return, -1, -1, retStr,
                      mir::inst::JumpKind::Undefined);
}

void SyntaxAnalyze::gm_assign_stmt() {
  SharedExNdPtr lVal;
  SharedExNdPtr rVal;
  RightVal rightValue;

  lVal = gm_l_val(ValueMode::left);
  match_one_word(Token::ASSIGN);
  rVal = gm_exp();
  match_one_word(Token::SEMICN);

  if (rVal->_type == NodeType::CONST) {
    rightValue.emplace<0>(rVal->_value);
  } else {
    rightValue = rVal->_name;
  }
  if (lVal->_operation == OperationType::PTR) {
    irGenerator.ir_store(lVal->_name, rightValue);
  } else {
    irGenerator.ir_assign(lVal->_name, rightValue);
  }
}

SharedExNdPtr SyntaxAnalyze::gm_cond() {
  SharedExNdPtr first;
  first = gm_and_exp();

  while (try_word(1, Token::OR)) {
    SharedExNdPtr second;
    SharedExNdPtr father(new ExpressNode());
    Op op;

    father->_operation = OperationType::OR;

    match_one_word(Token::OR);
    op = Op::Or;

    second = gm_and_exp();

    if (first->_type == NodeType::CONST && second->_type == NodeType::CONST) {
      father->_type = NodeType::CONST;
      father->_value = first->_value || second->_value;
    } else {
      string tmpName;

      tmpName = irGenerator.getNewTmpValueName(TyKind::Int);
      father->_type = NodeType::VAR;
      father->_name = tmpName;

      hp_gn_binary_mir(tmpName, first, second, op);
    }
    father->addChild(first);
    father->addChild(second);
    first = father;
  }

  return first;
}

SharedExNdPtr SyntaxAnalyze::gm_and_exp() {
  SharedExNdPtr first;
  first = gm_eq_exp();

  while (try_word(1, Token::AND)) {
    SharedExNdPtr second;
    SharedExNdPtr father(new ExpressNode());
    Op op;

    father->_operation = OperationType::OR;

    match_one_word(Token::AND);
    op = Op::And;

    second = gm_eq_exp();

    if (first->_type == NodeType::CONST && second->_type == NodeType::CONST) {
      father->_type = NodeType::CONST;
      father->_value = first->_value && second->_value;
    } else {
      string tmpName;

      tmpName = irGenerator.getNewTmpValueName(TyKind::Int);
      father->_type = NodeType::VAR;
      father->_name = tmpName;

      hp_gn_binary_mir(tmpName, first, second, op);
    }
    father->addChild(first);
    father->addChild(second);
    first = father;
  }
  return first;
}

SharedExNdPtr SyntaxAnalyze::gm_eq_exp() {
  SharedExNdPtr first;
  first = gm_rel_exp();

  while (try_word(1, Token::EQL, Token::NEQ)) {
    SharedExNdPtr second;
    SharedExNdPtr father(new ExpressNode());
    Op op;

    if (try_word(1, Token::EQL)) {
      match_one_word(Token::EQL);
      father->_operation = OperationType::EQL;
      op = Op::Eq;
    } else {
      match_one_word(Token::NEQ);
      father->_operation = OperationType::NEQ;
      op = Op::Neq;
    }

    second = gm_rel_exp();

    if (first->_type == NodeType::CONST && second->_type == NodeType::CONST) {
      father->_type = NodeType::CONST;
      switch (father->_operation) {
      case OperationType::EQL:
        father->_value = first->_value == second->_value;
        break;
      case OperationType::NEQ:
        father->_value = first->_value != second->_value;
        break;
      default:
        break;
      }
    } else {
      string tmpName;

      tmpName = irGenerator.getNewTmpValueName(TyKind::Int);
      father->_type = NodeType::VAR;
      father->_name = tmpName;

      hp_gn_binary_mir(tmpName, first, second, op);
    }

    father->addChild(first);
    father->addChild(second);
    first = father;
  }
  return first;
}

SharedExNdPtr SyntaxAnalyze::gm_rel_exp() {
  SharedExNdPtr first;
  first = gm_exp();

  while (try_word(1, Token::LSS, Token::GRE) ||
         try_word(1, Token::LEQ, Token::GEQ)) {
    SharedExNdPtr second;
    SharedExNdPtr father(new ExpressNode());
    Op op;

    if (try_word(1, Token::LSS)) {
      father->_operation = OperationType::LSS;
      match_one_word(Token::LSS);
      op = Op::Lt;
    } else if (try_word(1, Token::GRE)) {
      father->_operation = OperationType::GRE;
      match_one_word(Token::GRE);
      op = Op::Gt;
    } else if (try_word(1, Token::LEQ)) {
      father->_operation = OperationType::LEQ;
      match_one_word(Token::LEQ);
      op = Op::Lte;
    } else {
      father->_operation = OperationType::GEQ;
      match_one_word(Token::GEQ);
      op = Op::Gte;
    }

    second = gm_exp();

    if (first->_type == NodeType::CONST && second->_type == NodeType::CONST) {
      father->_type = NodeType::CONST;
      switch (father->_operation) {
      case OperationType::LSS:
        father->_value = first->_value < second->_value;
        break;
      case OperationType::LEQ:
        father->_value = first->_value <= second->_value;
        break;
      case OperationType::GRE:
        father->_value = first->_value > second->_value;
        break;
      case OperationType::GEQ:
        father->_value = first->_value >= second->_value;
        break;
      default:
        break;
      }
    } else {
      string tmpName;

      tmpName = irGenerator.getNewTmpValueName(TyKind::Int);
      father->_type = NodeType::VAR;
      father->_name = tmpName;

      hp_gn_binary_mir(tmpName, first, second, op);
    }

    father->addChild(first);
    father->addChild(second);
    first = father;
  }
  return first;
}

bool SyntaxAnalyze::try_word(int n, Token tk) {
  bool return_value = false;
  if (matched_index + n < word_list.size() &&
      word_list[matched_index + n].match_token(tk)) {
    return_value = true;
  }
  return return_value;
}

bool SyntaxAnalyze::try_word(int n, Token tk1, Token tk2) {
  bool return_value = false;
  if (matched_index + n < word_list.size()) {
    return_value = try_word(n, tk1) || try_word(n, tk2);
  }
  return return_value;
}

bool SyntaxAnalyze::try_word(int n, Token tk1, Token tk2, Token tk3) {
  bool return_value = false;
  if (matched_index + n < word_list.size()) {
    return_value = try_word(n, tk1) || try_word(n, tk2) || try_word(n, tk3);
  }
  return return_value;
}

bool SyntaxAnalyze::find_word(Token tk, Token end) {
  size_t size = word_list.size();
  bool r = false;
  for (size_t i = matched_index + 1; i < size; i++) {
    if (word_list[i].match_token(tk)) {
      r = true;
      break;
    } else if (word_list[i].match_token(end)) {
      break;
    }
  }
  return r;
}

bool SyntaxAnalyze::match_one_word(Token tk) {
  bool return_value = false;
  if (matched_index + 1 < word_list.size()) {
    if (word_list[matched_index + 1].match_token(tk)) {
      LOG(TRACE) << word_list[matched_index + 1].get_self() << std::endl;
      matched_index++;
      return_value = true;
    }
  } else {
    assert(false);
  }
  return return_value;
}

void SyntaxAnalyze::in_layer() { layer_num++; }

void SyntaxAnalyze::out_layer() {
  symbolTable.pop_layer_symbols(layer_num);
  layer_num--;
}

bool SyntaxAnalyze::inGlobalLayer() { return layer_num == _initLayerNum; }

const Word &SyntaxAnalyze::get_least_matched_word() {
  return word_list[matched_index];
}

const Word &SyntaxAnalyze::get_next_unmatched_word() {
  if (matched_index + 1 < word_list.size()) {
    return word_list[matched_index + 1];
  } else {
    assert(false);
  }
}

size_t SyntaxAnalyze::get_matched_index() { return matched_index; }

void SyntaxAnalyze::set_matched_index(size_t new_index) {
  matched_index = new_index;
}
