#pragma once
#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "../../include/aixlog.hpp"
#include "../../mir/mir.hpp"
#include "../backend.hpp"
#include "./var_replace.hpp"
#include "livevar_analyse.hpp"

namespace optimization::common_expr_del {
class Node;

typedef mir::inst::Op NormOp;
enum class ExtraNormOp { Ref, Ptroffset, Assign };
enum class InNormOp { Load, Store, Call, Phi };
typedef std::shared_ptr<Node> sharedNode;
typedef std::variant<NormOp, ExtraNormOp, InNormOp> Op;
// typedef std::variant<int, mir::inst::VarId, std::string> Value;
class NodeId {
 public:
  uint32_t id;
  NodeId(uint32_t id) : id(id) {}
  NodeId() {}
  bool operator<(const NodeId& other) const { return id < other.id; }
  bool operator==(const NodeId& other) const { return id == other.id; }
};
typedef std::variant<int, NodeId, std::string> Operand;
// class Operand : public  {
//   Operand(int r):std::variant<int,NodeId>(r){}
//   Operand(NodeId r):std::variant<int,NodeId>(r){}
//   bool operator==(const Operand & other){
//     if(index()!=)
//   }
// };
class Env {
 public:
  mir::inst::MirFunction& func;
  int varId;
  Env(mir::inst::MirFunction& func) : func(func) {
    if (func.variables.size()) {
      auto end_iter = func.variables.end();
      end_iter--;
      varId = end_iter->first;
    } else {
      varId = mir::inst::VarId(65535);
    }
  }
  mir::inst::VarId get_new_VarId() { return mir::inst::VarId(++varId); }
  mir::inst::VarId copy_var(mir::inst::VarId var) {
    auto new_var = get_new_VarId();
    func.variables.insert({new_var.id, func.variables.at(var.id)});
    return new_var;
  }

  void remove_var(mir::inst::VarId var) {
    if (func.variables.count(var.id)) {
      func.variables.erase(var.id);
    }
  }
};

std::shared_ptr<Env> env;

class Node {
 public:
  std::vector<Operand> operands;
  Op op;
  NodeId nodeId;
  mir::inst::Value mainVar =
      0;  // mainVar is an imm only when the node starts with a cosnt
          // assignInst.In other cases,the mainVar must be varId
  std::list<mir::inst::VarId> live_vars;
  std::list<mir::inst::VarId> local_vars;
  std::optional<mir::inst::Value> value;  // int Imm or VarId
  std::set<NodeId> parents;
  std::variant<mir::inst::VarId, std::string> refVal;
  bool is_leaf;
  Node() : is_leaf(true) {}
  Node(Op op, std::vector<Operand> operands)
      : op(op), operands(operands), is_leaf(false) {
    // if (op.index() == 1 && std::get<ExtraNormOp>(op) == ExtraNormOp::Ref) {
    //   assert(operands.size() == 1);
    // }
  }
  void add_live_var(mir::inst::VarId var) { live_vars.push_back(var); }
  void add_local_var(mir::inst::VarId var) { local_vars.push_back(var); }

  void init_main_var() {
    if (value.has_value()
        // && value->index() == 0 ||
        //     value.has_value() && value->index() == 1 &&
        //         std::get<mir::inst::VarId>(value.value()).id > 10000
    ) {
      mainVar = value.value();
    } else {
      // if (local_vars.size()) {  // func para has high main priority
      //   auto iter = local_vars.begin();
      //   for (; iter != local_vars.end(); ++iter) {
      //     if (iter->id < 10000) {
      //       break;
      //     }
      //   }
      //   if (iter != local_vars.end()) {
      //     mainVar = *iter;
      //     has_para = true;
      //     local_vars.erase(iter);
      //   }
      // }
      // if (live_vars.size()) {
      //   mainVar = live_vars.front();
      //   live_vars.pop_front();
      // } else
      if (local_vars.size()) {
        // assert(local_vars.size()); store inst has no dest
        mainVar = local_vars.front();
        local_vars.pop_front();
        for (auto var : local_vars) {
          env->remove_var(var);
        }
      } else if (live_vars.size()) {  // add new var (store op has no local or
                                      // live vars)
        // mainVar = env->copy_var(live_vars.front());
        mainVar = live_vars.front();
        live_vars.pop_front();
      }
    }
  }

  bool operator<(const Node& other) const {
    if (is_leaf != other.is_leaf) {
      return is_leaf < other.is_leaf;
    }
    if (op.index() != other.op.index()) {
      return op.index() < other.op.index();
    }
    // same kind op
    if (op.index() == 0) {
      if (std::get<NormOp>(op) != std::get<NormOp>(other.op)) {
        return std::get<NormOp>(op) < std::get<NormOp>(other.op);
      }
    } else if (op.index() == 1) {
      if (std::get<ExtraNormOp>(op) != std::get<ExtraNormOp>(other.op)) {
        return std::get<ExtraNormOp>(op) < std::get<ExtraNormOp>(other.op);
      }
    } else {
      if (std::get<InNormOp>(op) != std::get<InNormOp>(other.op)) {
        return std::get<InNormOp>(op) < std::get<InNormOp>(other.op);
      }
    }
    assert(op.index() == 0 ||
           op.index() == 1);  // only Norm op is allowed to reuse the node
    // same op
    if (op.index() == 0) {
      if (std::get<NormOp>(op) == NormOp::Add ||
          std::get<NormOp>(op) == NormOp::Mul) {
        if (operands == other.operands) {
          return false;
        }
        std::vector<Operand> reverse_operands(operands);
        std::reverse(reverse_operands.begin(), reverse_operands.end());
        if (reverse_operands == other.operands) {
          return false;
        }
      }
    }

    return operands < other.operands;
  }
};

class BlockNodes {
 public:
  std::vector<sharedNode> nodes;
  std::map<mir::inst::VarId, NodeId> var_map;
  std::map<Node, NodeId> node_map;  // this map only uses for checking whether
                                    // the new node(op,operands) existes,other
                                    // attrs in the node should never be used
  std::map<uint32_t, mir::inst::Variable>& variables;
  std::shared_ptr<livevar_analyse::Block_Live_Var>& blv;

  std::map<uint32_t, bool> added;
  std::list<uint32_t> exportQueue;
  ~BlockNodes() {}
  BlockNodes(std::map<uint32_t, mir::inst::Variable>& variables,
             std::shared_ptr<livevar_analyse::Block_Live_Var>& blv)
      : variables(variables), blv(blv) {}

  NodeId add_leaf_node(mir::inst::Value val) {
    auto id = nodes.size();
    NodeId nodeId(id);
    auto node = std::make_shared<Node>();
    node->value = val;
    if (val.index() == 1) {
      var_map[std::get<mir::inst::VarId>(val)] = nodeId;
    }
    nodes.push_back(node);
    node_map[*node] = id;
    return nodeId;
  }

  NodeId add_node(Op op, std::vector<Operand> operands) {
    auto node = std::make_shared<Node>(op, operands);
    auto id = nodes.size();
    NodeId nodeId(id);
    nodes.push_back(node);
    if (op.index() != 2) {
      node_map[*node] = nodeId;
    }
    for (auto operand : operands) {
      if (operand.index() == 1) {
        nodes[std::get<NodeId>(operand).id]->parents.insert(id);
      }
    }

    return nodeId;
  }
  NodeId add_node(Op op, std::vector<Operand> operands, mir::inst::VarId var) {
    auto nodeId = add_node(op, operands);
    add_var(var, nodeId.id);
    return nodeId;
  }

  bool query_node(Op op, std::vector<Operand> operands) {
    Node node(op, operands);
    return node_map.count(node);
  }

  NodeId query_nodeId(Op op, std::vector<Operand> operands) {
    Node node(op, operands);

    NodeId nodeId(node_map[node]);
    return nodeId;
  }

  NodeId query_nodeId(mir::inst::VarId var) { return var_map.at(var); }

  // NodeId query_node_else_add(Op op, std::vector<Operand> operands){
  //   if(!query_node(op,operands)){
  //     add_node(op, operands);
  //   }
  // }

  void add_var(mir::inst::VarId var, NodeId nodeId) {
    var_map[var] = nodeId;
    if (in_live_out(var)) {
      nodes[nodeId.id]->add_live_var(var);
    } else {
      nodes[nodeId.id]->add_local_var(var);
    }
  }

  // void add_var(mir::inst::VarId var) {
  //   auto nodeId = var_map[var];
  //   add_var(var, nodeId.id);
  // }

  bool in_live_out(mir::inst::VarId var) {
    // Consider the jumpInst use vars
    // if (!blv->instLiveVars.size()) {
    //   return blv->live_vars_out->count(var);
    // }
    return blv->live_vars_out->count(var);
  }

  bool exportNode(uint32_t idx) {
    if (added.count(idx)) {
      return false;
    }
    bool flag = false;
    auto parents = nodes[idx]->parents;
    for (auto nodeId : parents) {
      if (!added.count(nodeId.id)) {
        flag = true;
      }
    }
    if (flag) {
      return false;
    }
    exportQueue.push_back(idx);
    added[idx] = true;
    // for (auto operand : nodes[idx]->operands) {
    //   if (operand.index() == 1) {
    //     auto nodeId = std::get<NodeId>(operand);
    //     exportNode(nodeId.id);
    //     break;
    //   }
    // }
    return true;
  }

  mir::inst::Value convert_operand_to_value(Operand operand) {
    if (operand.index() == 0) {
      auto imm = std::get<int>(operand);
      return imm;
    } else {
      auto nodeId = std::get<NodeId>(operand);
      return nodes[nodeId.id]->mainVar;
    }
  }

  void export_insts(mir::inst::BasicBlk& block) {
    auto& inst = block.inst;
    for (auto iter = nodes.begin(); iter != nodes.end(); ++iter) {
      iter->get()->init_main_var();
    }
    while (added.size() < nodes.size()) {
      for (int i = nodes.size() - 1; i >= 0; --i) {
        exportNode(i);
      }
    }
    inst.clear();
    // cause var replace only works for other block's vars
    var_replace::Var_Replace vp(env->func);
    // for (auto& varpair : var_map) {
    //   if (varpair.first > 10000) {
    //     break;
    //   }
    //   if (varpair.first >= 1) {
    //     inst.push_back(std::make_unique<mir::inst::AssignInst>(
    //         std::get<mir::inst::VarId>(nodes[varpair.second.id]->mainVar),
    //         varpair.first));
    //   }
    // }
    std::reverse(exportQueue.begin(), exportQueue.end());
    for (auto idx : exportQueue) {
      auto& node = nodes[idx];
      if (!node->is_leaf) {
        auto opKind = node->op.index();
        switch (opKind) {
          case 0: {
            auto lhs = convert_operand_to_value(node->operands[0]);
            auto rhs = convert_operand_to_value(node->operands[1]);
            auto opInst = std::make_unique<mir::inst::OpInst>(
                std::get<mir::inst::VarId>(node->mainVar), lhs, rhs,
                std::get<NormOp>(node->op));
            inst.push_back(std::move(opInst));
            break;
          }

          case 1: {
            switch (std::get<ExtraNormOp>(node->op)) {
              case ExtraNormOp::Ref: {
                std::variant<mir::inst::VarId, std::string> val;
                if (node->operands[0].index() == 1) {
                  val = std::get<mir::inst::VarId>(
                      nodes[std::get<NodeId>(node->operands[0]).id]->mainVar);
                  //这里可能的VarId是局部数组，理论上不会出现int成为mainVar的情况

                } else {
                  val = std::get<std::string>(node->operands[0]);
                }
                inst.push_back(std::make_unique<mir::inst::RefInst>(
                    std::get<mir::inst::VarId>(node->mainVar), val));
                break;
              }
              case ExtraNormOp::Ptroffset: {
                auto ptr = std::get<mir::inst::VarId>(
                    convert_operand_to_value(node->operands[0]));
                auto offset = convert_operand_to_value(node->operands[1]);
                auto ptrInst = std::make_unique<mir::inst::PtrOffsetInst>(
                    std::get<mir::inst::VarId>(node->mainVar), ptr, offset);
                inst.push_back(std::move(ptrInst));
                break;
              }
              case ExtraNormOp::Assign: {
                mir::inst::Value src(0);
                auto operand = node->operands[0];
                if (operand.index() == 1) {
                  auto srcId = std::get<NodeId>(node->operands[0]);
                  src = nodes[srcId.id]->mainVar;
                } else {
                  src = std::get<int>(operand);
                }

                auto assignInst = std::make_unique<mir::inst::AssignInst>(
                    std::get<mir::inst::VarId>(node->mainVar), src);
                inst.push_back(std::move(assignInst));
                break;
              }
            }

            break;
          }

          case 2: {
            if (std::get<InNormOp>(node->op) == InNormOp::Call) {
              auto func = std::get<std::string>(
                  node->operands[node->operands.size() - 1]);
              node->operands.pop_back();
              std::vector<mir::inst::Value> params;
              for (auto operand : node->operands) {
                params.push_back(convert_operand_to_value(operand));
              }
              auto callInst = std::make_unique<mir::inst::CallInst>(
                  std::get<mir::inst::VarId>(node->mainVar), func, params);
              inst.push_back(std::move(callInst));
            }
            if (std::get<InNormOp>(node->op) == InNormOp::Phi) {
              std::vector<mir::inst::VarId> vars;
              for (auto operand : node->operands) {
                auto oper_node = nodes[std::get<NodeId>(operand).id];
                mir::inst::VarId var;
                if (oper_node->mainVar.index() == 0) {
                  // imm

                  if (oper_node->live_vars.size()) {
                    var = oper_node->live_vars.front();
                  } else {
                    var = oper_node->local_vars.front();
                    auto assignInst = std::make_unique<mir::inst::AssignInst>(
                        var, node->mainVar);
                    inst.push_back(std::move(assignInst));
                  }
                } else {
                  var = std::get<mir::inst::VarId>(oper_node->mainVar);
                }
                vars.push_back(var);
              }
              auto phiInst = std::make_unique<mir::inst::PhiInst>(
                  std::get<mir::inst::VarId>(node->mainVar), vars);
              inst.push_back(std::move(phiInst));
            } else if (std::get<InNormOp>(node->op) == InNormOp::Load) {
              auto src = nodes[std::get<NodeId>(node->operands[0]).id]->mainVar;
              auto loadInst = std::make_unique<mir::inst::LoadInst>(
                  src, std::get<mir::inst::VarId>(node->mainVar));
              inst.push_back(std::move(loadInst));
            } else if (std::get<InNormOp>(node->op) == InNormOp::Store) {
              auto val = convert_operand_to_value(node->operands[0]);
              auto dest = convert_operand_to_value(node->operands[1]);
              auto storeInst = std::make_unique<mir::inst::StoreInst>(
                  val, std::get<mir::inst::VarId>(dest));
              inst.push_back(std::move(storeInst));
            }
            break;
          }
          default:;
        }
      }
      // if (node->live_vars.size()) {
      //   for (auto var : node->live_vars) {
      //     //
      //     if(node->op.index()==1&&std::get<ExtraNormOp>(node->op)==ExtraNormOp::Assign&&node->operands)
      //     auto assignInst =
      //         std::make_unique<mir::inst::AssignInst>(var, node->mainVar);
      //     inst.push_back(std::move(assignInst));
      //   }
      // }
    }
    // this must be in the end, or the livevar might overides the leaf var used
    // after this because they share the same reg
    // export and replace
    for (auto idx : exportQueue) {
      auto& node = nodes[idx];
      if (node->live_vars.size()) {
        std::optional<mir::inst::VarId> not_phi_var;
        for (auto var : node->live_vars) {
          if (!env->func.variables.at(var.id).is_phi_var) {
            not_phi_var = var;
            break;
          }
        }
        if (not_phi_var.has_value()) {
          for (auto var : node->live_vars) {
            if (!env->func.variables.at(var.id).is_phi_var &&
                var != not_phi_var.value()) {
              vp.replace(var, not_phi_var.value());
            }
          }
        } else {
          for (auto var : node->live_vars) {
            auto assignInst =
                std::make_unique<mir::inst::AssignInst>(var, node->mainVar);
            inst.push_back(std::move(assignInst));
          }
        }
      }
      // if (node->live_vars.size()) {
      //   for (auto var : node->live_vars) {
      //     auto assignInst =
      //         std::make_unique<mir::inst::AssignInst>(var, node->mainVar);
      //     inst.push_back(std::move(assignInst));
      //   }
      // }
    }

    auto& jump = block.jump;
    if (!jump.cond_or_ret.has_value() || jump.cond_or_ret.value().id == 0) {
      return;
    }
    auto varId = jump.cond_or_ret.value();
    auto& node = nodes[var_map[varId].id];
    mir::inst::VarId var = std::get<mir::inst::VarId>(node->mainVar);
    jump.cond_or_ret = var;
    for (auto iter = block.inst.begin(); iter != block.inst.end();) {
      auto& inst = *iter;
      if (inst->dest == var && inst->inst_kind() == mir::inst::InstKind::Op) {
        int idx = iter - block.inst.begin();
        block.inst.push_back(std::move(inst));
        iter = block.inst.begin() + idx;
        iter = block.inst.erase(iter);

        break;
      } else {
        iter++;
      }
    }

    // replace

    for (auto idx : exportQueue) {
      auto& node = nodes[idx];
    }
  }

  void adjustJumpInst(mir::inst::JumpInstruction& jump) {}

  std::vector<Operand> cast_operands(std::vector<mir::inst::Value> value) {
    std::vector<Operand> operands;
    for (auto val : value) {
      if (val.index() == 0) {
        operands.push_back(std::get<int>(val));
      } else {
        auto var = std::get<mir::inst::VarId>(val);
        if (!query_var(var)) {
          add_leaf_node(var);
        }
        auto nodeId = var_map[var];
        operands.push_back(nodeId);
      }
    }
    return operands;
  }

  bool query_var(mir::inst::VarId var) {
    // if (variables[var].is_memory_var) {
    //   return false;
    // }
    return var_map.count(var);
  }
};

class Common_Expr_Del : public backend::MirOptimizePass {
 public:
  const std::string name = "Common expression delete";
  std::string pass_name() const { return name; }

  void optimize_block(mir::inst::BasicBlk& block,
                      std::shared_ptr<livevar_analyse::Block_Live_Var>& blv,
                      std::map<uint32_t, mir::inst::Variable> variables) {
    BlockNodes blnd(variables, blv);
    for (auto& inst : block.inst) {
      auto kind = inst->inst_kind();
      auto& i = *inst;
      switch (kind) {
        case mir::inst::InstKind::Ref: {
          auto refInst = dynamic_cast<mir::inst::RefInst*>(&i);
          auto val = refInst->val;
          std::vector<Operand> operands;
          if (val.index() == 0) {
            std::vector<mir::inst::Value> vals;
            vals.push_back(std::get<mir::inst::VarId>(val));
            auto val_operands = blnd.cast_operands(vals);
            operands.push_back(val_operands.front());
          } else {
            auto str = std::get<std::string>(val);
            operands.push_back(std::get<std ::string>(val));
          }
          assert(operands.size() == 1);
          auto op = ExtraNormOp::Ref;
          if (!blnd.query_node(op, operands)) {
            blnd.add_node(op, operands, refInst->dest);
          } else {
            auto nodeId = blnd.query_nodeId(op, operands);
            blnd.add_var(refInst->dest, nodeId.id);
          }
          break;
        }
        case mir::inst::InstKind::Op: {
          auto opInst = dynamic_cast<mir::inst::OpInst*>(&i);
          std::vector<mir::inst::Value> values;
          values.push_back(opInst->lhs);
          values.push_back(opInst->rhs);
          auto operands = blnd.cast_operands(values);
          auto op = opInst->op;
          if (!blnd.query_node(op, operands)) {
            blnd.add_node(op, operands, opInst->dest);
          } else {
            auto nodeId = blnd.query_nodeId(op, operands);
            blnd.add_var(opInst->dest, nodeId.id);
          }
          break;
        }
        case mir::inst::InstKind::Assign: {
          auto assignInst = dynamic_cast<mir::inst::AssignInst*>(&i);
          if (assignInst->src.index() == 1) {
            auto srcvar = std::get<mir::inst::VarId>(assignInst->src);
            if (variables.at(srcvar.id).is_temp_var) {
              auto nodeId = blnd.query_nodeId(srcvar);
              blnd.add_var(assignInst->dest, nodeId);
              break;
            }
          }
          std::vector<mir::inst::Value> values;
          values.push_back(assignInst->src);
          auto op = ExtraNormOp::Assign;
          auto operands = blnd.cast_operands(values);
          if (!blnd.query_node(op, operands)) {
            blnd.add_node(op, operands, assignInst->dest);
          } else {
            auto nodeId = blnd.query_nodeId(op, operands);
            blnd.add_var(assignInst->dest, nodeId.id);
          }
          break;
        }
        case mir::inst::InstKind::Load: {
          auto loadInst = dynamic_cast<mir::inst::LoadInst*>(&i);
          auto op = InNormOp::Load;
          std::vector<mir::inst::Value> values;
          values.push_back(loadInst->src);
          auto operands = blnd.cast_operands(values);
          blnd.add_node(op, operands, loadInst->dest);
          break;
        }
        case mir::inst::InstKind::Store: {
          auto storeInst = dynamic_cast<mir::inst::StoreInst*>(&i);
          auto op = InNormOp::Store;
          std::vector<mir::inst::Value> values;
          values.push_back(storeInst->val);
          values.push_back(storeInst->dest);
          auto operands = blnd.cast_operands(values);
          blnd.add_node(op, operands);
          break;
        }
        case mir::inst::InstKind::Call: {
          auto callInst = dynamic_cast<mir::inst::CallInst*>(&i);
          auto op = InNormOp::Call;
          std::vector<mir::inst::Value> values(callInst->params);
          auto operands = blnd.cast_operands(values);
          operands.push_back(callInst->func);
          blnd.add_node(op, operands, callInst->dest);
          break;
        }
        case mir::inst::InstKind::PtrOffset: {
          auto ptrInst = dynamic_cast<mir::inst::PtrOffsetInst*>(&i);
          auto op = ExtraNormOp::Ptroffset;
          std::vector<mir::inst::Value> values;
          values.push_back(ptrInst->ptr);
          values.push_back(ptrInst->offset);
          auto operands = blnd.cast_operands(values);
          if (!blnd.query_node(op, operands)) {
            blnd.add_node(op, operands, ptrInst->dest);
          } else {
            auto nodeId = blnd.query_nodeId(op, operands);
            blnd.add_var(ptrInst->dest, nodeId.id);
          }
          break;
        }
        case mir::inst::InstKind::Phi: {
          auto phiInst = dynamic_cast<mir::inst::PhiInst*>(&i);
          auto op = InNormOp::Phi;
          std::vector<mir::inst::Value> values;
          for (auto var : phiInst->vars) {
            values.push_back(var);
          }
          auto operands = blnd.cast_operands(values);
          blnd.add_node(op, operands, phiInst->dest);
          break;
        }

        default:
          LOG(TRACE) << "error!" << std::endl;
      }
    }
    if (block.jump.cond_or_ret.has_value()) {
      auto var = block.jump.cond_or_ret.value();
      if (!blnd.query_var(var)) {
        blnd.add_leaf_node(var);
      }
    }
    blnd.export_insts(block);
  }
  void optimize_func(mir::inst::MirFunction& func) {
    if (func.type->is_extern) {
      return;
    }
    livevar_analyse::Livevar_Analyse lva(func);
    lva.build();

    env = std::make_shared<Env>(func);
    for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
         ++iter) {
      // if (iter->first == 5) {
      //   LOG(TRACE) << "sasq" << std::endl;
      // }
      optimize_block(iter->second, lva.livevars[iter->first], func.variables);
    }
  }
  void optimize_mir(mir::inst::MirPackage& package,
                    std::map<std::string, std::any>& extra_data_repo) {
    for (auto iter = package.functions.begin(); iter != package.functions.end();
         ++iter) {
      optimize_func(iter->second);
    }
  }
};

}  // namespace optimization::common_expr_del
