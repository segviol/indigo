#include <fstream>
#include <iostream>
#include "frontend/syntax_analyze.hpp"
#include "frontend/ir_generator.hpp"
#include "frontend/optim_mir.hpp"
#include "backend/backend.hpp"
#include "backend/codegen/codegen.hpp"
#include "backend/optimization/graph_color.hpp"
#include "mir/mir.hpp"
#include "prelude/fake_mir_generate.hpp"

using std::ifstream;
using std::ofstream;
using std::istreambuf_iterator;
using std::cout;
using std::endl;
using std::string;

const bool debug = false;

string& read_input();

int main()
{
    //frontend
    std::vector<front::word::Word> word_arr(VECTOR_SIZE);
    word_arr.clear();

    string& input_str = read_input();

    word_analyse(input_str, word_arr);
    delete& input_str;

    front::syntax::SyntaxAnalyze syntax_analyze(word_arr);
    syntax_analyze.gm_comp_unit();

    syntax_analyze.outputInstructions(std::cout);

    front::irGenerator::irGenerator& irgenerator = syntax_analyze.getIrGenerator();
    std::map<string, std::vector<front::irGenerator::Instruction>> inst = irgenerator.getfuncNameToInstructions();
    mir::inst::MirPackage& package = irgenerator.getPackage();

    gen_ssa(inst, package, irgenerator);

    package.display(std::cout);

    //backend
    front::fake::FakeGenerator x;
    x.fakeMirGenerator1();
    auto mir = x._package;
    std::cout << "MIR:" << std::endl << *mir << std::endl;
    backend::Backend backend(std::move(*mir));
    auto code = backend.generate_code();
    std::cout << "CODE:" << std::endl << code;
    return 0;

}

string& read_input()
{
    ifstream input;
    input.open("testfile.txt");
    string* input_str_p = new string(istreambuf_iterator<char>(input), istreambuf_iterator<char>());
    return *input_str_p;
}