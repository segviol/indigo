#include "optim_mir.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "../mir/mir.hpp"

using namespace front::optim_mir;

JumpLableId::JumpLableId(int n) { _jumpLabelId = n; }

BasicBlock::BasicBlock(int _id, int _pre_id) {
  id = _id;
  pre_id = _pre_id;
}

map<int, BasicBlock*> generate_CFG(vector<Instruction> instructions) {
  int id = -3;                  // entry is -1 and exit is -2
  map<int, BasicBlock*> nodes;  // pair(id, Node)
  BasicBlock* entry = new BasicBlock(-1, -1);
  nodes.insert(map<int, BasicBlock*>::value_type(-1, entry));
  int preid = -1;  // tight front block
  for (int i = 0; i < instructions.size(); i++) {
    BasicBlock* block;
    if (instructions[i].index() == 2) {  // first instruction is JumpLableId
      block = new BasicBlock(get<2>(instructions[i])->_jumpLabelId, preid);
      preid = get<2>(instructions[i])->_jumpLabelId;
      i++;
    } else {
      block = new BasicBlock(id, preid);
      preid = id--;
    }
    // br is the ending, JumpLableId is the beginning.
    // The next sentence of br must be JumpLableId.
    while (instructions[i].index() != 2 && i < instructions.size()) {
      block->inst.push_back(instructions[i]);
      i++;
    }
    nodes.insert(map<int, BasicBlock*>::value_type(preid, block));
  }
  BasicBlock* exit = new BasicBlock(-2, preid);
  nodes.insert(map<int, BasicBlock*>::value_type(preid, exit));
  // walk each node and generate control flow graph
  map<int, BasicBlock*>::iterator iter = nodes.begin();
  while (iter != nodes.end()) {
    map<int, BasicBlock*>::iterator it = nodes.find(iter->second->pre_id);
    if (it != nodes.end()) {  // find the pre node
      if (it->second->inst[it->second->inst.size() - 1].index() ==
          0) {                             // the last instruction isn't br
        if (iter->second->pre_id != -1) {  // not the entry node
          iter->second->preBlock.push_back(it->second);
          it->second->nextBlock.push_back(iter->second);
        }
      } else {  // the last instruction is br, pre_id is unnecessary
        BasicBlock* b = it->second;
        int l1 = get<1>(b->inst[b->inst.size() - 1])->bb_true;
        map<int, BasicBlock*>::iterator itt = nodes.find(l1);
        if (itt != nodes.end()) {
          b->nextBlock.push_back(itt->second);
          itt->second->preBlock.push_back(b);
        }
        if (get<1>(b->inst[b->inst.size() - 1])->kind ==
            mir::inst::JumpInstructionKind::BrCond) {
          int l2 = get<1>(b->inst[b->inst.size() - 1])->bb_true;
          itt = nodes.find(l2);
          if (itt != nodes.end()) {
            b->nextBlock.push_back(itt->second);
            itt->second->preBlock.push_back(b);
          }
        }
      }
    }
    iter++;
  }
  return nodes;
}

BasicBlock* find_entry(map<int, BasicBlock*> nodes) {
  map<int, BasicBlock*>::iterator iter = nodes.find(-1);
  return iter->second;
}

vector<int> vectors_intersection(vector<int> v1, vector<int> v2) {
  vector<int> v;
  sort(v1.begin(), v1.end());
  sort(v2.begin(), v2.end());
  set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(),
                   back_inserter(v));
  return v;
}

vector<int> vectors_set_union(vector<int> v1, vector<int> v2) {
  vector<int> v;
  sort(v1.begin(), v1.end());
  sort(v2.begin(), v2.end());
  set_union(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(v));
  return v;
}

vector<int> vectors_difference(vector<int> v1, vector<int> v2) {
  vector<int> v;
  sort(v1.begin(), v1.end());
  sort(v2.begin(), v2.end());
  set_difference(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(v));
  return v;
}

BasicBlock* find_exit(map<int, BasicBlock*> nodes) {
  BasicBlock* entry = find_entry(nodes);
  vector<int> visit;
  // BFS
  queue<BasicBlock*> q;
  visit.push_back(entry->id);
  q.push(entry);
  while (!q.empty()) {
    BasicBlock* p = q.front();
    q.pop();
    for (int i = 0; i < p->nextBlock.size(); i++) {
      vector<int>::iterator it =
          find(visit.begin(), visit.end(), p->nextBlock[i]->id);
      if (it == visit.end()) {
        if (p->nextBlock[i]->id == -2) {
          return p->nextBlock[i];
        } else {
          visit.push_back(p->nextBlock[i]->id);
          q.push(p->nextBlock[i]);
        }
      }
    }
  }
}

vector<int> pred_intersection(map<int, BasicBlock*> nodes, int n,
                              map<int, vector<int>> dom) {
  //将n的所有直接前驱结点的dom取交，并与n的dom取并
  map<int, BasicBlock*>::iterator it = nodes.find(n);
  if (it != nodes.end()) {
    BasicBlock* b = it->second;
    map<int, vector<int>>::iterator iter = dom.find(n);
    vector<int> v1 = iter->second;
    if (b->preBlock.size() == 1) {
      iter = dom.find(b->preBlock[0]->id);
      vector<int> v2 = iter->second;
      vector<int> v = vectors_set_union(v1, v2);
      return v;
    } else if (b->preBlock.size() >= 2) {
      iter = dom.find(b->preBlock[0]->id);
      vector<int> v2 = iter->second;
      vector<int> v3;
      for (int i = 1; i < b->preBlock.size(); i++) {
        iter = dom.find(b->preBlock[i]->id);
        v3 = iter->second;
        v2 = vectors_intersection(v2, v3);
      }
      vector<int> v = vectors_set_union(v1, v2);
      return v;
    } else {
      // error
    }
  }
  vector<int> v;
  return v;
}

map<int, vector<int>> find_idom(map<int, BasicBlock*> nodes) {
  BasicBlock* exit = find_exit(nodes);
  // step1: Calculate the domin nodes of each block
  // rule: Dom(n) = U Dom(Pred(n))
  map<int, vector<int>> dom;
  // step1.1: initialize map_dom
  map<int, BasicBlock*>::iterator it;
  for (it = nodes.begin(); it != nodes.end(); it++) {
    vector<int> pre;
    pre.push_back(it->first);
    dom.insert(map<int, vector<int>>::value_type(it->first, pre));
  }
  // step1.2: Loop until dom does not change
  bool flag = true;
  while (flag) {
    flag = false;
    // BFS
    vector<int> visit;
    queue<BasicBlock*> q;
    vector<int> v = pred_intersection(nodes, exit->id, dom);
    map<int, vector<int>>::iterator iter = dom.find(exit->id);
    vector<int> prev = iter->second;
    if (v.size() > prev.size()) {
      flag = true;
      dom.insert(map<int, vector<int>>::value_type(exit->id, prev));
    }
    visit.push_back(exit->id);
    q.push(exit);
    while (!q.empty()) {
      BasicBlock* p = q.front();
      q.pop();
      for (int i = 0; i < p->preBlock.size(); i++) {
        BasicBlock* prep = p->preBlock[i];
        vector<int>::iterator itt = find(visit.begin(), visit.end(), prep->id);
        if (itt == visit.end()) {
          v = pred_intersection(nodes, prep->id, dom);
          iter = dom.find(prep->id);
          prev = iter->second;
          if (v.size() > prev.size()) {
            flag = true;
            dom.insert(map<int, vector<int>>::value_type(prep->id, prev));
          }
          visit.push_back(prep->id);
          q.push(prep);
        }
      }
    }
  }
  // step2: Calculate the immediate domin nodes of each block
  // step2.1: delete node itself;
  map<int, vector<int>>::iterator iter;
  for (iter = dom.begin; iter != dom.end; iter++) {
    vector<int>::iterator it = iter->second.begin();
    iter->second.erase(it);
  }
  // step2.2: BFS
  vector<int> visit;
  queue<BasicBlock*> q;
  map<int, vector<int>>::iterator iter = dom.find(exit->id);
  vector<int> v1 = iter->second;
  for (int i = 0; i < exit->preBlock.size(); i++) {
    iter = dom.find(exit->preBlock[i]->id);
    vector<int> v2 = iter->second;
    v1 = vectors_difference(v1, v2);
  }
  dom.insert(map<int, vector<int>>::value_type(exit->id, v1));
  visit.push_back(exit->id);
  q.push(exit);
  while (!q.empty()) {
    BasicBlock* p = q.front();
    q.pop();
    for (int i = 0; i < p->preBlock.size(); i++) {
      BasicBlock* prep = p->preBlock[i];
      vector<int>::iterator itt = find(visit.begin(), visit.end(), prep->id);
      if (itt == visit.end()) {  // never visit
        iter = dom.find(prep->id);
        vector<int> v1 = iter->second;
        for (int i = 0; i < prep->preBlock.size(); i++) {
          iter = dom.find(prep->preBlock[i]->id);
          vector<int> v2 = iter->second;
          v1 = vectors_difference(v1, v2);
        }
        dom.insert(map<int, vector<int>>::value_type(prep->id, v1));
        visit.push_back(prep->id);
        q.push(prep);
      }
    }
  }
  return dom;
}

map<int, vector<int>> dominac_frontier(map<int, vector<int>> dom,
                                       map<int, BasicBlock*> nodes) {
  map<int, vector<int>> dom_f;
  map<int, vector<int>>::iterator iter;
  for (iter = dom.begin(); iter != dom.end(); iter++) {
    map<int, BasicBlock*>::iterator it = nodes.find(iter->first);
    if (it->second->preBlock.size() > 1) {
      for (int i = 0; i < it->second->preBlock.size(); i++) {
        int runner = it->second->preBlock[i]->id;
        map<int, vector<int>>::iterator itt = dom.find(it->first);
        int idom_b = itt->second[0];
        while (runner != idom_b) {
          // add b to runner's dominance frontier set
          itt = dom_f.find(runner);
          if (itt == dom_f.end()) {
            vector<int> b;
            b.push_back(it->first);
            dom_f.insert(map<int, vector<int>>::value_type(runner, b));
          } else {
            vector<int> b = itt->second;
            b.push_back(it->first);
            dom_f.insert(map<int, vector<int>>::value_type(runner, b));
          }
          // runner := idom(runner
          itt = dom.find(runner);
          runner = itt->second[0];
        }
      }
    }
  }
  return dom_f;
}

bool has_this(vector<string> v, string s) {
  for (int i = 0; i < v.size(); i++) {
    if (v[i] == s) {
      return true;
    }
  }
  return false;
}

vector<string> vectors_set_union(vector<string> v1, vector<string> v2) {
  vector<string> v;
  sort(v1.begin(), v1.end());
  sort(v2.begin(), v2.end());
  set_union(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(v));
  return v;
}

vector<string> vectors_difference(vector<string> v1, vector<string> v2) {
  vector<string> v;
  sort(v1.begin(), v1.end());
  sort(v2.begin(), v2.end());
  set_difference(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(v));
  return v;
}

vector<string> vectors_intersection(vector<string> v1, vector<string> v2) {
  vector<string> v;
  sort(v1.begin(), v1.end());
  sort(v2.begin(), v2.end());
  set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(),
                   back_inserter(v));
  return v;
}

vector<string> calcu_out(BasicBlock* b, map<int, vector<string>> in) {
  if (b->nextBlock.size() > 0) {
    map<int, vector<string>>::iterator it = in.find(b->nextBlock[0]->id);
    vector<string> v1 = it->second;
    for (int i = 1; i < b->nextBlock.size(); i++) {
      it = in.find(b->nextBlock[i]->id);
      vector<string> v2 = it->second;
      v1 = vectors_set_union(v1, v2);
    }
    return v1;
  }
}

vector<string> calcu_in(int n, map<int, vector<string>> use,
                        map<int, vector<string>> out,
                        map<int, vector<string>> def) {
  map<int, vector<string>>::iterator it;
  it = out.find(n);
  vector<string> v1 = it->second;
  it = def.find(n);
  v1 = vectors_difference(v1, it->second);
  it = use.find(n);
  return vectors_set_union(v1, it->second);
}

map<int, vector<string>> active_var(map<int, BasicBlock*> nodes) {
  map<int, vector<string>> def;
  map<int, vector<string>> use;
  map<int, vector<string>> in;
  map<int, vector<string>> out;
  BasicBlock* entry = find_entry(nodes);
  // step1: Find the def and use variables for each block
  // BFS
  vector<int> visit;
  queue<BasicBlock*> q;
  visit.push_back(entry->id);
  q.push(entry);
  while (!q.empty()) {
    BasicBlock* p = q.front();
    q.pop();
    for (int i = 0; i < p->nextBlock.size(); i++) {
      vector<int>::iterator it =
          find(visit.begin(), visit.end(), p->nextBlock[i]->id);
      if (it == visit.end()) {
        vector<string> def1;
        vector<string> use1;
        for (int j = 0; j < p->inst.size(); j++) {
          if (p->inst[j].index() == 0) {
            shared_ptr<Inst> ins = get<0>(p->inst[j]);
            if (ins->inst_kind == mir::inst::InstKind::Assign) {
              shared_ptr<mir::inst::AssignInst> in =
                  std::static_pointer_cast<mir::inst::AssignInst>(ins);
              if (in->src.index == 2) {
                if (!has_this(use1, get<2>(in->src)) &&
                    !has_this(def1, get<2>(in->src))) {
                  use1.push_back(get<2>(in->src));
                }
              }
            } else if (ins->inst_kind == mir::inst::InstKind::Op) {
              shared_ptr<mir::inst::OpInst> in =
                  std::static_pointer_cast<mir::inst::OpInst>(ins);
              if (!has_this(use1, get<2>(in->lhs)) &&
                  !has_this(def1, get<2>(in->lhs))) {
                use1.push_back(get<2>(in->lhs));
              }
              if (!has_this(use1, get<2>(in->rhs)) &&
                  !has_this(def1, get<2>(in->rhs))) {
                use1.push_back(get<2>(in->rhs));
              }
            } else if (ins->inst_kind == mir::inst::InstKind::Load) {
              shared_ptr<mir::inst::LoadInst> in =
                  std::static_pointer_cast<mir::inst::LoadInst>(ins);
              if (!has_this(use1, get<2>(in->src)) &&
                  !has_this(def1, get<2>(in->src))) {
                use1.push_back(get<2>(in->src));
              }
            } else if (ins->inst_kind == mir::inst::InstKind::Store) {
              shared_ptr<mir::inst::StoreInst> in =
                  std::static_pointer_cast<mir::inst::StoreInst>(ins);
              if (!has_this(use1, get<2>(in->val)) &&
                  !has_this(def1, get<2>(in->val))) {
                use1.push_back(get<2>(in->val));
              }
            } else if (ins->inst_kind == mir::inst::InstKind::PtrOffset) {
              shared_ptr<mir::inst::PtrOffsetInst> in =
                  std::static_pointer_cast<mir::inst::PtrOffsetInst>(ins);
              if (!has_this(use1, get<2>(in->offset)) &&
                  !has_this(def1, get<2>(in->offset))) {
                use1.push_back(get<2>(in->offset));
              }
              // ptr
            } else if (ins->inst_kind == mir::inst::InstKind::Ref) {
              shared_ptr<mir::inst::RefInst> in =
                  std::static_pointer_cast<mir::inst::RefInst>(ins);
              if (!has_this(use1, get<2>(in->val)) &&
                  !has_this(def1, get<2>(in->val))) {
                use1.push_back(get<2>(in->val));
              }
            } else {
              // nothing
            }
          }
        }
        def.insert(
            map<int, vector<string>>::value_type(p->nextBlock[i]->id, def1));
        use.insert(
            map<int, vector<string>>::value_type(p->nextBlock[i]->id, use1));
        visit.push_back(p->nextBlock[i]->id);
        q.push(p->nextBlock[i]);
      }
    }
  }
  // step2: fill the in and out
  bool flag = true;
  while (flag) {
    flag = false;
    map<int, BasicBlock*>::iterator iter;
    for (iter = nodes.begin(); iter != nodes.end(); iter++) {
      vector<string> _out = calcu_out(iter->second, in);
      vector<string> _in = calcu_in(iter->first, use, out, def);
      map<int, vector<string>>::iterator it = in.find(iter->first);
      if (vectors_difference(_in, it->second).size() != 0) {
        flag = true;
        out.insert(map<int, vector<string>>::value_type(iter->first, _out));
        in.insert(map<int, vector<string>>::value_type(iter->first, _in));
      }
    }
  }
  // step3: find the global var
  map<int, vector<string>> global;
  map<int, vector<string>>::iterator it;
  for (it = def.begin(); it != def.end(); it++) {
    map<int, vector<string>>::iterator iter = out.find(it->first);
    global.insert(map<int, vector<string>>::value_type(
        it->first, vectors_intersection(it->second, iter->second)));
  }
  return global;
}

map<int, BasicBlock*> generate_SSA(map<int, BasicBlock*> nodes,
                                   map<int, vector<string>> global,
                                   map<int, vector<int>> dom_f) {
  map<phi_index, vector<phi_info>> info;
  //对每个块的全局变量，建立info这个map
  map<int, vector<string>>::iterator it;
  map<string, int> note;
  for (it = global.begin(); it != global.end(); it++) {
    for (int j = 0; j < it->second.size(); j++) {
      note.insert(map<string, int>::value_type(it->second[j], 0));
    }
  }
  for (it = global.begin(); it != global.end(); it++) {
    for (int j = 0; j < it->second.size(); j++) {
      map<int, vector<int>>::iterator ite = dom_f.find(it->first);
      for (int i = 0; i < ite->second.size(); i++) {
        phi_index index;
        index.m = ite->second[i];
        index.name = it->second[j];
        phi_info inf;
        inf.m = ite->second[i];
        inf.name = it->second[j];
        inf.n = it->first;
        map<string, int>::iterator itt = note.find(inf.name);
        inf.num = itt->second;
        note.insert(map<string, int>::value_type(inf.name, inf.num + 1));
        map<phi_index, vector<phi_info>>::iterator iter = info.find(index);
        if (iter == info.end()) {
          vector<phi_info> v;
          v.push_back(inf);
          info.insert(map<phi_index, vector<phi_info>>::value_type(index, v));
        } else {
          vector<phi_info> v = iter->second;
          v.push_back(inf);
          info.insert(map<phi_index, vector<phi_info>>::value_type(index, v));
        }
      }
    }
  }
}

/*map<int, Node> generate_CFG(vector<Instruction> instructions) {
    map<int, Node> nodes;
    Entry entry;
    Node n;
    n.emplace<1>(entry);
    nodes.insert(map<int, Node>::value_type(-2, n));
    for (int i = 0; i < instructions.size(); i++) {
        BasicBlock block;
        block.lid = -1;
        block.nid = -3;
        //entry的lid为-2，第一块的lid为-1，最后一块nid为-3
        //理论上除了第一块，所有基本块第一句是label
        if (instructions[i].index() == 2) {
            block.lid = get<2>(instructions[i])->_jumpLabelId;
            i++;
        }
        while(instructions[i].index() != 2 && i < instructions.size()) {
            block.inst.push_back(instructions[i]);
            i++;
        }
        if (i < instructions.size()) {
            block.nid = get<2>(instructions[i])->_jumpLabelId;
        }
        n.emplace<0>(block);
        nodes.insert(map<int, Node>::value_type(block.lid, n));
    }
    BasicBlock endb;
    endb.lid = -3;
    endb.nid = -4;
    n.emplace<0>(endb);
    nodes.insert(map<int, Node>::value_type(endb.lid, n));
    map<int, Node>::iterator iter = nodes.begin();
    while (iter != nodes.end()) {
        if (iter->second.index() == 0) {
            BasicBlock *b = get_if<0>(&iter->second);
            if (b->inst[b->inst.size() - 1].index() == 0) {
                map<int, Node>::iterator it = nodes.find(b->nid);
                if (it != nodes.end()) {
                    b->nextBlock.push_back(get_if<0>(&it->second));
                }
            }
            else if (b->inst[b->inst.size() - 1].index() == 1) {
                int l1 = get<1>(b->inst[b->inst.size() - 1])->bb_true;
                int l2 = get<1>(b->inst[b->inst.size() - 1])->bb_false;
                map<int, Node>::iterator it = nodes.find(l1);
                if (it != nodes.end()) {
                    b->nextBlock.push_back(get_if<0>(&it->second));
                }
                map<int, Node>::iterator it = nodes.find(l2);
                if (it != nodes.end()) {
                    b->nextBlock.push_back(get_if<0>(&it->second));
                }
            }
        }
        else{
            Entry* p = get_if<1>(&iter->second);
            p->nextBlock.push_back(get_if<0>(&nodes[1]));
        }
    }
    return nodes;
}*/

/*map<int, Node> generate_SSA(map<int, Node> cfg, vector<Instruction>
instructions) { map<mir::inst::VarId, int> dests;

}*/