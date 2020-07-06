#pragma once

#include <map>
#include <vector>
#include <string>
#include <variant>
#include <memory>
#include <algorithm>
#include <queue>
#include <iostream>

#include "../mir/mir.hpp"
#include "ir_generator.hpp"

#ifndef COMPILER_FRONT_OPTIM_MIR_H_
#define COMPILER_FRONT_OPTIM_MIR_H_

namespace front::optim_mir {

    class JumpLableId {
    public:
        int _jumpLabelId;
        JumpLableId(int jumpLabelId);
    };

    class BasicBlock {
    public:
        int id;
        int pre_id;
        std::vector<front::irGenerator::Instruction> inst;
        std::vector<BasicBlock*> nextBlock;
        std::vector<BasicBlock*> preBlock;
        BasicBlock(int _id, int _pre_id);
    };

    class phi_index {
    public:
        int m;
        mir::inst::VarId name;
        phi_index(int _m, mir::inst::VarId _name) :m(_m), name(_name) {};
        bool operator < (const phi_index& c) const {
            if (m < c.m)
                return true;
            if (m > c.m)
                return false;
            return name < c.name;
        }
    };

    typedef struct {
        int n;
        int m;
        mir::inst::VarId name;
        int num;
    }phi_info;

}

void gen_ssa(std::map<std::string, std::vector<front::irGenerator::Instruction>> f,
    mir::inst::MirPackage& p, front::irGenerator::irGenerator& irgenerator);

#endif