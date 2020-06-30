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
    }
    else
    {
        symbol.reset(new ArraySymbol(name, IntSymbol::getHolderIntSymbol(), layer_num, true, false));
        for (auto var : dimensions)
        {
            std::static_pointer_cast<ArraySymbol>(symbol)->addDimension(var);
        }
        for (auto var : init_values)
        {
            std::static_pointer_cast<ArraySymbol>(symbol)->addValue(var);
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
    }
    else
    {
        symbol.reset(new ArraySymbol(name, IntSymbol::getHolderIntSymbol(), layer_num, false, false));
        for (auto var : dimensions)
        {
            std::static_pointer_cast<ArraySymbol>(symbol)->addDimension(var);
        }
        // TODO: mir to init var
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
        string opStr;

        if (try_word(1, Token::PLUS))
        {
            match_one_word(Token::PLUS);
            father->_operation = OperationType::PLUS;
        }
        else
        {
            match_one_word(Token::MINU);
            father->_operation = OperationType::MINU;
        }
        opStr = get_least_matched_word().get_self();

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
            father->_type = NodeType::VAR;
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
        string opStr;

        if (try_word(1, Token::MULT))
        {
            match_one_word(Token::MULT);
            father->_operation = OperationType::MUL;
        }
        else if (try_word(1, Token::DIV))
        {
            match_one_word(Token::DIV);
            father->_operation = OperationType::DIV;
        }
        else
        {
            match_one_word(Token::MOD);
            father->_operation = OperationType::MOD;
        }
        opStr = get_least_matched_word().get_self();

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
            father->_type = NodeType::VAR;
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
    string opStr;

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
        node.reset(new ExpressNode());
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
                node->_type = NodeType::VAR;

                if (node->_operation == OperationType::UN_MINU)
                {
                }
                else
                {
                }
            }
            node->addChild(child);
        }
    }
    return node;
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

        while (try_word(1, Token::LBRACK))
        {
            match_one_word(Token::LBRACK);
            child = gm_exp();
            node->addChild(child);
            match_one_word(Token::RBRACK);
        }

        // TODO: gen ir to compute index

        node->_type = NodeType::VAR;
        node->_operation = OperationType::ARR;
        node->addChild(index);
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
        }
    }

    return node;
}

SharedExNdPtr SyntaxAnalyze::gm_func_call()
{
    SharedExNdPtr node(new ExpressNode());
    SharedSyPtr func;
    string name;
    vector<SharedExNdPtr> params;

    match_one_word(Token::IDENFR);
    name = get_least_matched_word().get_self();
    func = symbolTable.find_least_layer_symbol(name);

    // TODO: deal IO function
    match_one_word(Token::LPARENT);
    if (!try_word(1, Token::RPARENT))
    {
        gm_exp();

        while (try_word(1, Token::COMMA))
        {
            match_one_word(Token::COMMA);
            gm_exp();
        }
    }
    match_one_word(Token::RPARENT);

    if (std::static_pointer_cast<FunctionSymbol>(func)->getRet() == SymbolKind::INT)
    {
        node->_type = NodeType::VAR;
        node->_operation = OperationType::RETURN_FUNC_CALL;
        // TODO: init node name
    }
    else
    {
        node.reset();
    }

    // TODO: deal IO function

    if (name == "putf")
    {
        match_one_word(Token::LPARENT);

        match_one_word(Token::STRCON);
        while (try_word(1, Token::COMMA))
        {
            match_one_word(Token::COMMA);
            gm_exp();
        }
        match_one_word(Token::RPARENT);
    }
    else
    {
        match_one_word(Token::LPARENT);
        if (!try_word(1, Token::RPARENT))
        {
            SharedExNdPtr child;
            child = gm_exp();
            params.push_back(child);
            node->addChild(child);

            while (try_word(1, Token::COMMA))
            {
                match_one_word(Token::COMMA);
                child = gm_exp();
                params.push_back(child);
                node->addChild(child);
            }
        }
        match_one_word(Token::RPARENT);
    }
    return node;
}

void SyntaxAnalyze::gm_func_def()
{
    string name;
    SymbolKind ret;
    int funcLayerNum;
    vector<SharedSyPtr> params;
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

    match_one_word(Token::LPARENT);
    if (!try_word(1, Token::RPARENT))
    {
        gm_func_param(params, funcLayerNum);
        while (try_word(1, Token::COMMA))
        {
            match_one_word(Token::COMMA);
            gm_func_param(params, funcLayerNum);
        }
    }
    match_one_word(Token::RPARENT);

    func.reset(new FunctionSymbol(name, ret, funcLayerNum));
    symbolTable.push_symbol(func);
    for (auto var : params)
    {
        symbolTable.push_symbol(var);
    }

    gm_block();
}

void SyntaxAnalyze::gm_func_param(vector<SharedSyPtr>& params, int funcLayerNum)
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
                // TODO: generate a new var to save the variable dimension
            }
        }
    }
    else
    {
        symbol.reset(new IntSymbol(name, funcLayerNum + 1, false));
    }
    params.push_back(symbol);
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
    }
    else if (try_word(1, Token::CONTINUETK))
    {
        match_one_word(Token::CONTINUETK);
        match_one_word(Token::SEMICN);
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

    match_one_word(Token::IFTK);

    match_one_word(Token::LPARENT);

    cond = gm_cond();

    match_one_word(Token::RPARENT);

    gm_stmt();

    if (try_word(1, Token::ELSETK))
    {
        match_one_word(Token::ELSETK);

        gm_stmt();
    }
}

void SyntaxAnalyze::gm_while_stmt()
{
    SharedExNdPtr cond;

    match_one_word(Token::WHILETK);
    match_one_word(Token::LPARENT);

    cond = gm_cond();

    match_one_word(Token::RPARENT);

    gm_stmt();
}

void SyntaxAnalyze::gm_return_stmt()
{
    match_one_word(Token::RETURNTK);
    if (!try_word(1, Token::SEMICN))
    {
        SharedExNdPtr retValue;
        retValue = gm_exp();
    }
    match_one_word(Token::SEMICN);
}

void SyntaxAnalyze::gm_assign_stmt()
{
    SharedExNdPtr lVal;
    SharedExNdPtr rVal;

    lVal = gm_l_val();
    match_one_word(Token::ASSIGN);
    rVal = gm_exp();
    match_one_word(Token::SEMICN);
}

SharedExNdPtr SyntaxAnalyze::gm_cond()
{
    SharedExNdPtr first;
    first = gm_and_exp();

    while (try_word(1, Token::OR))
    {
        SharedExNdPtr second;
        SharedExNdPtr father(new ExpressNode());
        string opStr;

        father->_operation = OperationType::OR;

        match_one_word(Token::OR);
        opStr = get_least_matched_word().get_self();

        second = gm_and_exp();

        if (first->_type == NodeType::CONST && second->_type == NodeType::CONST)
        {
            father->_type = NodeType::CONST;
            father->_value = first->_value || second->_value;
        }
        else
        {
            father->_type = NodeType::VAR;
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
        string opStr;

        father->_operation = OperationType::OR;

        match_one_word(Token::AND);
        opStr = get_least_matched_word().get_self();

        second = gm_eq_exp();

        if (first->_type == NodeType::CONST && second->_type == NodeType::CONST)
        {
            father->_type = NodeType::CONST;
            father->_value = first->_value && second->_value;
        }
        else
        {
            father->_type = NodeType::VAR;
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
        string opStr;

        if (try_word(1, Token::EQL))
        {
            match_one_word(Token::EQL);
            father->_operation = OperationType::EQL;
        }
        else
        {
            match_one_word(Token::NEQ);
            father->_operation = OperationType::NEQ;
        }
        opStr = get_least_matched_word().get_self();

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
            father->_type = NodeType::VAR;
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
        string opStr;

        if (try_word(1, Token::LSS))
        {
            father->_operation = OperationType::LSS;
            match_one_word(Token::LSS);
        }
        else if (try_word(1, Token::GRE))
        {
            father->_operation = OperationType::GRE;
            match_one_word(Token::GRE);
        }
        else if (try_word(1, Token::LEQ))
        {
            father->_operation = OperationType::LEQ;
            match_one_word(Token::LEQ);
        }
        else
        {
            father->_operation = OperationType::GEQ;
            match_one_word(Token::GEQ);
        }
        opStr = get_least_matched_word().get_self();

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
            father->_type = NodeType::VAR;
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

bool SyntaxAnalyze::in_global_scope()
{
    return layer_num == 1;
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

