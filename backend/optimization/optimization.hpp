#pragma once

#include <string>
#include <unordered_map>

#include "../backend.hpp"

namespace optimization {

const std::string BASIC_BLOCK_ORDERING_DATA_NAME = "basic_block_ordering";

using BasicBlockOrderingType =
    std::unordered_map<std::string, std::vector<uint32_t>>;

const std::string CYCLE_START_DATA_NAME = "cycle_start";

using CycleStartType = std::unordered_map<std::string, std::set<uint32_t>>;

const std::string inline_blks = "inline_blks";

using InlineBlksType = std::unordered_map<std::string, std::set<uint32_t>>;

const std::string MIR_VARIABLE_TO_ARM_VREG_DATA_NAME = "mir_variable_to_vreg";

using MirVariableToArmVRegType =
    std::unordered_map<std::string, std::map<mir::inst::VarId, arm::Reg>>;

}  // namespace optimization
