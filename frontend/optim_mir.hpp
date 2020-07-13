#pragma once

#include <map>
#include <vector>
#include <string>
#include <variant>
#include <memory>
#include <algorithm>
#include <queue>
#include <iostream>
#include <iomanip>

#include "../mir/mir.hpp"
#include "ir_generator.hpp"

#ifndef COMPILER_FRONT_OPTIM_MIR_H_
#define COMPILER_FRONT_OPTIM_MIR_H_

namespace front::optim_mir {

    class BasicBlock {
    public:
        int id;
        int pre_id;
        std::vector<front::irGenerator::Instruction> inst;
        std::vector<BasicBlock*> nextBlock;
        std::vector<BasicBlock*> preBlock;
        BasicBlock(int _id, int _pre_id);
    };

}

void gen_ssa(std::map<std::string, std::vector<front::irGenerator::Instruction>> f,
    mir::inst::MirPackage& p, front::irGenerator::irGenerator& irgenerator);

#endif