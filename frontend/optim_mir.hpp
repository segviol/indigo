#pragma once

#include <map>
#include "../mir/mir.hpp"

#ifndef COMPILER_FRONT_OPTIM_MIR_H_
#define COMPILER_FRONT_OPTIM_MIR_H_

namespace front::optim_mir {

    using std::variant;
    using std::unique_ptr;
    using std::shared_ptr;
    using std::map;
    using std::vector;
    using std::queue;
    using std::string;
    using std::find;
    using std::get;
    using std::get_if;
    using mir::types::LabelId;
    using mir::inst::Inst;

    class JumpLableId {
    public:
        LabelId _jumpLabelId;
        JumpLableId(int jumpLabelId);
    };

    typedef variant<shared_ptr<mir::inst::Inst>,
                    shared_ptr<mir::inst::JumpInstruction>,
                    shared_ptr<JumpLableId>>
        Instruction;

    class BasicBlock {
    public:
        LabelId id;
        LabelId pre_id;
        vector<Instruction> inst;
        vector<BasicBlock*> nextBlock;
        vector<BasicBlock*> preBlock;
        BasicBlock(int _id, int _pre_id);
    };

    //Results of intermediate code generation
    map<LabelId, vector<Instruction>> _funcIdToInstructions;

    //map<int, Node> generate_CFG(vector<Instruction> instructions);

    typedef struct {
        int n;
        int m;
        string name;
        int num;
    }phi_info;

    typedef struct {
        int m;
        string name;
    }phi_index;
}

#endif