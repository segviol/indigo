#include "backend/backend.hpp"
#include "backend/codegen/codegen.hpp"
#include "backend/optimization/graph_color.hpp"
#include "mir/mir.hpp"
#include "prelude/fake_mir_generate.hpp"

int main(int argc, char const *argv[]) {
  // TODO: code
  front::fake::FakeGenerator x;
  x.fakeMirGenerator1();
  auto mir = x._package;

  backend::Backend backend(std::move(*mir));
  auto code = backend.generate_code();

  return 0;
}
