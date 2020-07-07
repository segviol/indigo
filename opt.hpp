#pragma once

#include <string>

#include "include/spdlog/include/spdlog/common.h"
#include "include/spdlog/include/spdlog/spdlog.h"

struct Options {
  std::string in_file;
  std::string out_file;
  spdlog::level::level_enum log_level;
};
