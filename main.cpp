#include <fstream>
#include <iostream>

#include "backend/backend.hpp"
#include "backend/codegen/codegen.hpp"
#include "backend/optimization/graph_color.hpp"
#include "frontend/ir_generator.hpp"
#include "frontend/optim_mir.hpp"
#include "frontend/syntax_analyze.hpp"
#include "include/argparse/argparse.h"
#include "include/spdlog/include/spdlog/spdlog.h"
#include "mir/mir.hpp"
#include "opt.hpp"
#include "prelude/fake_mir_generate.hpp"
#include "spdlog/common.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using std::cout;
using std::endl;
using std::ifstream;
using std::istreambuf_iterator;
using std::ofstream;
using std::string;

const bool debug = false;

string& read_input();
Options parse_options(int argc, const char** argv);

int main(int argc, const char** argv) {
  auto options = parse_options(argc, argv);
  // frontend
  std::vector<front::word::Word> word_arr(VECTOR_SIZE);
  word_arr.clear();

  string& input_str = read_input();

  word_analyse(input_str, word_arr);
  delete &input_str;

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

  std::cout << "Mir" << std::endl << package << std::endl;

  spdlog::info("generating ARM code");

  backend::Backend backend(package);
  auto code = backend.generate_code();
  std::cout << "CODE:" << std::endl << code;

  spdlog::info("writing to output file: {}", options.out_file);

  ofstream output_file(options.out_file);
  output_file << code << std::endl;
  return 0;
}

string& read_input() {
  ifstream input;
  input.open("testfile.txt");
  string* input_str_p =
      new string(istreambuf_iterator<char>(input), istreambuf_iterator<char>());
  return *input_str_p;
}

Options parse_options(int argc, const char** argv) {
  argparse::ArgumentParser parser(
      "compiler", "Compiler for SysY language, by SEGVIOL team.");

  parser.add_argument("-o", "--output", "Output file", false);
  parser.add_argument().names({"-v", "--verbose"}).description("Set verbosity");
  parser.enable_help();

  parser.parse(argc, argv);

  if (parser.exists("help")) {
    parser.print_help();
    exit(0);
  }

  Options options;
  if (parser.exists("output")) {
    auto out = parser.get<std::string>("output");
    options.out_file = out;
  } else {
    options.out_file = "out.s";
  }

  if (parser.exists("verbosity")) {
  } else {
    spdlog::set_level(spdlog::level::warn);
  }
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_default_logger(spdlog::stderr_color_st("console"));

  spdlog::info("output file is {}", options.out_file);

  return std::move(options);
}
