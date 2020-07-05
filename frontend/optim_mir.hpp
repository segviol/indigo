#pragma once

#include <map>
#include <vector>
#include <string>
#include <variant>
#include <memory>
#include <algorithm>
#include <queue>

#include "../mir/mir.hpp"

#ifndef COMPILER_FRONT_OPTIM_MIR_H_
#define COMPILER_FRONT_OPTIM_MIR_H_

namespace front::optim_mir {

    class JumpLableId {
    public:
        int _jumpLabelId;
        JumpLableId(int jumpLabelId);
    };

    /*class Inst {
    public:
        int s;
        Inst(int _s) :s(_s) {};
    };

    class JumpInstruction {
    public:
        int kind;
        int bb_true;
        int bb_false;
        JumpInstruction(int _kind, int _bb_true, int _bb_false)
            :kind(_kind), bb_true(_bb_true), bb_false(_bb_false) {};
    };*/

    typedef std::variant<std::shared_ptr<mir::inst::Inst>,
        std::shared_ptr<mir::inst::JumpInstruction>,
        std::shared_ptr<JumpLableId>>
        Instruction;

    class BasicBlock {
    public:
        int id;
        int pre_id;
        std::vector<Instruction> inst;
        std::vector<BasicBlock*> nextBlock;
        std::vector<BasicBlock*> preBlock;
        BasicBlock(int _id, int _pre_id);
    };

    //Results of intermediate code generation
    //std::map<int, std::vector<Instruction>> _funcIdToInstructions;

    //map<int, Node> generate_CFG(vector<Instruction> instructions);

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

    //std::map<int, BasicBlock> front::optim_mir::generate_CFG(std::vector<Instruction> instructions);
}

std::map<int, front::optim_mir::BasicBlock*> generate_CFG(std::vector<front::optim_mir::Instruction> instructions);
front::optim_mir::BasicBlock* find_entry(std::map<int, front::optim_mir::BasicBlock*> nodes);
front::optim_mir::BasicBlock* find_exit(std::map<int, front::optim_mir::BasicBlock*> nodes);
std::vector<int> vectors_intersection(std::vector<int> v1, std::vector<int> v2);
std::vector<int> vectors_set_union(std::vector<int> v1, std::vector<int> v2);
std::vector<int> vectors_difference(std::vector<int> v1, std::vector<int> v2);
bool has_this(std::vector<mir::inst::VarId> v, mir::inst::VarId s);
std::vector<std::string> vectors_intersection(std::vector<std::string> v1, std::vector<std::string> v2);
std::vector<std::string> vectors_set_union(std::vector<std::string> v1, std::vector<std::string> v2);
std::vector<std::string> vectors_difference(std::vector<std::string> v1, std::vector<std::string> v2);
std::vector<std::string> calcu_out(front::optim_mir::BasicBlock* b, std::map<int, std::vector<std::string>> in);
std::vector<std::string> calcu_in(int n, std::map<int, std::vector<std::string>> use,
    std::map<int, std::vector<std::string>> out, std::map<int, std::vector<std::string>> def);
std::map<int, std::vector<mir::inst::VarId>> active_var(std::map<int, front::optim_mir::BasicBlock*> nodes);
std::vector<int> pred_intersection(std::map<int, front::optim_mir::BasicBlock*> nodes, int n, std::map<int, std::vector<int>> dom);
std::map<int, int> find_idom(std::map<int, front::optim_mir::BasicBlock*> nodes);
std::map<int, std::vector<int>> dominac_frontier(std::map<int, int> dom, std::map<int, front::optim_mir::BasicBlock*> nodes);
std::map<int, front::optim_mir::BasicBlock*> generate_SSA(std::map<int, front::optim_mir::BasicBlock*> nodes, std::map<int,
    std::vector<std::string>> global, std::map<int, std::vector<int>> dom_f, std::vector<front::optim_mir::Instruction> instructions);

#endif