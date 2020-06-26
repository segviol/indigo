#include "syntax_analyze.hpp"

using namespace front::syntax;

vector<Word> tmp_vect_word;

SyntaxAnalyze::SyntaxAnalyze()
    : matched_index(-1), word_list(tmp_vect_word)
{}

SyntaxAnalyze::SyntaxAnalyze(vector<Word>& in_word_list)
    : matched_index(-1), word_list(in_word_list)
{
    //output.open("output.txt");
}

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

    match_one_word(Token::IDENFR);

    if (try_word(1, Token::LBRACK))
    {
        while (try_word(1, Token::LBRACK))
        {
            match_one_word(Token::LBRACK);
            match_one_word(Token::RBRACK);
        }
    }

    match_one_word(Token::ASSIGN);

    gm_const_init_val();
}

void SyntaxAnalyze::gm_const_init_val()
{
    if (try_word(1, Token::LBRACE))
    {
        match_one_word(Token::LBRACE);
        if (!try_word(1, Token::RBRACE))
        {
            gm_const_init_val();
            while (try_word(1, Token::COMMA))
            {
                match_one_word(Token::COMMA);
                gm_const_init_val();
            }
        }
        match_one_word(Token::RBRACE);
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
    match_one_word(Token::IDENFR);

    if (try_word(1, Token::LBRACK))
    {
        while (try_word(1, Token::LBRACK))
        {
            match_one_word(Token::LBRACK);
            match_one_word(Token::RBRACK);
        }
    }

    if (try_word(1, Token::ASSIGN))
    {
        match_one_word(Token::ASSIGN);
        gm_init_val();
    }
}

void SyntaxAnalyze::gm_init_val()
{
    if (try_word(1, Token::LBRACE))
    {
        match_one_word(Token::LBRACE);
        if (!try_word(1, Token::RBRACE))
        {
            gm_init_val();
            while (try_word(1, Token::COMMA))
            {
                match_one_word(Token::COMMA);
                gm_init_val();
            }
        }
        match_one_word(Token::RBRACE);
    }
}


void SyntaxAnalyze::gm_const_exp()
{
    return gm_exp();
}

void SyntaxAnalyze::gm_exp()
{
    gm_mul_exp();

    while (try_word(1, Token::PLUS, Token::MINU))
    {
        if (try_word(1, Token::PLUS))
        {
            match_one_word(Token::PLUS);
        }
        else
        {
            match_one_word(Token::MINU);
        }
    }
}

void SyntaxAnalyze::gm_mul_exp()
{
    gm_unary_exp();

    while (try_word(1, Token::MULT, Token::DIV, Token::MOD))
    {
        if (try_word(1, Token::MULT))
        {
            match_one_word(Token::MULT);
        }
        else if (try_word(1, Token::DIV))
        {
            match_one_word(Token::DIV);
        }
        else
        {
            match_one_word(Token::MOD);
        }
    }
}

void SyntaxAnalyze::gm_unary_exp()
{
    if (try_word(1, Token::LPARENT))
    {
        match_one_word(Token::LPARENT);

        gm_exp();

        match_one_word(Token::RPARENT);
    }
    else if (try_word(1, Token::IDENFR))
    {
        if (try_word(2, Token::LPARENT))
        {
            gm_func_call();
        }
        else
        {
            gm_l_val();
        }
    }
    else if (try_word(1, Token::INTCON))
    {
        match_one_word(Token::INTCON);
    }
    else
    {
        if (try_word(1, Token::PLUS, Token::MINU, Token::NOT))
        {
            if (try_word(1, Token::PLUS))
            {
                match_one_word(Token::PLUS);
            }
            else if (try_word(1, Token::MINU))
            {
                match_one_word(Token::MINU);
            }
            else
            {
                match_one_word(Token::NOT);
            }
        }

        gm_unary_exp();
    }
}

void SyntaxAnalyze::gm_l_val()
{
    match_one_word(Token::IDENFR);

    if (try_word(1, Token::LBRACK))
    {
        while (try_word(1, Token::LBRACK))
        {
            match_one_word(Token::LBRACK);
            match_one_word(Token::RBRACK);
        }
    }
}

void SyntaxAnalyze::gm_func_call()
{
    match_one_word(Token::IDENFR);

    // TODO: deal IO function and redefine ir
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
}

void SyntaxAnalyze::gm_func_def()
{
    if (try_word(1, Token::INTTK))
    {
        match_one_word(Token::INTTK);
    }
    else
    {
        match_one_word(Token::VOIDTK);
    }

    match_one_word(Token::IDENFR);

    match_one_word(Token::LPARENT);
    if (!try_word(1, Token::RPARENT))
    {
        while (try_word(1, Token::COMMA))
        {
            match_one_word(Token::COMMA);
        }
    }
    match_one_word(Token::RPARENT);

    gm_block();
}

void SyntaxAnalyze::gm_func_param()
{
    match_one_word(Token::INTTK);
    match_one_word(Token::IDENFR);

    if (try_word(1, Token::LBRACK))
    {
        match_one_word(Token::LBRACK);
        match_one_word(Token::RBRACK);

        while (try_word(1, Token::LBRACK))
        {
            match_one_word(Token::LBRACK);
            gm_exp();
            match_one_word(Token::RBRACK);
            // TODO: generate a new var to save the variable dimension
        }
    }
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
    match_one_word(Token::IFTK);

    match_one_word(Token::LPARENT);

    gm_cond();

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
    match_one_word(Token::WHILETK);
    match_one_word(Token::LPARENT);

    gm_cond();

    match_one_word(Token::RPARENT);

    gm_stmt();
}

void SyntaxAnalyze::gm_return_stmt()
{
    match_one_word(Token::RETURNTK);
    if (!try_word(1, Token::SEMICN))
    {
        gm_exp();
    }
    match_one_word(Token::SEMICN);
}

void SyntaxAnalyze::gm_assign_stmt()
{
    gm_l_val();
    match_one_word(Token::ASSIGN);
    gm_exp();
    match_one_word(Token::SEMICN);
}

void SyntaxAnalyze::gm_cond()
{
    gm_and_exp();

    while (try_word(1, Token::OR))
    {
        match_one_word(Token::OR);

        gm_and_exp();
    }
}

void SyntaxAnalyze::gm_and_exp()
{
    gm_eq_exp();

    while (try_word(1, Token::AND))
    {
        match_one_word(Token::AND);

        gm_eq_exp();
    }
}

void SyntaxAnalyze::gm_eq_exp()
{
    gm_rel_exp();

    while (try_word(1, Token::EQL, Token::NEQ))
    {
        if (try_word(1, Token::EQL))
        {
            match_one_word(Token::EQL);
        }
        else
        {
            match_one_word(Token::NEQ);
        }

        gm_rel_exp();
    }
}

void SyntaxAnalyze::gm_rel_exp()
{
    gm_exp();

    while (try_word(1, Token::LSS, Token::GRE) || try_word(1, Token::LEQ, Token::GEQ))
    {
        if (try_word(1, Token::LSS))
        {
            match_one_word(Token::LSS);
        }
        else if (try_word(1, Token::GRE))
        {
            match_one_word(Token::GRE);
        }
        else if (try_word(1, Token::LEQ))
        {
            match_one_word(Token::LEQ);
        }
        else
        {
            match_one_word(Token::GEQ);
        }

        gm_exp();
    }
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

void SyntaxAnalyze::set_matched_idnex(size_t new_index)
{
    matched_index = new_index;
}

