#pragma once

#include <set>
#include <string>
#include <vector>

struct Options {
  std::string in_file;
  std::string out_file;
  bool show_code_after_each_pass;
  std::optional<std::set<std::string>> run_pass;
  std::set<std::string> skip_pass;
};
