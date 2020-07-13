#pragma once

#include <set>
#include <string>
#include <vector>

#include "include/spdlog/include/spdlog/common.h"
#include "include/spdlog/include/spdlog/spdlog.h"

struct Options {
  std::string in_file;
  std::string out_file;
  bool show_code_after_each_pass;
  spdlog::level::level_enum log_level;
  std::optional<std::set<std::string>> run_pass;
  std::set<std::string> skip_pass;
};
