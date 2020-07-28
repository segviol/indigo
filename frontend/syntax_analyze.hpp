#include <assert.h>

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "../mir/mir.hpp"
#include "express_node.hpp"
#include "ir_generator.hpp"
#include "symbol.hpp"
#include "symbol_table.hpp"
#include "word_analyze.hpp"

#ifndef COMPILER_FRONT_SYNTAX_H_
#define COMPILER_FRONT_SYNTAX_H_

namespace front::syntax {

using std::endl;
using std::ofstream;
using std::stoi;
using std::string;
using std::to_string;
using std::vector;

using mir::inst::Op;
using mir::types::LabelId;
using mir::types::TyKind;

using express::ExpressNode;
using express::NodeType;
using express::OperationType;
using express::SharedExNdPtr;
using irGenerator::RightVal;
using symbol::ArraySymbol;
using symbol::FunctionSymbol;
using symbol::IntSymbol;
using symbol::SharedSyPtr;
using symbol::Symbol;
using symbol::SymbolKind;
using symbolTable::SymbolTable;
using word::Token;
using word::Word;

enum class ValueMode { right, left };

class SyntaxAnalyze {
 public:
  SyntaxAnalyze();
  SyntaxAnalyze(vector<Word> &in_word_list);
  ~SyntaxAnalyze();

  void gm_comp_unit();

  void outputInstructions(std::ostream &out) {
    irGenerator.outputInstructions(out);
  }

  irGenerator::irGenerator &getIrGenerator() { return irGenerator; }

 private:
  const std::uint32_t _initLayerNum = 0;

  size_t matched_index = -1;
  unsigned int layer_num = _initLayerNum;
  unsigned int _genValueNum = 0;

  const vector<Word> &word_list;
  SymbolTable symbolTable;
  irGenerator::irGenerator irGenerator;

  bool try_word(int n, Token tk);
  bool try_word(int n, Token tk1, Token tk2);
  bool try_word(int n, Token tk1, Token tk2, Token tk3);
  bool find_word(Token tk, Token end);
  bool match_one_word(Token tk);
  const Word &get_least_matched_word();
  const Word &get_next_unmatched_word();

  void in_layer();
  void out_layer();
  bool inGlobalLayer();

  void gm_const_decl();
  void gm_const_def();
  uint32_t gm_const_init_val(vector<SharedExNdPtr> &init_values,
                             vector<SharedExNdPtr> &dimensions,
                             uint32_t braceLayerNum);

  void gm_var_decl();
  void gm_var_def();
  uint32_t gm_init_val(vector<SharedExNdPtr> &init_values,
                       vector<SharedExNdPtr> &dimensions,
                       uint32_t braceLayerNum);

  void gm_func_def();
  void gm_func_param(vector<SharedSyPtr> &params, int funcLayerNum,
                     vector<std::pair<SharedSyPtr, string>> &genValues);
  void gm_block();
  void gm_block_item();
  void gm_stmt();
  void gm_if_stmt();
  void gm_while_stmt();
  void gm_while_stmt_normal();
  void gm_while_stmt_flat();
  void gm_return_stmt();
  void gm_assign_stmt();

  SharedExNdPtr computeIndex(SharedSyPtr arr, SharedExNdPtr node);
  void hp_gn_binary_mir(string tmpName, SharedExNdPtr first,
                        SharedExNdPtr second, Op op);
  string hp_gen_save_value();
  void hp_init_external_function();

  SharedExNdPtr gm_cond();
  SharedExNdPtr gm_and_exp();
  SharedExNdPtr gm_eq_exp();
  SharedExNdPtr gm_rel_exp();

  SharedExNdPtr gm_const_exp();
  SharedExNdPtr gm_exp();
  SharedExNdPtr gm_l_val(ValueMode mode);
  SharedExNdPtr gm_mul_exp();
  SharedExNdPtr gm_unary_exp();
  SharedExNdPtr gm_func_call();

  size_t get_matched_index();
  void set_matched_index(size_t new_index);
};
}  // namespace front::syntax

#endif  // !COMPILER_FRONT_SYNTAX_H_
