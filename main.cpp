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
#include "backend/optimization/algebraic_simplification.hpp"
#include "backend/optimization/block_merge.hpp"
#include "backend/optimization/cast_inst.hpp"
#include "backend/optimization/common_expression_delete.hpp"
#include "backend/optimization/const_merge.hpp"
#include "backend/optimization/const_propagation.hpp"
#include "backend/optimization/excess_reg_delete.hpp"
#include "backend/optimization/global_expression_move.hpp"
#include "backend/optimization/graph_color.hpp"
#include "backend/optimization/inline.hpp"
#include "backend/optimization/memvar_propagation.hpp"
#include "backend/optimization/ref_count.hpp"
#include "backend/optimization/remove_dead_code.hpp"
#include "backend/optimization/remove_temp_var.hpp"
#include "backend/optimization/value_shift_collapse.hpp"
#include "backend/optimization/var_mir_fold.hpp"
#include "frontend/ir_generator.hpp"
#include "frontend/optim_mir.hpp"
#include "frontend/syntax_analyze.hpp"
#include "include/aixlog.hpp"
#include "include/argparse/argparse.hpp"
#include "mir/mir.hpp"
#include "opt.hpp"
#include "prelude/fake_mir_generate.hpp"

using std::cout;
using std::endl;
using std::ifstream;
using std::istreambuf_iterator;
using std::ofstream;
using std::string;

const bool debug = false;

string read_input(std::string&);
Options parse_options(int argc, const char** argv);
void add_passes_master(backend::Backend& backend) {
  backend.add_pass(
      std::make_unique<optimization::remove_dead_code::Remove_Dead_Code>());
  // backend.add_pass(std::make_unique<optimization::inlineFunc::Inline_Func>());
  backend.add_pass(std::make_unique<optimization::mergeBlocks::Merge_Block>());
  // backend.add_pass(
  //     std::make_unique<optimization::common_expr_del::Common_Expr_Del>());
  backend.add_pass(std::make_unique<backend::codegen::BasicBlkRearrange>());
  backend.add_pass(std::make_unique<optimization::graph_color::Graph_Color>(7));
  backend.add_pass(std::make_unique<backend::codegen::MathOptimization>());
  backend.add_pass(std::make_unique<backend::codegen::RegAllocatePass>());
  backend.add_pass(std::make_unique<backend::optimization::ExcessRegDelete>());
}
void add_passes(backend::Backend& backend) {
  backend.add_pass(
      std::make_unique<optimization::remove_temp_var::Remove_Temp_Var>());
  backend.add_pass(
      std::make_unique<optimization::const_propagation::Const_Propagation>());
  backend.add_pass(std::make_unique<optimization::var_mir_fold::VarMirFold>());
  backend.add_pass(
      std::make_unique<optimization::remove_dead_code::Remove_Dead_Code>());
  backend.add_pass(std::make_unique<optimization::inlineFunc::Inline_Func>());
  backend.add_pass(std::make_unique<optimization::mergeBlocks::Merge_Block>());
  // inside block only and remove tmp vars
  backend.add_pass(
      std::make_unique<optimization::common_expr_del::Common_Expr_Del>());
  backend.add_pass(
      std::make_unique<optimization::global_expr_move::Global_Expr_Mov>());
  // delete common exprs new created and replace not phi vars
  backend.add_pass(
      std::make_unique<optimization::common_expr_del::Common_Expr_Del>());
  backend.add_pass(std::make_unique<
                   optimization::memvar_propagation::Memory_Var_Propagation>());
  backend.add_pass(std::make_unique<optimization::const_merge::Merge_Const>());
  backend.add_pass(std::make_unique<
                   optimization::memvar_propagation::Memory_Var_Propagation>());
  backend.add_pass(
      std::make_unique<optimization::const_propagation::Const_Propagation>());
  // backend.add_pass(
  //     std::make_unique<optimization::remove_dead_code::Remove_Dead_Code>());
  backend.add_pass(std::make_unique<optimization::cast_inst::Cast_Inst>());
  backend.add_pass(
      std::make_unique<optimization::remove_dead_code::Remove_Dead_Code>());
  backend.add_pass(std::make_unique<optimization::ref_count::Ref_Count>());
  backend.add_pass(
      std::make_unique<
          optimization::algebraic_simplification::AlgebraicSimplification>());
  backend.add_pass(std::make_unique<
                   optimization::value_shift_collapse::ValueShiftCollapse>());
  backend.add_pass(
      std::make_unique<optimization::remove_dead_code::Remove_Dead_Code>());
  backend.add_pass(std::make_unique<backend::codegen::BasicBlkRearrange>());
  backend.add_pass(
      std::make_unique<optimization::graph_color::Graph_Color>(7, true));

  // ARM Passes
  backend.add_pass(std::make_unique<backend::codegen::MathOptimization>());
  backend.add_pass(std::make_unique<backend::codegen::RegAllocatePass>());
  backend.add_pass(std::make_unique<backend::optimization::ExcessRegDelete>());
}

int main(int argc, const char** argv) {
  auto options = parse_options(argc, argv);

  if (options.dry_run) {
    // Only show which passes will be run
    auto pkg = mir::inst::MirPackage();
    backend::Backend backend(pkg, options);
    add_passes_master(backend);
    backend.show_passes(std::cout);
    return 0;
  } else {
    // Run.
    // ==== Frontend ====
    std::vector<front::word::Word> word_arr(VECTOR_SIZE);
    word_arr.clear();

    string input_str = read_input(options.in_file);

    word_analyse(input_str, word_arr);

    front::syntax::SyntaxAnalyze syntax_analyze(word_arr);
    syntax_analyze.gm_comp_unit();

    if (options.verbose) syntax_analyze.outputInstructions(std::cout);

    front::irGenerator::irGenerator& irgenerator =
        syntax_analyze.getIrGenerator();
    std::map<string, std::vector<front::irGenerator::Instruction>> inst =
        irgenerator.getfuncNameToInstructions();
    mir::inst::MirPackage& package = irgenerator.getPackage();

    LOG(INFO) << "generating SSA" << std::endl;

    gen_ssa(inst, package, irgenerator);

    // LOG(TRACE) << "Mir" << std::endl << package << std::endl;
    LOG(INFO) << ("Mir_Before") << std::endl;
    if (options.verbose) std::cout << package << std::endl;
    LOG(INFO) << ("generating ARM code") << std::endl;

    // ==== Backend ====

    backend::Backend backend(package, options);
    add_passes_master(backend);
    auto code = backend.generate_code();
    if (options.verbose) {
      LOG(TRACE) << "CODE:" << std::endl;
      std::cout << code;
    }

    LOG(INFO) << "writing to output file: " << options.out_file;

    ofstream output_file(options.out_file);
    output_file << code << std::endl;
    return 0;
  }
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
  parser.add_argument("--dry-run")
      .help("Dry run. Show the sequence of passes but does not generate code")
      .implicit_value(true)
      .default_value(false);

  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cout << err.what() << std::endl;
    std::cout << parser;
    exit(0);
  }

  AixLog::Severity lvl;
  if (parser.get<bool>("--verbose")) {
    lvl = AixLog::Severity::trace;
    options.verbose = true;
  } else {
    lvl = AixLog::Severity::info;
    options.verbose = false;
  }
  AixLog::Log::init<AixLog::SinkCout>(lvl);

  options.in_file = parser.get<std::string>("input");
  options.out_file = parser.get<std::string>("--output");

  options.show_code_after_each_pass = parser.get<bool>("--pass-diff");
  options.dry_run = parser.get<bool>("--dry-run");

  if (parser.present("--run-pass")) {
    auto out = parser.get<std::string>("--run-pass");
    std::set<std::string> run_pass;
    {
      int low = 0;
      for (int i = 0; i < out.size(); i++) {
        if (out[i] == ',') {
          std::string item = out.substr(low, i);
          run_pass.insert(item);
          low = i + 1;
        }
      }
      std::string item = out.substr(low, out.size());
      run_pass.insert(item);
    }
    {
      std::stringstream pass_name;
      for (auto i = run_pass.cbegin(); i != run_pass.cend(); i++) {
        if (i != run_pass.cbegin()) pass_name << ", ";
        pass_name << *i;
      }
      LOG(INFO) << "Only running the following passes: {}" << pass_name.str()
                << "\n";
    }
    options.run_pass = {std::move(run_pass)};
  } else {
    options.run_pass = {};
  }

  if (parser.present("--skip-pass")) {
    auto out = parser.get<std::string>("--skip-pass");
    std::set<std::string> skip_pass;
    {
      int low = 0;
      for (int i = 0; i < out.size(); i++) {
        if (out[i] == ',') {
          std::string item = out.substr(low, i);
          skip_pass.insert(item);
          low = i + 1;
        }
      }
      std::string item = out.substr(low, out.size());
      skip_pass.insert(item);
    }
    {
      std::stringstream pass_name;
      for (auto i = skip_pass.cbegin(); i != skip_pass.cend(); i++) {
        if (i != skip_pass.cbegin()) pass_name << ", ";
        pass_name << *i;
      }
      LOG(INFO) << "Skipping the following passes: {}" << pass_name.str()
                << "\n";
    }
    options.skip_pass = std::move(skip_pass);
  } else {
    options.skip_pass = {};
  }

  LOG(INFO) << "input file is " << options.in_file << "\n";
  LOG(INFO) << "output file is " << options.out_file << "\n";

  return std::move(options);
}
