#pragma once

#include <unordered_map>

#include "../backend.hpp"

namespace optimization {

const std::string BASIC_BLOCK_ORDERING_DATA_NAME = "basic_block_ordering";

using BasicBlockOrderingType =
    std::unordered_map<std::string, std::vector<uint32_t>>;

}  // namespace optimization
