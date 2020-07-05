#include <assert.h>
#include <sys/types.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "../../mir/mir.hpp"
#include "../backend.hpp"
#include "livevar_analyse.hpp"
namespace optimization::common_expr_del {
class Node;

enum class NormOp {
  Add,
  Sub,
  Mul,
  Div,
  Rem,
  Gt,
  Lt,
  Gte,
  Lte,
  Eq,
  Neq,
  And,
  Or,
  Ref
};

enum class InNormOp { Load, Store, Ptroffset, Call };
typedef std::shared_ptr<Node> sharedNode;
typedef std::variant<NormOp, InNormOp> Op;
class NodeId {
 public:
  u_int32_t id;
  NodeId(u_int32_t id) : id(id) {}
  NodeId() {}
  bool operator<(const NodeId& other) const { return id < other.id; }
  bool operator==(const NodeId& other) const { return id == other.id; }
};
typedef std::variant<int, NodeId> Operand;
// class Operand : public  {
//   Operand(int r):std::variant<int,NodeId>(r){}
//   Operand(NodeId r):std::variant<int,NodeId>(r){}
//   bool operator==(const Operand & other){
//     if(index()!=)
//   }
// };
class Node {
 public:
  std::vector<Operand> operands;
  std::variant<NormOp, InNormOp> op;
  NodeId nodeId;
  std::optional<mir::inst::VarId> mainVar;
  std::vector<mir::inst::VarId> live_vars;
  std::vector<mir::inst::VarId> local_vars;
<<<<<<< HEAD
  mir::inst::Value value;
=======
  mir::inst::Value value;  // int Imm
>>>>>>> develop
  bool is_leaf;
  Node() : is_leaf(true) {}
  Node(Op op, std::vector<Operand> operands)
      : op(op), operands(operands), is_leaf(false) {}
  void add_live_var(mir::inst::VarId var) { live_vars.push_back(var); }
  void add_local_var(mir::inst::VarId var) { local_vars.push_back(var); }

  void init_main_var() {
    if (live_vars.size()) {
      mainVar = live_vars.back();
      live_vars.pop_back();
    } else {
      assert(local_vars.size());
      mainVar = local_vars.back();
      local_vars.pop_back();
    }
  }

  bool operator<(const Node& other) {
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
    } else {
      if (std::get<InNormOp>(op) != std::get<InNormOp>(other.op)) {
        return std::get<InNormOp>(op) < std::get<InNormOp>(other.op);
      }
    }
    assert(op.index() == 0);  // only Norm op is allowed to reuse the node
    // same op
    if (op.index() == 0) {
      if (std::get<NormOp>(op) == NormOp::Add ||
          std::get<NormOp>(op) == NormOp::Mul) {
        std::vector<Operand> reverse_operands;
        std::reverse_copy(operands.begin(), operands.end(), reverse_operands);
        if (operands == other.operands || reverse_operands == other.operands) {
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
  BlockNodes(std::map<uint32_t, mir::inst::Variable>& variables,
             std::shared_ptr<livevar_analyse::Block_Live_Var>& blv)
      : variables(variables), blv(blv) {}

  NodeId add_leaf_node(mir::inst::VarId var) {
    auto id = nodes.size();
    NodeId nodeId(id);
    auto node = std::make_shared<Node>(true);
    nodes.push_back(node);
    var_map[var] = nodeId;
    node_map[*node] = id;
    return nodeId;
  }

  NodeId add_node(Op op, std::vector<Operand> operands) {
    auto node = std::make_shared<Node>(false);
    node->op = op;
    node->operands = operands;
    auto id = nodes.size();
    NodeId nodeId(id);
    nodes.push_back(node);
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

  // NodeId query_node_else_add(Op op, std::vector<Operand> operands){
  //   if(!query_node(op,operands)){
  //     add_node(op, operands);
  //   }
  // }

  void add_var(mir::inst::VarId var, NodeId nodeId) {
    if (in_live_out(var)) {
      nodes[nodeId.id]->add_live_var(var);
    } else {
      nodes[nodeId.id]->add_local_var(var);
    }
  }

  void add_var(mir::inst::VarId var) {
    auto nodeId = var_map[var];
    add_var(var, nodeId.id);
  }

  bool in_live_out(mir::inst::VarId var) {
    return blv->live_vars_out->count(var);
  }

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
    if (variables[var].is_memory_var) {
      return false;
    }
    return var_map.count(var);
  }

  Op cast_op(mir::inst::Op op) {
    switch (op) {
      case mir::inst::Op::Add:
        return NormOp::Add;
        break;
      case mir::inst::Op::Sub:
        return NormOp::Sub;
        break;
      case mir::inst::Op::Div:
        return NormOp::Div;
        break;
      case mir::inst::Op::Mul:
        return NormOp::Mul;
        break;
      case mir::inst::Op::Rem:
        return NormOp::Rem;
        break;
      case mir::inst::Op::Gt:
        return NormOp::Gt;
        break;
      case mir::inst::Op::Gte:
        return NormOp::Gte;
        break;
      case mir::inst::Op::Lt:
        return NormOp::Lt;
        break;
      case mir::inst::Op::Lte:
        return NormOp::Lte;
        break;
      case mir::inst::Op::Eq:
        return NormOp::Eq;
        break;
      case mir::inst::Op::Neq:
        return NormOp::Neq;
        break;
      case mir::inst::Op::And:
        return NormOp::And;
        break;
      case mir::inst::Op::Or:
        return NormOp::Or;
        break;
    }
  }
};

class Common_Expr_Del : backend::MirOptimizePass {
  const std::string name = "Common expression delete";
  std::string pass_name() { return name; }

  void optimize_block(mir::inst::BasicBlk& block,
                      std::shared_ptr<livevar_analyse::Block_Live_Var>& blv,
                      std::map<u_int32_t, mir::inst::Variable> variables) {
    BlockNodes blnd(variables, blv);
    for (auto& inst : block.inst) {
      auto kind = inst->inst_kind();
      auto& i = *inst;
      switch (kind) {
        case mir::inst::InstKind::Ref: {
          auto refInst = dynamic_cast<mir::inst::RefInst*>(&i);
          std::vector<mir::inst::Value> values;

          mir::inst::Value val = std::get<mir::inst::VarId>(refInst->val);
          values.push_back(val);
          auto operands = blnd.cast_operands(values);
          auto op = NormOp::Ref;
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
          auto op = blnd.cast_op(opInst->op);
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
          if (assignInst->src.index() == 0) {
          }
        }
        case mir::inst::InstKind::Load:
        case mir::inst::InstKind::Store: {
        }
        case mir::inst::InstKind::Call:
        case mir::inst::InstKind::PtrOffset:

        default:
          std::cout << "error!" << std::endl;
      }
    }
  }
  void optimize_func(mir::inst::MirFunction& func) {
    for (auto iter = func.basic_blks.begin(); iter != func.basic_blks.end();
         ++iter) {
      livevar_analyse::Livevar_Analyse lva(func);
      lva.build();
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