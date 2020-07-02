#include "syntax_analyze.hpp"

using namespace front::syntax;

vector<Word> tmp_vect_word;

SyntaxAnalyze::SyntaxAnalyze()
    : matched_index(-1), word_list(tmp_vect_word)
{}

SyntaxAnalyze::SyntaxAnalyze(vector<Word>& in_word_list)
    : matched_index(-1), word_list(in_word_list)
{}

SyntaxAnalyze::~SyntaxAnalyze()
{}


void SyntaxAnalyze::gm_comp_unit()
{
    in_layer();
    while (matched_index + 1 < word_list.size())
    {
        if (try_word(1, Token::INTTK, Token::VOIDTK) && try_word(2, Token::IDENFR) && try_word(3, Token::LPARENT))
        {
            gm_func_def();
        }
        else if (try_word(1, Token::CONSTTK))
        {
            gm_const_decl();
        }
        else
        {
            gm_var_decl();
        }
    }
    out_layer();
}

void SyntaxAnalyze::gm_const_decl()
{
    match_one_word(Token::CONSTTK);
    match_one_word(Token::INTTK);
    gm_const_def();
    while (try_word(1, Token::COMMA))
    {
        match_one_word(Token::COMMA);
        gm_const_def();
    }
    match_one_word(Token::SEMICN);
}

void SyntaxAnalyze::gm_const_def()
{
    string name;
    SymbolKind kind;
    SharedSyPtr symbol;
    SharedExNdPtr dimension;
    vector<SharedExNdPtr> init_values;
    vector<SharedExNdPtr> dimensions;

    match_one_word(Token::IDENFR);
    name = get_least_matched_word().get_self();

    if (try_word(1, Token::LBRACK))
    {
        kind = SymbolKind::Array;
        while (try_word(1, Token::LBRACK))
        {
            match_one_word(Token::LBRACK);
            dimension = gm_const_exp();
            match_one_word(Token::RBRACK);

            dimensions.push_back(dimension);
        }
    }
    else
    {
        kind = SymbolKind::INT;
    }

    match_one_word(Token::ASSIGN);

    gm_const_init_val(init_values);

    if (kind == SymbolKind::INT)
    {
        symbol.reset(new IntSymbol(name, layer_num, true, init_values.front()->_value));
        irGenerator.ir_declare_value(name, kind);

    }
    else
    {
        symbol.reset(new ArraySymbol(name, IntSymbol::getHolderIntSymbol(), layer_num, true, false));
        for (auto var : dimensions)
        {
            std::static_pointer_cast<ArraySymbol>(symbol)->addDimension(var);
        }

        irGenerator.ir_declare_value(name, kind, std::static_pointer_cast<ArraySymbol>(symbol)->getLen());
    }

    if (init_values.size() > 0)
    {
        LabelId initPtr;
        RightVal rightVal;
        int offset = 0;

        initPtr = irGenerator.getNewTmpValueId();
        irGenerator.ir_ref(initPtr, name);

        for (auto var : init_values)
        {
            if (offset > 0)
            {
                rightVal.emplace<0>(1);
                irGenerator.ir_offset(initPtr, initPtr, rightVal);
            }

            std::static_pointer_cast<ArraySymbol>(symbol)->addValue(var);

            rightVal.emplace<0>(var->_value);
            irGenerator.ir_store(initPtr, rightVal);
            offset++;
        }
    }

    symbolTable.push_symbol(symbol);
}

void SyntaxAnalyze::gm_const_init_val(vector<SharedExNdPtr>& init_values)
{
    if (try_word(1, Token::LBRACE))
    {
        match_one_word(Token::LBRACE);
        if (!try_word(1, Token::RBRACE))
        {
            gm_const_init_val(init_values);
            while (try_word(1, Token::COMMA))
            {
                match_one_word(Token::COMMA);
                gm_const_init_val(init_values);
            }
        }
        match_one_word(Token::RBRACE);
    }
    else
    {
        SharedExNdPtr value;
        value = gm_const_exp();
        init_values.push_back(value);
    }
}

void SyntaxAnalyze::gm_var_decl()
{
    match_one_word(Token::INTTK);
    gm_var_def();
    while (try_word(1, Token::COMMA))
    {
        match_one_word(Token::COMMA);
        gm_var_def();
    }
    match_one_word(Token::SEMICN);
}

void SyntaxAnalyze::gm_var_def()
{
    string name;
    SymbolKind kind;
    SharedExNdPtr dimension;
    vector<SharedExNdPtr> dimensions;
    vector<SharedExNdPtr> init_values;
    SharedSyPtr symbol;

    match_one_word(Token::IDENFR);
    name = get_least_matched_word().get_self();

    if (try_word(1, Token::LBRACK))
    {
        kind = SymbolKind::Array;
        while (try_word(1, Token::LBRACK))
        {
            match_one_word(Token::LBRACK);
            dimension = gm_const_exp();
            match_one_word(Token::RBRACK);
        }
    }
    else
    {
        kind = SymbolKind::INT;
    }

    if (try_word(1, Token::ASSIGN))
    {
        match_one_word(Token::ASSIGN);
        gm_init_val(init_values);
    }

    if (kind == SymbolKind::INT)
    {
        // TODO: mir to init var
        symbol.reset(new IntSymbol(name, layer_num, false));
        irGenerator.ir_declare_value(name, kind);
    }
    else
    {
        symbol.reset(new ArraySymbol(name, IntSymbol::getHolderIntSymbol(), layer_num, false, false));
        for (auto var : dimensions)
        {
            std::static_pointer_cast<ArraySymbol>(symbol)->addDimension(var);
        }

        irGenerator.ir_declare_value(name, kind, std::static_pointer_cast<ArraySymbol>(symbol)->getLen());
    }

    if (init_values.size() > 0)
    {
        LabelId initPtr;
        int offset = 0;
        irGenerator::RightVal rightVal;

        initPtr = irGenerator.getNewTmpValueId();
        irGenerator.ir_ref(initPtr, name);

        for (auto var : init_values)
        {
            if (offset > 0)
            {
                rightVal.emplace<0>(1);
                irGenerator.ir_offset(initPtr, initPtr, rightVal);
            }
            else
            {
                initPtr = irGenerator.getNewTmpValueId();
                irGenerator.ir_ref(initPtr, name);
            }

            std::static_pointer_cast<ArraySymbol>(symbol)->addValue(var);

            if (var->_type == NodeType::CONST)
            {
                rightVal.emplace<0>(var->_value);
            }
            else
            {
                rightVal.emplace<2>(var->_name);
            }
            irGenerator.ir_store(initPtr, rightVal);
            offset++;
        }
    }
    symbolTable.push_symbol(symbol);
}

void SyntaxAnalyze::gm_init_val(vector<SharedExNdPtr>& init_values)
{
    if (try_word(1, Token::LBRACE))
    {
        match_one_word(Token::LBRACE);
        if (!try_word(1, Token::RBRACE))
        {
            gm_init_val(init_values);
            while (try_word(1, Token::COMMA))
            {
                match_one_word(Token::COMMA);
                gm_init_val(init_values);
            }
        }
        match_one_word(Token::RBRACE);
    }
    else
    {
        SharedExNdPtr value;
        value = gm_exp();
        init_values.push_back(value);
    }
}

void SyntaxAnalyze::hp_gn_binary_mir(LabelId tmpId ,SharedExNdPtr first, SharedExNdPtr second, Op op)
{
    RightVal op1;
    RightVal op2;

    if (first->_type == NodeType::CONST)
    {
        op1.emplace<0>(first->_value);
    }
    else
    {
        op1 = first->_name;
    }
    if (second->_type == NodeType::CONST)
    {
        op1.emplace<0>(second->_value);
    }
    else
    {
        op1 = second->_name;
    }
    irGenerator.ir_op(tmpId, op1, op2, op);
}


SharedExNdPtr SyntaxAnalyze::gm_const_exp()
{
    return gm_exp();
}

SharedExNdPtr SyntaxAnalyze::gm_exp()
{
    SharedExNdPtr first;

    first = gm_mul_exp();

    while (try_word(1, Token::PLUS, Token::MINU))
    {
        SharedExNdPtr second;
        SharedExNdPtr father(new ExpressNode());
        Op op;

        if (try_word(1, Token::PLUS))
        {
            match_one_word(Token::PLUS);
            father->_operation = OperationType::PLUS;
            op = Op::Add;
        }
        else
        {
            match_one_word(Token::MINU);
            father->_operation = OperationType::MINU;
            op = Op::Sub;
        }

        second = gm_mul_exp();

        if (first->_type == NodeType::CONST && second->_type == NodeType::CONST)
        {
            father->_type = NodeType::CONST;
            switch (father->_operation)
            {
            case OperationType::PLUS:
                father->_value = first->_value + second->_value;
                break;
            case OperationType::MINU:
                father->_value = first->_value - second->_value;
                break;
            default:
                break;
            }
        }
        else
        {
            LabelId tmpId;

            tmpId = irGenerator.getNewLabelId();
            father->_type = NodeType::VAR;
            father->_name = to_string(tmpId);

            hp_gn_binary_mir(tmpId, first, second, op);
        }
        father->addChild(first);
        father->addChild(second);

        first = father;
    }

    return first;
}

SharedExNdPtr SyntaxAnalyze::gm_mul_exp()
{
    SharedExNdPtr first;
    first = gm_unary_exp();

    while (try_word(1, Token::MULT, Token::DIV, Token::MOD))
    {
        SharedExNdPtr second;
        SharedExNdPtr father(new ExpressNode());
        Op op;

        if (try_word(1, Token::MULT))
        {
            match_one_word(Token::MULT);
            father->_operation = OperationType::MUL;
            op = Op::Mul;
        }
        else if (try_word(1, Token::DIV))
        {
            match_one_word(Token::DIV);
            father->_operation = OperationType::DIV;
            op = Op::Div;
        }
        else
        {
            match_one_word(Token::MOD);
            father->_operation = OperationType::MOD;
            op = Op::Rem;
        }

        second = gm_unary_exp();

        if (first->_type == NodeType::CONST && second->_type == NodeType::CONST)
        {
            father->_type = NodeType::CONST;
            switch (father->_operation)
            {
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
        }
        else
        {
            LabelId tmpId;

            tmpId = irGenerator.getNewLabelId();
            father->_type = NodeType::VAR;
            father->_name = to_string(tmpId);

            hp_gn_binary_mir(tmpId, first, second, op);
        }
        father->addChild(first);
        father->addChild(second);
        first = father;
    }
    return first;
}

SharedExNdPtr SyntaxAnalyze::gm_unary_exp()
{
    SharedExNdPtr node;

    if (try_word(1, Token::LPARENT))
    {
        match_one_word(Token::LPARENT);

        node = gm_exp();

        match_one_word(Token::RPARENT);
    }
    else if (try_word(1, Token::IDENFR))
    {
        if (try_word(2, Token::LPARENT))
        {
            node = gm_func_call();
        }
        else
        {
            node = gm_l_val();
        }
    }
    else if (try_word(1, Token::INTCON))
    {
        string num;
        match_one_word(Token::INTCON);
        num = get_least_matched_word().get_self();
        node.reset(new ExpressNode());
        node->_value = front::word::stringToInt(num);
        node->_type = NodeType::CONST;
        node->_operation = OperationType::NUMBER;
    }
    else
    {
        if (try_word(1, Token::PLUS, Token::MINU, Token::NOT))
        {
            if (try_word(1, Token::PLUS))
            {
                match_one_word(Token::PLUS);
                node->_operation = OperationType::UN_PLUS;
            }
            else if (try_word(1, Token::MINU))
            {
                match_one_word(Token::MINU);
                node->_operation = OperationType::UN_MINU;
            }
            else
            {
                match_one_word(Token::NOT);
                node->_operation = OperationType::UN_NOT;
            }
        }

        if (node->_operation == OperationType::UN_PLUS)
        {
            node = gm_unary_exp();
        }
        else
        {
            node.reset(new ExpressNode());
            SharedExNdPtr child = gm_unary_exp();

            if (child->_type == NodeType::CONST)
            {
                node->_type = NodeType::CONST;
                switch (node->_operation)
                {
                case OperationType::UN_MINU:
                    node->_value = -child->_value;
                    break;
                case OperationType::UN_NOT:
                    node->_value = !child->_value;
                    break;
                default:
                    break;
                }
            }
            else
            {
                SharedExNdPtr tmp;
                LabelId nodeId;

                nodeId = irGenerator.getNewTmpValueId();
                node->_type = NodeType::VAR;
                node->_name = to_string(nodeId);

                tmp->_type = NodeType::CONST;
                tmp->_operation = OperationType::NUMBER;
                tmp->_value = 0;

                if (node->_operation == OperationType::UN_MINU)
                {
                    hp_gn_binary_mir(nodeId, tmp, child, Op::Sub);
                }
                else
                {
                    hp_gn_binary_mir(nodeId, tmp, child, Op::Eq);
                }
            }
            node->addChild(child);
        }
    }
    return node;
}

SharedExNdPtr SyntaxAnalyze::computeIndex(SharedSyPtr arr, SharedExNdPtr node)
{
    vector<SharedExNdPtr>& dimesions = std::static_pointer_cast<ArraySymbol>(arr)->_dimensions;
    LabelId tmpPtr;
    LabelId tmpOff;
    RightVal rightvalue1;
    RightVal rightvalue2;
    SharedExNdPtr index = node->_children.front();
    int i;
    int j;

    tmpPtr = irGenerator.getNewTmpValueId();
    irGenerator.ir_load(tmpPtr, std::static_pointer_cast<ArraySymbol>(arr)->getName());

    tmpOff = irGenerator.getNewTmpValueId();
    if (index->_type == NodeType::CONST)
    {
        rightvalue1.emplace<0>(index->_value);
        irGenerator.ir_assign(tmpOff, rightvalue1);
    }
    else
    {
        irGenerator.ir_assign(tmpOff, index->_name);
    }

    // int function, there should be a virtual d in dimensions.at(0)
    for (i = 1; i < dimesions.size(); i++)
    {
        SharedExNdPtr d = dimesions.at(i);

        SharedExNdPtr mulNode = SharedExNdPtr(new ExpressNode());
        SharedExNdPtr addNode = SharedExNdPtr(new ExpressNode());
        SharedExNdPtr index2 = node->_children.at(i);

        mulNode->_operation = OperationType::MUL;
        if (index->_type == NodeType::CONST && d->_type == NodeType::CONST)
        {
            mulNode->_type = NodeType::CONST;
            mulNode->_value = index->_value * d->_value;
        }
        else
        {
            mulNode->_type = NodeType::VAR;
            mulNode->_name = to_string(tmpOff);
            rightvalue1.emplace<2>(to_string(tmpOff));
            if (d->_type == NodeType::CONST)
            {
                rightvalue2.emplace<0>(d->_value);
            }
            else
            {
                rightvalue2 = d->_name;
            }
            irGenerator.ir_op(tmpOff, rightvalue1, rightvalue2, Op::Mul);
        }
        mulNode->addChild(index);
        mulNode->addChild(d);


        addNode->_operation = OperationType::PLUS;
        if (mulNode->_type == NodeType::CONST && index2->_type == NodeType::CONST)
        {
            addNode->_type = NodeType::CONST;
            addNode->_value = mulNode->_value + index2->_value;
        }
        else
        {
            mulNode->_type = NodeType::VAR;
            mulNode->_name = to_string(tmpOff);
            rightvalue1.emplace<2>(to_string(tmpOff));
            if (index2->_type == NodeType::CONST)
            {
                rightvalue2.emplace<0>(index2->_value);
            }
            else
            {
                rightvalue2 = index2->_name;
            }
            irGenerator.ir_op(tmpOff, rightvalue1, rightvalue2, Op::Add);
        }
        addNode->addChild(mulNode);
        addNode->addChild(index2);

        index = addNode;
    }

    SharedExNdPtr addr = SharedExNdPtr(new ExpressNode());
    LabelId tmpValue;

    tmpValue = irGenerator.getNewLabelId();
    addr->_type = NodeType::VAR;
    addr->_operation = OperationType::VAR;
    addr->_name = to_string(tmpValue);
    addr->addChild(node);
    addr->addChild(index);

    if (index->_type == NodeType::CONST)
    {
        rightvalue1.emplace<0>(index->_value);
    }
    else
    {
        rightvalue1 = index->_name;
    }
    irGenerator.ir_offset(tmpPtr, tmpPtr, rightvalue1);
    irGenerator.ir_load(tmpValue, to_string(tmpPtr));

    return addr;
}

SharedExNdPtr SyntaxAnalyze::gm_l_val()
{
    SharedExNdPtr node(new ExpressNode());
    string name;

    match_one_word(Token::IDENFR);
    name = get_least_matched_word().get_self();

    if (try_word(1, Token::LBRACK))
    {
        SharedSyPtr arr = symbolTable.find_least_layer_symbol(name);
        SharedExNdPtr index;
        SharedExNdPtr child;

        node->_type = NodeType::VAR;
        node->_operation = OperationType::ARR;
        node->_name = name;
        while (try_word(1, Token::LBRACK))
        {
            match_one_word(Token::LBRACK);
            child = gm_exp();
            node->addChild(child);
            match_one_word(Token::RBRACK);
        }

        // TODO: deal with the bug for assign to specify array element
        node = computeIndex(arr, node);
    }
    else
    {
        SharedSyPtr var = symbolTable.find_least_layer_symbol(name);

        if (var->kind() == SymbolKind::INT)
        {
            if (std::static_pointer_cast<IntSymbol>(var)->isConst())
            {
                node->_type = NodeType::CONST;
                node->_operation = OperationType::VAR;
                node->_value = std::static_pointer_cast<IntSymbol>(var)->getValue();
            }
            else
            {
                node->_type = NodeType::VAR;
                node->_operation = OperationType::VAR;
            }
        }
        else if (var->kind() == SymbolKind::Array)
        {
            // The existing of arr name alone which must be the param to func
            node->_type = NodeType::VAR;
            node->_operation = OperationType::ARR_NAME;
            node->_name = name;
        }
    }

    return node;
}

SharedExNdPtr SyntaxAnalyze::gm_func_call()
{
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

    // TODO: deal IO function
    match_one_word(Token::LPARENT);
    if (!try_word(1, Token::RPARENT))
    {
        param = gm_exp();
        params.push_back(param);

        while (try_word(1, Token::COMMA))
        {
            match_one_word(Token::COMMA);
            param = gm_exp();
            params.push_back(param);
        }
    }
    match_one_word(Token::RPARENT);

    if (std::static_pointer_cast<FunctionSymbol>(func)->getRet() == SymbolKind::INT)
    {
        LabelId retValue = irGenerator.getNewTmpValueId();
        node->_type = NodeType::VAR;
        node->_operation = OperationType::RETURN_FUNC_CALL;
        node->_name = to_string(retValue);
    }
    else
    {
        node->_type = NodeType::VAR;
        node->_operation = OperationType::VOID_FUNC_CALL;
        node->_name = name;
    }

    for (auto var : params)
    {
        if (var->_type == NodeType::CONST)
        {
            rightVal.emplace<0>(var->_value);
        }
        else
        {
            rightVal = var->_name;
        }
        rightParams.push_back(rightVal);
    }
    irGenerator.ir_function_call(
        node->_name,
        std::static_pointer_cast<FunctionSymbol>(func)->getRet(),
        std::static_pointer_cast<FunctionSymbol>(func)->getName(), 
        rightParams
    );

    return node;
}

void SyntaxAnalyze::gm_func_def()
{
    string name;
    SymbolKind ret;
    int funcLayerNum;
    vector<SharedSyPtr> params;
    vector<std::pair<SharedSyPtr, string>> gemValues;
    SharedSyPtr func;

    if (try_word(1, Token::INTTK))
    {
        match_one_word(Token::INTTK);
        ret = SymbolKind::INT;
    }
    else
    {
        match_one_word(Token::VOIDTK);
        ret = SymbolKind::VOID;
    }

    match_one_word(Token::IDENFR);
    name = get_least_matched_word().get_self();
    funcLayerNum = layer_num;

    func.reset(new FunctionSymbol(name, ret, funcLayerNum));
    symbolTable.push_symbol(func);

    irGenerator.ir_declare_function(name, ret);

    match_one_word(Token::LPARENT);
    if (!try_word(1, Token::RPARENT))
    {
        gm_func_param(params, funcLayerNum, gemValues);
        while (try_word(1, Token::COMMA))
        {
            match_one_word(Token::COMMA);
            gm_func_param(params, funcLayerNum, gemValues);
        }
    }

    /* keep the arrane to deal with declaring and initualizing
     * 1. declare params in source code
     * 2. declare and initualize generateed values one by one 
     */

    for (auto var : params)
    {
        std::static_pointer_cast<FunctionSymbol>(func)->getParams().push_back(var);
    }

    for (auto var : gemValues)
    {
        irGenerator.ir_declare_value(var.first->getName(), var.first->kind());
        irGenerator.ir_assign(var.first->getName(), var.second);
    }

    match_one_word(Token::RPARENT);

    gm_block();

    irGenerator.ir_leave_function();
}

void SyntaxAnalyze::gm_func_param(vector<SharedSyPtr>& params, int funcLayerNum, vector<std::pair<SharedSyPtr, string>>& genValues)
{
    string name;
    SharedSyPtr symbol;

    match_one_word(Token::INTTK);
    match_one_word(Token::IDENFR);
    name = get_least_matched_word().get_self();

    if (try_word(1, Token::LBRACK))
    {
        symbol.reset(new ArraySymbol(name, IntSymbol::getHolderIntSymbol(), funcLayerNum + 1, false, true));
        match_one_word(Token::LBRACK);
        match_one_word(Token::RBRACK);

        while (try_word(1, Token::LBRACK))
        {
            SharedExNdPtr dimension;
            match_one_word(Token::LBRACK);
            dimension = gm_exp();
            match_one_word(Token::RBRACK);
            if (dimension->_type == NodeType::CONST)
            {
                std::static_pointer_cast<ArraySymbol>(symbol)->addDimension(dimension);
            }
            else
            {
                SharedSyPtr genValue;
                SharedExNdPtr genNode;

                genValue.reset(new IntSymbol(hp_gen_save_value(), funcLayerNum + 1, false));
                genValues.push_back(std::pair<SharedSyPtr, string>(genValue, dimension->_name));
                symbolTable.push_symbol(genValue);

                genNode->_type = NodeType::VAR;
                genNode->_operation = OperationType::ASSIGN;
                genNode->addChild(dimension);
                genNode->_name = genValue->_name;
                std::static_pointer_cast<ArraySymbol>(symbol)->addDimension(genNode);
            }
        }
    }
    else
    {
        symbol.reset(new IntSymbol(name, funcLayerNum + 1, false));
    }
    params.push_back(symbol);

    symbolTable.push_symbol(symbol);
    irGenerator.ir_declare_param(symbol->getName(), symbol->kind());
}

string SyntaxAnalyze::hp_gen_save_value()
{
    string name = "@@";
    name = name + "__Compiler__gen__save__value__" + to_string(_genValueNum) + "__@@";
    _genValueNum++;
    return name;
}

void SyntaxAnalyze::gm_block()
{
    match_one_word(Token::LBRACE);
    in_layer();

    while (!try_word(1, Token::RBRACE))
    {
        gm_block_item();
    }

    out_layer();
    match_one_word(Token::RBRACE);
}

void SyntaxAnalyze::gm_block_item()
{
    if (try_word(1, Token::CONSTTK))
    {
        gm_const_decl();
    }
    else if (try_word(1, Token::INTTK))
    {
        gm_var_decl();
    }
    else
    {
        gm_stmt();
    }
}

void SyntaxAnalyze::gm_stmt()
{
    if (try_word(1, Token::LBRACE))
    {
        gm_block();
    }
    else if (try_word(1, Token::IFTK))
    {
        gm_if_stmt();
    }
    else if (try_word(1, Token::WHILETK))
    {
        gm_while_stmt();
    }
    else if (try_word(1, Token::BREAK))
    {
        match_one_word(Token::BREAK);
        match_one_word(Token::SEMICN);

        irGenerator.ir_jump(
            mir::inst::JumpInstructionKind::Br,
            irGenerator.checkWhile()._endLabel,
            -1,
            std::nullopt,
            mir::inst::JumpKind::Loop
        );
    }
    else if (try_word(1, Token::CONTINUETK))
    {
        match_one_word(Token::CONTINUETK);
        match_one_word(Token::SEMICN);

        irGenerator.ir_jump(
            mir::inst::JumpInstructionKind::Br,
            irGenerator.checkWhile()._beginLabel,
            -1,
            std::nullopt,
            mir::inst::JumpKind::Loop
        );
    }
    else if (try_word(1, Token::RETURNTK))
    {
        gm_return_stmt();
    }
    else if (find_word(Token::ASSIGN, Token::SEMICN))
    {
        gm_assign_stmt();
    }
    else if (!try_word(1, Token::SEMICN))
    {
        gm_exp();
    }
    else
    {
        match_one_word(Token::SEMICN);
    }
}

void SyntaxAnalyze::gm_if_stmt()
{
    SharedExNdPtr cond;
    LabelId ifTrue = irGenerator.getNewLabelId();
    LabelId ifFalse = irGenerator.getNewLabelId();
    std::optional<string> condStr;

    match_one_word(Token::IFTK);

    match_one_word(Token::LPARENT);

    cond = gm_cond();

    match_one_word(Token::RPARENT);

    if (cond->_type == NodeType::CONST)
    {
        LabelId tmpId = irGenerator.getNewTmpValueId();
        RightVal right;
        right.emplace<0>(cond->_value);
        irGenerator.ir_assign(to_string(tmpId), right);
        condStr = to_string(tmpId);
    }
    else
    {
        condStr = cond->_name;
    }
    irGenerator.ir_jump(
        mir::inst::JumpInstructionKind::BrCond,
        ifTrue,
        ifFalse,
        condStr,
        mir::inst::JumpKind::Branch
    );
    irGenerator.ir_label(ifTrue);

    gm_stmt();

    if (try_word(1, Token::ELSETK))
    {
        LabelId ifEnd = irGenerator.getNewLabelId();
        irGenerator.ir_jump(
            mir::inst::JumpInstructionKind::Br,
            ifEnd,
            -1,
            std::nullopt,
            mir::inst::JumpKind::Branch
        );
        irGenerator.ir_label(ifFalse);

        match_one_word(Token::ELSETK);
        gm_stmt();

        irGenerator.ir_label(ifEnd);
    }
    else
    {
        irGenerator.ir_label(ifFalse);
    }
}

void SyntaxAnalyze::gm_while_stmt()
{
    SharedExNdPtr cond;
    LabelId whileTrue = irGenerator.getNewLabelId();
    LabelId whileEnd = irGenerator.getNewLabelId();
    LabelId whileBegin = irGenerator.getNewLabelId();
    std::optional<string> condStr;
    irGenerator::WhileLabels whileLabels = irGenerator::WhileLabels(whileBegin, whileEnd);
    
    irGenerator.pushWhile(whileLabels);
    irGenerator.ir_label(whileBegin);

    match_one_word(Token::WHILETK);
    match_one_word(Token::LPARENT);

    cond = gm_cond();

    if (cond->_type == NodeType::CONST)
    {
        LabelId tmpId = irGenerator.getNewTmpValueId();
        RightVal right;
        right.emplace<0>(cond->_value);
        irGenerator.ir_assign(to_string(tmpId), right);
        condStr = to_string(tmpId);
    }
    else
    {
        condStr = cond->_name;
    }
    irGenerator.ir_jump(
        mir::inst::JumpInstructionKind::BrCond,
        whileTrue,
        whileEnd,
        condStr,
        mir::inst::JumpKind::Loop
    );
    irGenerator.ir_label(whileTrue);

    match_one_word(Token::RPARENT);
    gm_stmt();

    irGenerator.ir_jump(
        mir::inst::JumpInstructionKind::Br,
        whileBegin,
        -1,
        std::nullopt,
        mir::inst::JumpKind::Loop
    );
    irGenerator.ir_label(whileEnd);
    irGenerator.popWhile();
}

void SyntaxAnalyze::gm_return_stmt()
{
    std::optional<string> retStr;

    match_one_word(Token::RETURNTK);
    if (!try_word(1, Token::SEMICN))
    {
        SharedExNdPtr retValue;

        retValue = gm_exp();

        if (retValue->_type == NodeType::CONST)
        {
            LabelId tmpId = irGenerator.getNewTmpValueId();
            RightVal right;
            right.emplace<0>(retValue->_value);
            irGenerator.ir_assign(to_string(tmpId), right);
            retStr = to_string(tmpId);
        }
        else
        {
            retStr = retValue->_name;
        }
    }
    match_one_word(Token::SEMICN);

    irGenerator.ir_jump(
        mir::inst::JumpInstructionKind::Return,
        -1,
        -1,
        retStr,
        mir::inst::JumpKind::Undefined
    );
}

void SyntaxAnalyze::gm_assign_stmt()
{
    SharedExNdPtr lVal;
    SharedExNdPtr rVal;
    RightVal rightValue;

    lVal = gm_l_val();
    match_one_word(Token::ASSIGN);
    rVal = gm_exp();
    match_one_word(Token::SEMICN);

    if (rVal->_type == NodeType::CONST)
    {
        rightValue.emplace<0>(rVal->_value);
    }
    else
    {
        rightValue = rVal->_name;
    }
    irGenerator.ir_assign(lVal->_name, rightValue);
}

SharedExNdPtr SyntaxAnalyze::gm_cond()
{
    SharedExNdPtr first;
    first = gm_and_exp();

    while (try_word(1, Token::OR))
    {
        SharedExNdPtr second;
        SharedExNdPtr father(new ExpressNode());
        Op op;

        father->_operation = OperationType::OR;

        match_one_word(Token::OR);
        op = Op::Or;

        second = gm_and_exp();

        if (first->_type == NodeType::CONST && second->_type == NodeType::CONST)
        {
            father->_type = NodeType::CONST;
            father->_value = first->_value || second->_value;
        }
        else
        {
            LabelId tmpId;

            tmpId = irGenerator.getNewLabelId();
            father->_type = NodeType::VAR;
            father->_name = to_string(tmpId);

            hp_gn_binary_mir(tmpId, first, second, op);
        }
        father->addChild(first);
        father->addChild(second);
        first = father;
    }

    return first;
}

SharedExNdPtr SyntaxAnalyze::gm_and_exp()
{
    SharedExNdPtr first;
    first = gm_eq_exp();

    while (try_word(1, Token::AND))
    {
        SharedExNdPtr second;
        SharedExNdPtr father(new ExpressNode());
        Op op;

        father->_operation = OperationType::OR;

        match_one_word(Token::AND);
        op = Op::And;

        second = gm_eq_exp();

        if (first->_type == NodeType::CONST && second->_type == NodeType::CONST)
        {
            father->_type = NodeType::CONST;
            father->_value = first->_value && second->_value;
        }
        else
        {
            LabelId tmpId;

            tmpId = irGenerator.getNewLabelId();
            father->_type = NodeType::VAR;
            father->_name = to_string(tmpId);

            hp_gn_binary_mir(tmpId, first, second, op);
        }
        father->addChild(first);
        father->addChild(second);
        first = father;
    }
    return first;
}

SharedExNdPtr SyntaxAnalyze::gm_eq_exp()
{
    SharedExNdPtr first;
    first = gm_rel_exp();

    while (try_word(1, Token::EQL, Token::NEQ))
    {
        SharedExNdPtr second;
        SharedExNdPtr father(new ExpressNode());
        Op op;

        if (try_word(1, Token::EQL))
        {
            match_one_word(Token::EQL);
            father->_operation = OperationType::EQL;
            op = Op::Eq;
        }
        else
        {
            match_one_word(Token::NEQ);
            father->_operation = OperationType::NEQ;
            op = Op::Neq;
        }

        second = gm_rel_exp();

        if (first->_type == NodeType::CONST && second->_type == NodeType::CONST)
        {
            father->_type = NodeType::CONST;
            switch (father->_operation)
            {
            case OperationType::EQL:
                father->_value = first->_value == second->_value;
                break;
            case OperationType::NEQ:
                father->_value = first->_value != second->_value;
                break;
            default:
                break;
            }
        }
        else
        {
            LabelId tmpId;

            tmpId = irGenerator.getNewLabelId();
            father->_type = NodeType::VAR;
            father->_name = to_string(tmpId);

            hp_gn_binary_mir(tmpId, first, second, op);
        }

        father->addChild(first);
        father->addChild(second);
        first = father;
    }
    return first;
}

SharedExNdPtr SyntaxAnalyze::gm_rel_exp()
{
    SharedExNdPtr first;
    first = gm_exp();

    while (try_word(1, Token::LSS, Token::GRE) || try_word(1, Token::LEQ, Token::GEQ))
    {
        SharedExNdPtr second;
        SharedExNdPtr father(new ExpressNode());
        Op op;

        if (try_word(1, Token::LSS))
        {
            father->_operation = OperationType::LSS;
            match_one_word(Token::LSS);
            op = Op::Lt;
        }
        else if (try_word(1, Token::GRE))
        {
            father->_operation = OperationType::GRE;
            match_one_word(Token::GRE);
            op = Op::Gt;
        }
        else if (try_word(1, Token::LEQ))
        {
            father->_operation = OperationType::LEQ;
            match_one_word(Token::LEQ);
            op = Op::Lte;
        }
        else
        {
            father->_operation = OperationType::GEQ;
            match_one_word(Token::GEQ);
            op = Op::Gte;
        }

        second = gm_exp();
        if (first->_type == NodeType::CONST && second->_type == NodeType::CONST)
        {
            father->_type = NodeType::CONST;
            switch (father->_operation)
            {
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
        }
        else
        {
            LabelId tmpId;

            tmpId = irGenerator.getNewLabelId();
            father->_type = NodeType::VAR;
            father->_name = to_string(tmpId);

            hp_gn_binary_mir(tmpId, first, second, op);
        }

        father->addChild(first);
        father->addChild(second);
        first = father;
    }
    return first;
}

bool SyntaxAnalyze::try_word(int n, Token tk)
{
    bool return_value = false;
    if (matched_index + n < word_list.size() && word_list[matched_index + n].match_token(tk))
    {
        return_value = true;
    }
    return return_value;
}

bool SyntaxAnalyze::try_word(int n, Token tk1, Token tk2)
{
    bool return_value = false;
    if (matched_index + n < word_list.size())
    {
        return_value = try_word(n, tk1) || try_word(n, tk2);
    }
    return return_value;
}

bool SyntaxAnalyze::try_word(int n, Token tk1, Token tk2, Token tk3)
{
    bool return_value = false;
    if (matched_index + n < word_list.size())
    {
        return_value = try_word(n, tk1) || try_word(n, tk2) || try_word(n, tk3);
    }
    return return_value;
}

bool SyntaxAnalyze::find_word(Token tk, Token end)
{
    size_t size = word_list.size();
    bool r = false;
    for (size_t i = matched_index + 1; i < size; i++)
    {
        if (word_list[i].match_token(tk))
        {
            r = true;
            break;
        }
        else if (word_list[i].match_token(end))
        {
            break;
        }
    }
    return r;
}

bool SyntaxAnalyze::match_one_word(Token tk)
{
    bool return_value = false;
    if (matched_index + 1 < word_list.size())
    {
        if (word_list[matched_index + 1].match_token(tk))
        {
            matched_index++;
            return_value = true;
        }
    }
    return return_value;
}

void SyntaxAnalyze::in_layer()
{
    layer_num++;
}

void SyntaxAnalyze::out_layer()
{
    symbolTable.pop_layer_symbols(layer_num);
    layer_num--;
}

const Word& SyntaxAnalyze::get_least_matched_word()
{
    return word_list[matched_index];
}

const Word& SyntaxAnalyze::get_next_unmatched_word()
{
    if (matched_index + 1 < word_list.size())
    {
        return word_list[matched_index + 1];
    }
}

size_t SyntaxAnalyze::get_matched_index()
{
    return matched_index;
}

void SyntaxAnalyze::set_matched_index(size_t new_index)
{
    matched_index = new_index;
}

