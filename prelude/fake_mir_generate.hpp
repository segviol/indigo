#pragma once

#include <memory>
#include <vector>

#include "../mir/mir.hpp"

namespace front::fake {
using std::move;
using std::nullopt;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

using mir::types::ArrayTy;
using mir::types::FunctionTy;
using mir::types::IntTy;
using mir::types::PtrTy;
using mir::types::SharedTyPtr;

using mir::inst::AssignInst;
using mir::inst::Inst;
using mir::inst::JumpInstruction;
using mir::inst::JumpInstructionKind;
using mir::inst::JumpKind;
using mir::inst::LoadInst;
using mir::inst::MirFunction;
using mir::inst::MirPackage;
using mir::inst::Op;
using mir::inst::OpInst;
using mir::inst::PtrOffsetInst;
using mir::inst::RefInst;
using mir::inst::StoreInst;
using mir::inst::Value;
using mir::inst::Variable;
using mir::inst::VarId;

class FakeGenerator {
 public:
  shared_ptr<MirPackage> _package;

  void fakeMirGenerator1() {
    int i;

    _package = shared_ptr<MirPackage>(new MirPackage());

    _package->functions.insert(
        {"main", MirFunction("main", shared_ptr<FunctionTy>(new FunctionTy(
                                         shared_ptr<IntTy>(new IntTy()),
                                         vector<SharedTyPtr>())))});

    _package->functions.at("main").variables.insert(
        {1,
         Variable(SharedTyPtr(new ArrayTy(SharedTyPtr(new IntTy), 10)), true)});
    _package->functions.at("main").variables.insert(
        {2, Variable(SharedTyPtr(new IntTy()), true)});
    _package->functions.at("main").variables.insert(
        {3, Variable(SharedTyPtr(new IntTy()), true)});
    _package->functions.at("main").variables.insert(
        {4, Variable(SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy()))), false,
                     true)});
    _package->functions.at("main").variables.insert(
        {5, Variable(SharedTyPtr(new IntTy()), false, true)});
    _package->functions.at("main").variables.insert(
        {6, Variable(SharedTyPtr(new PtrTy(SharedTyPtr(new IntTy()))), false,
                     true)});

    _package->functions.at("main").basic_blks.insert(
        {0, mir::inst::BasicBlk(0)});
    _package->functions.at("main").basic_blks.insert(
        {1, mir::inst::BasicBlk(1)});
    _package->functions.at("main").basic_blks.insert(
        {2, mir::inst::BasicBlk(2)});
    _package->functions.at("main").basic_blks.insert(
        {3, mir::inst::BasicBlk(3)});

    _package->functions.at("main").basic_blks.at(0).jump = JumpInstruction(
        JumpInstructionKind::Br, 3, -3, nullopt, JumpKind::Undefined);
    _package->functions.at("main").basic_blks.at(0).inst.push_back(
        move(unique_ptr<Inst>(new RefInst(VarId(4), VarId(1)))));

    for (i = 0; i < 10; i++) {
      _package->functions.at("main").basic_blks.at(0).inst.push_back(
          move(unique_ptr<Inst>(new StoreInst(i + 1, VarId(4)))));
      if (i < 9) {
        _package->functions.at("main").basic_blks.at(0).inst.push_back(move(
            unique_ptr<Inst>(new PtrOffsetInst(VarId(4), VarId(4), i + 1))));
      }
    }

    _package->functions.at("main").basic_blks.at(0).inst.push_back(
        move(unique_ptr<Inst>(new AssignInst(VarId(2), 0))));

    _package->functions.at("main").basic_blks.at(3).id = 3;
    _package->functions.at("main").basic_blks.at(3).jump =
        std::move(JumpInstruction(JumpInstructionKind::BrCond, 1, 2, VarId(5),
                                  JumpKind::Loop));
    _package->functions.at("main").basic_blks.at(3).preceding.insert(0);
    _package->functions.at("main").basic_blks.at(3).preceding.insert(1);

    _package->functions.at("main").basic_blks.at(3).inst.push_back(
        move(unique_ptr<Inst>(new OpInst(VarId(5), 9, VarId(2), Op::Sub))));

    _package->functions.at("main").basic_blks.at(3).inst.push_back(move(
        unique_ptr<Inst>(new OpInst(VarId(5), VarId(2), VarId(5), Op::Lt))));

    _package->functions.at("main").basic_blks.at(1).id = 1;
    _package->functions.at("main").basic_blks.at(1).jump = move(JumpInstruction(
        JumpInstructionKind::Br, 3, -1, nullopt, JumpKind::Loop));
    _package->functions.at("main").basic_blks.at(1).preceding.insert(3);
    _package->functions.at("main").basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new RefInst(VarId(4), VarId(1)))));

    _package->functions.at("main").basic_blks.at(1).inst.push_back(move(
        unique_ptr<Inst>(new PtrOffsetInst(VarId(4), VarId(4), VarId(2)))));

    _package->functions.at("main").basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new LoadInst(VarId(4), VarId(3)))));

    _package->functions.at("main").basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new OpInst(VarId(5), 9, VarId(2), Op::Sub))));
    _package->functions.at("main").basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new RefInst(VarId(6), VarId(1)))));

    _package->functions.at("main").basic_blks.at(1).inst.push_back(move(
        unique_ptr<Inst>(new PtrOffsetInst(VarId(6), VarId(6), VarId(2)))));

    _package->functions.at("main").basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new LoadInst(VarId(6), VarId(5)))));

    _package->functions.at("main").basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new StoreInst(VarId(4), VarId(5)))));

    _package->functions.at("main").basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new StoreInst(VarId(6), VarId(3)))));

    _package->functions.at("main").basic_blks.at(1).inst.push_back(
        move(unique_ptr<Inst>(new OpInst(VarId(2), 2, VarId(1), Op::Add))));

    _package->functions.at("main").basic_blks.at(2).id = 2;
    _package->functions.at("main").basic_blks.at(2).jump = move(JumpInstruction(
        JumpInstructionKind::Return, -1, -1, nullopt, JumpKind::Undefined));
    _package->functions.at("main").basic_blks.at(2).preceding.insert(3);
  }
};
}  // namespace front::fake
