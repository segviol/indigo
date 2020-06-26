#pragma once

#include <string>
#include <vector>
#include <fstream>

#include "word_analyze.hpp"

#ifndef COMPILER_FRONT_SYNTAX_H_
#define COMPILER_FRONT_SYNTAX_H_

namespace front::syntax {

    using std::string;
    using std::vector;
    using std::ofstream;
    using std::endl;
    using std::stoi;

    using front::word::Word;
    using front::word::Token;

    class SyntaxAnalyze
    {
    public:
        SyntaxAnalyze();
        SyntaxAnalyze(vector<Word>& in_word_list);
        ~SyntaxAnalyze();

        void gm_comp_unit();
    private:
        size_t matched_index = -1;
        unsigned int layer_num = 0;

        const vector<Word>& word_list;

        int _if_num = 0;
        int _while_num = 0;

        bool try_word(int n, Token tk);
        bool try_word(int n, Token tk1, Token tk2);
        bool try_word(int n, Token tk1, Token tk2, Token tk3);
        bool find_word(Token tk, Token end);
        bool match_one_word(Token tk);
        const Word& get_least_matched_word();
        const Word& get_next_unmatched_word();

        void in_layer();
        void out_layer();
        bool in_global_scope();

        void gm_const_decl();
        void gm_const_def();
        void gm_const_init_val();

        void gm_var_decl();
        void gm_var_def();
        void gm_init_val();

        void gm_func_def();
        void gm_func_param();
        void gm_block();
        void gm_block_item();
        void gm_stmt();
        void gm_if_stmt();
        void gm_while_stmt();
        void gm_return_stmt();
        void gm_assign_stmt();

        void gm_cond();
        void gm_and_exp();
        void gm_eq_exp();
        void gm_rel_exp();

        void gm_const_exp();
        void gm_exp();
        void gm_l_val();
        void gm_mul_exp();
        void gm_unary_exp();
        void gm_func_call();

        size_t get_matched_index();
        void set_matched_idnex(size_t new_index);
    };
}

#endif // !COMPILER_FRONT_SYNTAX_H_
