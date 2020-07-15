#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "backend/backend.hpp"
#include "backend/codegen/bb_rearrange.hpp"
#include "backend/codegen/codegen.hpp"
#include "backend/codegen/math_opt.hpp"
#include "backend/codegen/reg_alloc.hpp"
#include "backend/optimization/common_expression_delete.hpp"
#include "backend/optimization/graph_color.hpp"
#include "backend/optimization/inline.hpp"
#include "backend/optimization/remove_dead_code.hpp"
#include "frontend/ir_generator.hpp"
#include "frontend/optim_mir.hpp"
#include "frontend/syntax_analyze.hpp"
#include "include/argparse/argparse.hpp"
#include "include/spdlog/include/spdlog/spdlog.h"
#include "mir/mir.hpp"
#include "opt.hpp"
#include "prelude/fake_mir_generate.hpp"
#include "spdlog/common.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/stdout_sinks.h"
using std::cout;
using std::endl;
using std::ifstream;
using std::istreambuf_iterator;
using std::ofstream;
using std::string;

const bool debug = false;

string read_input(std::string&);

Options parse_options(int argc, const char** argv);

int main(int argc, const char** argv) {
  auto options = parse_options(argc, argv);
  // frontend
  std::vector<front::word::Word> word_arr(VECTOR_SIZE);
  word_arr.clear();

  string input_str = read_input(options.in_file);

  word_analyse(input_str, word_arr);

  front::syntax::SyntaxAnalyze syntax_analyze(word_arr);
  syntax_analyze.gm_comp_unit();

  syntax_analyze.outputInstructions(std::cout);

  front::irGenerator::irGenerator& irgenerator =
      syntax_analyze.getIrGenerator();
  std::map<string, std::vector<front::irGenerator::Instruction>> inst =
      irgenerator.getfuncNameToInstructions();
  mir::inst::MirPackage& package = irgenerator.getPackage();

  spdlog::info("generating SSA");

  gen_ssa(inst, package, irgenerator);

  // std::cout << "Mir" << std::endl << package << std::endl;
  spdlog::info("Mir_Before");
  std::cout << package << std::endl;
  spdlog::info("generating ARM code");

  backend::Backend backend(package, options);

  // backend.add_pass(std::make_unique<optimization::graph_color::Graph_Color>());
  backend.add_pass(
      std::make_unique<optimization::remove_dead_code::Remove_Dead_Code>());
  backend.add_pass(
      std::make_unique<optimization::common_expr_del::Common_Expr_Del>());
  backend.add_pass(std::make_unique<optimization::inlineFunc::Inline_Func>());
  backend.add_pass(std::make_unique<backend::codegen::BasicBlkRearrange>());
  // backend.add_pass(std::make_unique<optimization::graph_color::Graph_Color>(5));
  backend.add_pass(std::make_unique<backend::codegen::MathOptimization>());
  backend.add_pass(std::make_unique<backend::codegen::RegAllocatePass>());

  auto code = backend.generate_code();
  std::cout << "CODE:" << std::endl << code;

  spdlog::info("writing to output file: {}", options.out_file);

  ofstream output_file(options.out_file);
  output_file << code << std::endl;
  return 0;
}

string read_input(std::string& input_filename) {
  ifstream input;
  input.open(input_filename);
  auto in = std::string(istreambuf_iterator<char>(input),
                        istreambuf_iterator<char>());
  return std::move(in);
}

Options parse_options(int argc, const char** argv) {
  Options options;

  argparse::ArgumentParser parser("compiler", "0.1.0");
  parser.add_description("Compiler for SysY language, by SEGVIOL team.");

  parser.add_argument("input").help("Input file").required();
  parser.add_argument("-o", "--output")
      .help("Output file")
      .nargs(1)
      .default_value(std::string("out.s"));
  parser.add_argument("-v", "--verbose")
      .help("Set verbosity")
      .default_value(false)
      .implicit_value(true);
  parser.add_argument("-d", "--pass-diff")
      .help("Show code difference after each pass")
      .default_value(false)
      .implicit_value(true);
  parser.add_argument("-r", "--run-pass").help("Only run pass");
  parser.add_argument("-s", "--skip-pass").help("Skip pass");
  parser.add_argument("-S", "--asm")
      .help("Emit assembly code (no effect)")
      .implicit_value(true)
      .default_value(false);
  parser.add_argument("-O", "--optimize")
      .help("Optimize code (no effect)")
      .implicit_value(true)
      .default_value(false);
  parser.add_argument("-O2", "--optimize-2")
      .help("Optimize code (no effect)")
      .implicit_value(true)
      .default_value(false);

  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cout << err.what() << std::endl;
    std::cout << parser;
    exit(0);
  }

  options.in_file = parser.get<std::string>("input");
  options.out_file = parser.get<std::string>("--output");

  options.show_code_after_each_pass = parser.get<bool>("--pass-diff");

  if (parser.present("--run-pass")) {
    auto out = parser.get<std::vector<string>>("run-pass");
    std::set<std::string> run_pass(out.begin(), out.end());
    {
      std::stringstream pass_name;
      for (auto i = run_pass.cbegin(); i != run_pass.cend(); i++) {
        if (i != run_pass.cbegin()) pass_name << ", ";
        pass_name << *i;
      }
      spdlog::info("Only running the following passes: {}", pass_name.str());
    }
    options.run_pass = {std::move(run_pass)};
  } else {
    options.run_pass = {};
  }

  if (parser.present("--skip-pass")) {
    auto out = parser.get<std::vector<string>>("skip-pass");
    std::set<std::string> skip_pass(out.begin(), out.end());
    {
      std::stringstream pass_name;
      for (auto i = skip_pass.cbegin(); i != skip_pass.cend(); i++) {
        if (i != skip_pass.cbegin()) pass_name << ", ";
        pass_name << *i;
      }
      spdlog::info("Skipping the following passes: {}", pass_name.str());
    }
    options.skip_pass = std::move(skip_pass);
  } else {
    options.skip_pass = {};
  }

  spdlog::set_default_logger(spdlog::stdout_logger_st("console"));
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] %^[%l]%$ %v");

  spdlog::info("input file is {}", options.in_file);
  spdlog::info("output file is {}", options.out_file);

  return std::move(options);
}
