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
  bool is_leaf;
  Node() {}
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

class Nodes {
 public:
  std::vector<Node> nodes;
  std::map<mir::inst::VarId, uint32_t> var_map;
  std::map<Node, NodeId> node_map;
  u_int32_t add_leaf_node() {}
};

class Common_Expr_Del : backend::MirOptimizePass {
  const std::string name = "Common expression delete";
  std::string pass_name() { return name; }

  void optimize_block(mir::inst::BasicBlk& block,
                      std::shared_ptr<livevar_analyse::Block_Live_Var> blv) {
    std::vector<Node> nodes;
    std::map<mir::inst::VarId, uint32_t> var_map;
    std::map<Node, NodeId> node_map;
    for (auto iter = block.inst.begin(); iter != block.inst.end(); ++iter) {
      auto kind = iter->get()->inst_kind();
      switch (kind) {
        case mir::inst::InstKind::Ref:
        case mir::inst::InstKind::Op: {
        }
        case mir::inst::InstKind::Assign: {
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
      optimize_block(iter->second, lva.livevars[iter->first]);
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