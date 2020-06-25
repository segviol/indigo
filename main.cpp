#include "mir/mir.hpp"

int main(int argc, char const *argv[]) {
  // TODO: code
  auto val = mir::types::new_int_ty();
  std::cout << *val << std::endl;
  return 0;
}
