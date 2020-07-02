#pragma once

#include <string>
#include <vector>
#include <fstream>

#include "../mir/mir.hpp"

#include "word_analyze.hpp"
#include "symbol.hpp"
#include "symbol_table.hpp"
#include "express_node.hpp"
#include "ir_generator.hpp"

#ifndef COMPILER_FRONT_SYNTAX_H_
#define COMPILER_FRONT_SYNTAX_H_

namespace front::syntax {

    using std::string;
    using std::to_string;
    using std::vector;
    using std::ofstream;
    using std::endl;
    using std::stoi;

    using mir::types::LabelId;
    using mir::inst::Op;

    using word::Word;
    using word::Token;
    using symbolTable::SymbolTable;
    using symbol::SharedSyPtr;
    using symbol::SymbolKind;
    using symbol::Symbol;
    using symbol::IntSymbol;
    using symbol::ArraySymbol;
    using symbol::FunctionSymbol;
    using express::SharedExNdPtr;
    using express::ExpressNode;
    using express::NodeType;
    using express::OperationType;
    using irGenerator::RightVal;

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
        unsigned int _genValueNum = 0;

        const vector<Word>& word_list;
        SymbolTable symbolTable;
        irGenerator::irGenerator irGenerator;

        bool try_word(int n, Token tk);
        bool try_word(int n, Token tk1, Token tk2);
        bool try_word(int n, Token tk1, Token tk2, Token tk3);
        bool find_word(Token tk, Token end);
        bool match_one_word(Token tk);
        const Word& get_least_matched_word();
        const Word& get_next_unmatched_word();

        void in_layer();
        void out_layer();

        void gm_const_decl();
        void gm_const_def();
        void gm_const_init_val(vector<SharedExNdPtr>& init_values);

        void gm_var_decl();
        void gm_var_def();
        void gm_init_val(vector<SharedExNdPtr>& init_values);

        void gm_func_def();
        void gm_func_param(vector<SharedSyPtr>& params, int funcLayerNum, vector<std::pair<SharedSyPtr, string>>& genValues);
        void gm_block();
        void gm_block_item();
        void gm_stmt();
        void gm_if_stmt();
        void gm_while_stmt();
        void gm_return_stmt();
        void gm_assign_stmt();

        SharedExNdPtr computeIndex(SharedSyPtr arr, SharedExNdPtr node);
        void hp_gn_binary_mir(LabelId tmpId, SharedExNdPtr first, SharedExNdPtr second, Op op);
        string hp_gen_save_value();

        SharedExNdPtr gm_cond();
        SharedExNdPtr gm_and_exp();
        SharedExNdPtr gm_eq_exp();
        SharedExNdPtr gm_rel_exp();

        SharedExNdPtr gm_const_exp();
        SharedExNdPtr gm_exp();
        SharedExNdPtr gm_l_val();
        SharedExNdPtr gm_mul_exp();
        SharedExNdPtr gm_unary_exp();
        SharedExNdPtr gm_func_call();

        size_t get_matched_index();
        void set_matched_index(size_t new_index);
    };
}

#endif // !COMPILER_FRONT_SYNTAX_H_
