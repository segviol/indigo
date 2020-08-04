#include "optim_mir.hpp"

using namespace std;
using namespace front::optim_mir;

BasicBlock::BasicBlock(int _id, int _pre_id) {
  id = _id;
  pre_id = _pre_id;
}

vector<int> order;
vector<mir::inst::VarId> notRename;

map<int, BasicBlock*> generate_CFG(
    vector<front::irGenerator::Instruction> instructions,
    front::irGenerator::irGenerator& irgenerator) {
  map<int, BasicBlock*> nodes;  // pair(labelId, Node)
  BasicBlock* entry = new BasicBlock(-1, -1);
  nodes.insert(map<int, BasicBlock*>::value_type(-1, entry));
  int preid = -1;  // tight front block
  order.clear();
  // step1: builds the nodes
  for (int i = 0; i < instructions.size();) {
    BasicBlock* block;
    if (instructions[i].index() == 2) {  // first instruction is JumpLableId
      block = new BasicBlock(get<2>(instructions[i])->_jumpLabelId, preid);
      block->inst.push_back(instructions[i]);
      preid = get<2>(instructions[i])->_jumpLabelId;
      i++;
    } else {  // this else branch should not be entered
      int id = irgenerator.getNewLabelId();
      block = new BasicBlock(id, preid);
      shared_ptr<front::irGenerator::JumpLabelId> j(
          new front::irGenerator::JumpLabelId(id));
      block->inst.push_back(j);
      preid = id;
    }
    // the first instruction in the basic block must be the jumplabel
    order.push_back(preid);
    // the instructions between the br and labelid can be deleted
    bool add_ins = true;
    while (i < instructions.size() && instructions[i].index() != 2) {
      if (add_ins) {
        block->inst.push_back(instructions[i]);
      }
      i++;
      if (instructions[i - 1].index() == 1) {
        // after the branch instruction is encountered
        // the following non LabelID is no longer added to the basic block
        add_ins = false;
      }
    }
    nodes.insert(map<int, BasicBlock*>::value_type(preid, block));
  }
  BasicBlock* exit = new BasicBlock(-2, preid);
  nodes.insert(map<int, BasicBlock*>::value_type(-2, exit));

  // step2: add the edges between nodes and build the control flow graph
  map<int, BasicBlock*>::iterator iter;
  for (iter = nodes.begin(); iter != nodes.end(); iter++) {
    map<int, BasicBlock*>::iterator it = nodes.find(iter->second->pre_id);
    if (it != nodes.end() &&
        iter->second->id != -1) {  // find the pre node and not the entry node
      if (it->second->inst.size() >
          1) {  // has instruction(the first instruction is label)
        if (it->second->inst[it->second->inst.size() - 1].index() ==
            0) {  // the last instruction isn't br
          // add a br instruction to iter->second
          std::optional<mir::inst::VarId> v;
          shared_ptr<mir::inst::JumpInstruction> ir_br =
              shared_ptr<mir::inst::JumpInstruction>(
                  new mir::inst::JumpInstruction(
                      mir::inst::JumpInstructionKind::Br, iter->second->id, -1,
                      v, mir::inst::JumpKind::Branch));
          front::irGenerator::Instruction instr;
          instr.emplace<1>(ir_br);
          it->second->inst.push_back(instr);
          // add the edge
          iter->second->preBlock.push_back(it->second);
          it->second->nextBlock.push_back(iter->second);
        } else if (it->second->inst[it->second->inst.size() - 1].index() ==
                   1) {  // the last instruction is br
          BasicBlock* b = it->second;
          shared_ptr<mir::inst::JumpInstruction> ins =
              get<1>(b->inst[b->inst.size() - 1]);
          if (ins->kind ==
              mir::inst::JumpInstructionKind::Return) {  // final block
            exit->preBlock.push_back(it->second);
            it->second->nextBlock.push_back(exit);
          } else {
            int l1 = get<1>(b->inst[b->inst.size() - 1])->bb_true;
            map<int, BasicBlock*>::iterator itt = nodes.find(l1);
            if (itt != nodes.end()) {
              b->nextBlock.push_back(itt->second);
              itt->second->preBlock.push_back(b);
            }
            if (get<1>(b->inst[b->inst.size() - 1])->kind ==
                mir::inst::JumpInstructionKind::BrCond) {
              l1 = get<1>(b->inst[b->inst.size() - 1])->bb_false;
              itt = nodes.find(l1);
              if (itt != nodes.end()) {
                b->nextBlock.push_back(itt->second);
                itt->second->preBlock.push_back(b);
              }
            }
          }
        }
      } else {  // no instruction
        std::optional<mir::inst::VarId> v;
        shared_ptr<mir::inst::JumpInstruction> ir_br =
            shared_ptr<mir::inst::JumpInstruction>(
                new mir::inst::JumpInstruction(
                    mir::inst::JumpInstructionKind::Br, iter->second->id, -1, v,
                    mir::inst::JumpKind::Branch));
        front::irGenerator::Instruction instr;
        instr.emplace<1>(ir_br);
        it->second->inst.push_back(instr);
        iter->second->preBlock.push_back(it->second);
        it->second->nextBlock.push_back(iter->second);
      }
    }
  }

  // step3: delete node which do not have preBlock expect entry
  vector<int> del;
  for (iter = nodes.begin(); iter != nodes.end(); iter++) {
    if (iter->first != -1 && iter->second->preBlock.size() == 0) {
      del.push_back(iter->first);
      for (int i = 0; i < iter->second->nextBlock.size(); i++) {
        iter->second->nextBlock[i]->preBlock.erase(
            std::remove(iter->second->nextBlock[i]->preBlock.begin(),
                        iter->second->nextBlock[i]->preBlock.end(),
                        iter->second),
            iter->second->nextBlock[i]->preBlock.end());
      }
    }
  }
  for (int i = 0; i < del.size(); i++) {
    iter = nodes.find(del[i]);
    if (iter != nodes.end()) {
      nodes.erase(iter);
    }
    vector<int>::iterator j;
    for (j = order.begin(); j != order.end(); j++) {
      if (*j == del[i]) {
        order.erase(j);
        break;
      }
    }
  }

  cout << endl << "*** desplay CFG ***" << endl;
  for (iter = nodes.begin(); iter != nodes.end(); iter++) {
    cout << "node: " << setw(7) << iter->first << " successor node(s): ";
    if (iter->second->nextBlock.size() == 0) {
      cout << "               ";
    } else if (iter->second->nextBlock.size() == 1) {
      cout << setw(7) << iter->second->nextBlock[0]->id << "        ";
    } else {
      cout << setw(7) << iter->second->nextBlock[0]->id << " " << setw(7)
           << iter->second->nextBlock[1]->id;
    }
    cout << " precursor node(s):";
    for (int i = 0; i < iter->second->preBlock.size(); i++) {
      cout << " " << setw(7) << iter->second->preBlock[i]->id;
    }
    cout << endl;
  }
  return nodes;
}

BasicBlock* find_entry(map<int, BasicBlock*> nodes) {
  map<int, BasicBlock*>::iterator iter = nodes.find(-1);
  return iter->second;
}

BasicBlock* find_exit(map<int, BasicBlock*> nodes) {
  map<int, BasicBlock*>::iterator iter = nodes.find(-2);
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

bool has_this(vector<mir::inst::VarId> v, mir::inst::VarId s) {
  for (int i = 0; i < v.size(); i++) {
    if (v[i] == s) {
      return true;
    }
  }
  return false;
}

vector<mir::inst::VarId> vectors_set_union(vector<mir::inst::VarId> v1,
                                           vector<mir::inst::VarId> v2) {
  vector<mir::inst::VarId> v;
  sort(v1.begin(), v1.end());
  sort(v2.begin(), v2.end());
  set_union(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(v));
  return v;
}

vector<mir::inst::VarId> vectors_difference(vector<mir::inst::VarId> v1,
                                            vector<mir::inst::VarId> v2) {
  vector<mir::inst::VarId> v;
  sort(v1.begin(), v1.end());
  sort(v2.begin(), v2.end());
  set_difference(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(v));
  return v;
}

vector<mir::inst::VarId> vectors_intersection(vector<mir::inst::VarId> v1,
                                              vector<mir::inst::VarId> v2) {
  vector<mir::inst::VarId> v;
  sort(v1.begin(), v1.end());
  sort(v2.begin(), v2.end());
  set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(),
                   back_inserter(v));
  return v;
}

vector<mir::inst::VarId> calcu_out(BasicBlock* b,
                                   map<int, vector<mir::inst::VarId>> in) {
  // out[b] = U(in[s]) s are the successor of b
  vector<mir::inst::VarId> v1;
  if (b->nextBlock.size() > 0) {
    map<int, vector<mir::inst::VarId>>::iterator it =
        in.find(b->nextBlock[0]->id);
    v1 = it->second;
    for (int i = 1; i < b->nextBlock.size(); i++) {
      it = in.find(b->nextBlock[i]->id);
      vector<mir::inst::VarId> v2 = it->second;
      v1 = vectors_set_union(v1, v2);
    }
  }
  return v1;
}

vector<mir::inst::VarId> calcu_in(int n, map<int, vector<mir::inst::VarId>> use,
                                  map<int, vector<mir::inst::VarId>> out,
                                  map<int, vector<mir::inst::VarId>> def) {
  // in[b] = use[b] U (out[b] - def[b])
  map<int, vector<mir::inst::VarId>>::iterator it;
  it = out.find(n);
  vector<mir::inst::VarId> v1 = it->second;
  it = def.find(n);
  v1 = vectors_difference(v1, it->second);
  it = use.find(n);
  return vectors_set_union(v1, it->second);
}

set<int> blockHasVar(mir::inst::VarId var,
                     map<int, vector<mir::inst::VarId>> def,
                     map<int, vector<mir::inst::VarId>> use) {
  map<int, vector<mir::inst::VarId>>::iterator it;
  set<int> s;
  for (it = def.begin(); it != def.end(); it++) {
    for (int i = 0; i < it->second.size(); i++) {
      if (it->second[i] == var) {
        s.insert(it->first);
        break;
      }
    }
  }
  for (it = use.begin(); it != use.end(); it++) {
    for (int i = 0; i < it->second.size(); i++) {
      if (it->second[i] == var) {
        s.insert(it->first);
        break;
      }
    }
  }
  return s;
}

map<mir::inst::VarId, set<int>> active_var(map<int, BasicBlock*> nodes) {
  map<int, vector<mir::inst::VarId>> def;
  map<int, vector<mir::inst::VarId>> use;
  map<int, vector<mir::inst::VarId>> in;
  map<int, vector<mir::inst::VarId>> out;
  BasicBlock* entry = find_entry(nodes);
  map<int, vector<mir::inst::VarId>>::iterator it;
  // step0: initialization
  map<int, BasicBlock*>::iterator iter;
  for (iter = nodes.begin(); iter != nodes.end(); iter++) {
    vector<mir::inst::VarId> v;
    def.insert(map<int, vector<mir::inst::VarId>>::value_type(iter->first, v));
    use.insert(map<int, vector<mir::inst::VarId>>::value_type(iter->first, v));
    in.insert(map<int, vector<mir::inst::VarId>>::value_type(iter->first, v));
    out.insert(map<int, vector<mir::inst::VarId>>::value_type(iter->first, v));
  }
  // step1: Find the def and use variables for each block
  // BFS
  vector<int> visit;
  std::queue<int> q;
  visit.push_back(entry->id);
  q.push(entry->id);
  while (!q.empty()) {
    map<int, BasicBlock*>::iterator itt = nodes.find(q.front());
    BasicBlock* p = itt->second;
    q.pop();
    for (int i = 0; i < p->nextBlock.size(); i++) {
      vector<int>::iterator it =
          find(visit.begin(), visit.end(), p->nextBlock[i]->id);
      if (it == visit.end()) {
        vector<mir::inst::VarId> def1;
        vector<mir::inst::VarId> use1;
        for (int j = 0; j < p->nextBlock[i]->inst.size(); j++) {
          if (p->nextBlock[i]->inst[j].index() == 0) {
            shared_ptr<mir::inst::Inst> ins = get<0>(p->nextBlock[i]->inst[j]);
            if (ins->inst_kind() == mir::inst::InstKind::Assign) {
              shared_ptr<mir::inst::AssignInst> in =
                  std::static_pointer_cast<mir::inst::AssignInst>(ins);
              if (in->src.index() == 1) {
                if (!has_this(use1, get<1>(in->src)) &&
                    !has_this(def1, get<1>(in->src))) {
                  mir::inst::VarId s = get<1>(in->src);
                  use1.push_back(s);
                }
              }
              if (!has_this(use1, in->dest) && !has_this(def1, in->dest)) {
                def1.push_back(in->dest);
              }
            } else if (ins->inst_kind() == mir::inst::InstKind::Op) {
              shared_ptr<mir::inst::OpInst> in =
                  std::static_pointer_cast<mir::inst::OpInst>(ins);
              if (in->lhs.index() == 1) {
                if (!has_this(use1, get<1>(in->lhs)) &&
                    !has_this(def1, get<1>(in->lhs))) {
                  use1.push_back(get<1>(in->lhs));
                }
              }
              if (in->rhs.index() == 1) {
                if (!has_this(use1, get<1>(in->rhs)) &&
                    !has_this(def1, get<1>(in->rhs))) {
                  use1.push_back(get<1>(in->rhs));
                }
              }
              if (!has_this(use1, in->dest) && !has_this(def1, in->dest)) {
                def1.push_back(in->dest);
              }
            } else if (ins->inst_kind() == mir::inst::InstKind::Call) {
              shared_ptr<mir::inst::CallInst> in =
                  std::static_pointer_cast<mir::inst::CallInst>(ins);
              for (int k = 0; k < in->params.size(); k++) {
                if (in->params[k].index() == 1) {
                  if (!has_this(use1, get<1>(in->params[k])) &&
                      !has_this(def1, get<1>(in->params[k]))) {
                    mir::inst::VarId s = get<1>(in->params[k]);
                    use1.push_back(s);
                  }
                }
              }
              if (!has_this(use1, in->dest) && !has_this(def1, in->dest)) {
                def1.push_back(in->dest);
              }
            } else if (ins->inst_kind() == mir::inst::InstKind::Ref) {
              shared_ptr<mir::inst::RefInst> in =
                  std::static_pointer_cast<mir::inst::RefInst>(ins);
              if (in->val.index() == 0) {
                if (!has_this(use1, get<0>(in->val)) &&
                    !has_this(def1, get<0>(in->val))) {
                  use1.push_back(get<0>(in->val));
                }
              }
              if (!has_this(use1, in->dest) && !has_this(def1, in->dest)) {
                def1.push_back(in->dest);
              }
            } else if (ins->inst_kind() == mir::inst::InstKind::Load) {
              shared_ptr<mir::inst::LoadInst> in =
                  std::static_pointer_cast<mir::inst::LoadInst>(ins);
              if (in->src.index() == 1) {
                if (!has_this(use1, get<1>(in->src)) &&
                    !has_this(def1, get<1>(in->src))) {
                  use1.push_back(get<1>(in->src));
                }
              }
              if (!has_this(use1, in->dest) && !has_this(def1, in->dest)) {
                def1.push_back(in->dest);
              }
            } else if (ins->inst_kind() == mir::inst::InstKind::Store) {
              shared_ptr<mir::inst::StoreInst> in =
                  std::static_pointer_cast<mir::inst::StoreInst>(ins);
              if (in->val.index() == 1) {
                if (!has_this(use1, get<1>(in->val)) &&
                    !has_this(def1, get<1>(in->val))) {
                  use1.push_back(get<1>(in->val));
                }
              }
              if (!has_this(use1, in->dest) && !has_this(def1, in->dest)) {
                def1.push_back(in->dest);
              }
            } else if (ins->inst_kind() == mir::inst::InstKind::PtrOffset) {
              shared_ptr<mir::inst::PtrOffsetInst> in =
                  std::static_pointer_cast<mir::inst::PtrOffsetInst>(ins);
              if (in->offset.index() == 1) {
                if (!has_this(use1, get<1>(in->offset)) &&
                    !has_this(def1, get<1>(in->offset))) {
                  use1.push_back(get<1>(in->offset));
                }
              }
              if (!has_this(use1, in->ptr) && !has_this(def1, in->ptr)) {
                use1.push_back(in->ptr);
              }
              if (!has_this(use1, in->dest) && !has_this(def1, in->dest)) {
                def1.push_back(in->dest);
              }
            } else {
              // nothing
            }
          } else if (p->nextBlock[i]->inst[j].index() == 1) {
            shared_ptr<mir::inst::JumpInstruction> ins =
                get<1>(p->nextBlock[i]->inst[j]);
            if (ins->kind == mir::inst::JumpInstructionKind::BrCond ||
                ins->kind == mir::inst::JumpInstructionKind::Return) {
              mir::inst::VarId var;
              if (ins->cond_or_ret && !has_this(use1, *ins->cond_or_ret) &&
                  !has_this(def1, *ins->cond_or_ret)) {
                use1.push_back(*ins->cond_or_ret);
              }
            }
          }
        }
        def[p->nextBlock[i]->id] = def1;
        use[p->nextBlock[i]->id] = use1;
        visit.push_back(p->nextBlock[i]->id);
        q.push(p->nextBlock[i]->id);
      }
    }
  }
  // output for test
  /*for (it = def.begin(); it != def.end(); it++) {
      cout << "def " << it->first;
      for (int i = 0; i < it->second.size(); i++) {
          cout << " " << it->second[i];
      }
      cout << endl;
  }
  for (it = use.begin(); it != use .end(); it++) {
      cout << "use " << it->first;
      for (int i = 0; i < it->second.size(); i++) {
          cout << " " << it->second[i];
      }
      cout << endl;
  }*/

  // step2: fill the in and out
  bool flag = true;
  while (flag) {
    flag = false;
    for (iter = nodes.begin(); iter != nodes.end(); iter++) {
      // cout << "|" << iter->second->id << endl;
      vector<mir::inst::VarId> _out = calcu_out(iter->second, in);
      out[iter->first] = _out;
      vector<mir::inst::VarId> _in = calcu_in(iter->first, use, out, def);
      it = in.find(iter->first);
      if (vectors_difference(_in, it->second).size() != 0) {
        flag = true;
        in[iter->first] = _in;
      }
    }
  }
  // output for test
  /*for (it = in.begin(); it != in.end(); it++) {
      cout << it->first;
      for (int i = 0; i < it->second.size(); i++) {
          cout << " " << it->second[i];
      }
      cout << endl;
  }
  for (it = out.begin(); it != out.end(); it++) {
      cout << it->first;
      for (int i = 0; i < it->second.size(); i++) {
          cout << " " << it->second[i];
      }
      cout << endl;
  }*/

  // step3: find the global var -- def && out
  map<int, vector<mir::inst::VarId>> global;
  for (it = def.begin(); it != def.end(); it++) {
    map<int, vector<mir::inst::VarId>>::iterator iter = out.find(it->first);
    global.insert(map<int, vector<mir::inst::VarId>>::value_type(
        it->first, vectors_intersection(it->second, iter->second)));
  }
  // output for test
  /*cout << endl << "*** desplay global var ***" << endl;
  for (it = global.begin(); it != global.end(); it++) {
      cout << it->first;
      for (int i = 0; i < it->second.size(); i++) {
          cout << " " << it->second[i];
      }
      cout << endl;
  }*/

  // step4: build the blocks.
  map<mir::inst::VarId, set<int>> blocks;
  // mir::inst::VarId s0(0);
  for (it = global.begin(); it != global.end(); it++) {
    if (it->second.size() > 0) {
      for (int i = 0; i < it->second.size(); i++) {
        // if (it->second[i] != s0) {
        set<int> s = blockHasVar(it->second[i], def, use);
        blocks.insert(
            map<mir::inst::VarId, set<int>>::value_type(it->second[i], s));
        //}
      }
    }
  }
  cout << endl << "*** desplay blocks ***" << endl;
  map<mir::inst::VarId, set<int>>::iterator t;
  for (t = blocks.begin(); t != blocks.end(); t++) {
    cout << t->first;
    set<int>::iterator setit;
    for (setit = t->second.begin(); setit != t->second.end(); setit++) {
      cout << " " << *setit;
    }
    cout << endl;
  }
  return blocks;
}

void output(vector<int> v) {
  for (int i = 0; i < v.size(); i++) {
    cout << v[i] << " ";
  }
  cout << endl;
}

vector<int> pred_intersection(map<int, BasicBlock*> nodes, int n,
                              map<int, vector<int>> dom) {
  // intersect dom of all direct precursor nodes of n and merged with dom of n
  map<int, BasicBlock*>::iterator it = nodes.find(n);
  // cout << "$$" << n << endl;
  if (it != nodes.end()) {
    BasicBlock* b = it->second;
    map<int, vector<int>>::iterator iter = dom.find(n);
    vector<int> v1 = iter->second;
    // cout << "$$" << b->preBlock.size() << endl;
    if (b->preBlock.size() == 1) {
      iter = dom.find(b->preBlock[0]->id);
      v1 = vectors_set_union(v1, iter->second);
      // cout << b->id << "**" << v1[0] << endl;
      return v1;
    } else if (b->preBlock.size() >= 2) {
      iter = dom.find(b->preBlock[0]->id);
      vector<int> v2 = iter->second;
      for (int i = 1; i < b->preBlock.size(); i++) {
        iter = dom.find(b->preBlock[i]->id);
        v2 = vectors_intersection(v2, iter->second);
      }
      return vectors_set_union(v1, v2);
    } else {
      // error
    }
  }
  vector<int> v;
  return v;
}

map<int, int> find_idom(map<int, BasicBlock*> nodes) {
  BasicBlock* exit = find_exit(nodes);

  // step1: Calculate the domin nodes of each block
  // rule: Dom(n) = {n} ∩ Dom(Pred(n))
  map<int, vector<int>> dom;

  // step1.1: initialize dom
  map<int, BasicBlock*>::iterator it;
  vector<int> N;
  for (int i = 0; i < order.size(); i++) {
    N.push_back(order[i]);
  }
  N.push_back(-2);
  N.push_back(-1);
  for (it = nodes.begin(); it != nodes.end(); it++) {
    if (it->first == -1) {
      vector<int> pre;
      pre.push_back(it->first);
      dom.insert(map<int, vector<int>>::value_type(it->first, pre));
    } else {
      dom.insert(map<int, vector<int>>::value_type(it->first, N));
    }
  }

  // step1.2: Loop until dom does not change
  bool flag = true;
  map<int, vector<int>>::iterator iter;
  while (flag) {
    flag = false;
    for (int i = 0; i < N.size(); i++) {
      it = nodes.find(N[i]);
      vector<int> temp;
      temp.push_back(N[i]);
      if (it->second->preBlock.size() > 0) {
        iter = dom.find(it->second->preBlock[0]->id);
        vector<int> v1 = iter->second;
        for (int j = 1; j < it->second->preBlock.size(); j++) {
          iter = dom.find(it->second->preBlock[j]->id);
          v1 = vectors_intersection(v1, iter->second);
        }
        temp = vectors_set_union(temp, v1);
      }
      iter = dom.find(N[i]);
      // the set size monotonically decreases, so if the set size is the same
      // the set contents are the same
      if (iter->second.size() != temp.size()) {
        dom[N[i]] = temp;
        flag = true;
      }
    }
  }
  /*cout << endl << "*** desplay dom ***" << endl;
  for (iter = dom.begin(); iter != dom.end(); iter++) {
      cout << iter->first;
      for (int i = 0; i < iter->second.size(); i++) {
          cout << " " << iter->second[i];
      }
      cout << endl;
  }*/

  // step2: Calculate the immediate domin nodes of each block
  // step2.1: delete node itself;
  for (iter = dom.begin(); iter != dom.end(); iter++) {
    iter->second.erase(
        remove(iter->second.begin(), iter->second.end(), iter->first),
        iter->second.end());
  }

  // step2.2: BFS
  vector<int> visit;
  queue<int> q;
  iter = dom.find(exit->id);
  vector<int> v1 = iter->second;
  if (v1.size() > 0) {
    iter = dom.find(v1[0]);
    vector<int> v2 = iter->second;
    for (int i = 1; i < v1.size(); i++) {
      iter = dom.find(v1[i]);
      v2 = vectors_set_union(v2, iter->second);
    }
    v1 = vectors_difference(v1, v2);
  }
  dom[exit->id] = v1;
  visit.push_back(exit->id);
  q.push(exit->id);
  while (!q.empty()) {
    map<int, BasicBlock*>::iterator itt = nodes.find(q.front());
    BasicBlock* p = itt->second;
    q.pop();
    for (int i = 0; i < p->preBlock.size(); i++) {
      BasicBlock* prep = p->preBlock[i];
      vector<int>::iterator itt = find(visit.begin(), visit.end(), prep->id);
      if (itt == visit.end()) {  // never visit
        iter = dom.find(prep->id);
        vector<int> v1 = iter->second;
        if (v1.size() > 0) {
          iter = dom.find(v1[0]);
          vector<int> v2 = iter->second;
          for (int i = 1; i < v1.size(); i++) {
            iter = dom.find(v1[i]);
            v2 = vectors_set_union(v2, iter->second);
          }
          v1 = vectors_difference(v1, v2);
        }
        dom[prep->id] = v1;
        visit.push_back(prep->id);
        q.push(prep->id);
      }
    }
  }
  /*for (iter = dom.begin(); iter != dom.end(); iter++) {
      cout << iter->first;
      for (int i = 0; i < iter->second.size(); i++) {
          cout << " " << iter->second[i];
      }
      cout << endl;
  }*/
  map<int, int> dom1;
  for (iter = dom.begin(); iter != dom.end(); iter++) {
    if (iter->second.size() > 0) {
      dom1.insert(map<int, int>::value_type(iter->first, iter->second[0]));
    }
  }
  map<int, int>::iterator i;
  cout << endl << "*** desplay idom ***" << endl;
  for (i = dom1.begin(); i != dom1.end(); i++) {
    cout << i->first << " " << i->second << endl;
  }
  return dom1;
}

map<int, vector<int>> dominac_frontier(map<int, int> dom,
                                       map<int, BasicBlock*> nodes) {
  map<int, vector<int>> dom_f;
  map<int, int>::iterator iter;
  for (iter = dom.begin(); iter != dom.end(); iter++) {
    map<int, BasicBlock*>::iterator it = nodes.find(iter->first);
    if (it->second->preBlock.size() > 1) {
      for (int i = 0; i < it->second->preBlock.size(); i++) {
        int runner = it->second->preBlock[i]->id;
        map<int, vector<int>>::iterator itt;
        int idom_b = dom[it->first];
        while (runner != idom_b) {
          // add b to runner's dominance frontier set
          itt = dom_f.find(runner);
          if (itt == dom_f.end()) {
            vector<int> b;
            b.push_back(it->first);
            dom_f[runner] = b;
          } else {
            vector<int> b = itt->second;
            b.push_back(it->first);
            dom_f[runner] = b;
          }
          // runner := idom(runner)
          runner = dom[runner];
        }
      }
    }
  }
  map<int, vector<int>>::iterator i;
  /*cout << endl << "*** desplay dominac frontier ***" << endl;
  for (i = dom_f.begin(); i != dom_f.end(); i++) {
      cout << i->first;
      for (int j = 0; j < i->second.size(); j++) {
          cout << " " << i->second[j];
      }
      cout << endl;
  }*/
  return dom_f;
}

shared_ptr<mir::inst::PhiInst> ir_phi(mir::inst::VarId var, int n) {
  vector<mir::inst::VarId> vars;
  for (int i = 0; i < n; i++) {
    vars.push_back(var);
  }
  return shared_ptr<mir::inst::PhiInst>(new mir::inst::PhiInst(var, vars));
}

int global_id = 65535;
map<mir::inst::VarId, vector<mir::inst::VarId>> name_map;  // pair(oldid,
                                                           // newids)
mir::inst::VarId rename(mir::inst::VarId oldid, int line) {
  //cout << "call rename at : " << line << endl;
  mir::inst::VarId newid(global_id);
  global_id++;
  map<mir::inst::VarId, vector<mir::inst::VarId>>::iterator it =
      name_map.find(oldid);
  if (it == name_map.end()) {
    vector<mir::inst::VarId> v;
    v.push_back(newid);
    name_map.insert(
        map<mir::inst::VarId, vector<mir::inst::VarId>>::value_type(oldid, v));
  } else {
    vector<mir::inst::VarId> v = it->second;
    v.push_back(newid);
    name_map[oldid] = v;
  }
  return newid;
}

vector<mir::inst::VarId> V;
void push(mir::inst::VarId a) { V.push_back(a); }
mir::inst::VarId pop() {
  mir::inst::VarId a = V.back();
  V.pop_back();
  return a;
}
mir::inst::VarId top() { return V.back(); }

void rename_var(mir::inst::VarId id, BasicBlock* b, map<int, BasicBlock*> nodes,
                map<int, vector<int>> dom_tree) {
  mir::inst::VarId ve = top();
  for (int i = 0; i < b->inst.size(); i++) {

    if (b->inst[i].index() == 0) {
      shared_ptr<mir::inst::Inst> ins = get<0>(b->inst[i]);
      if (ins->inst_kind() == mir::inst::InstKind::Assign) {
        shared_ptr<mir::inst::AssignInst> in =
            static_pointer_cast<mir::inst::AssignInst>(ins);
        if (in->src.index() == 1) {
          mir::inst::VarId x = get<1>(in->src);
          if (x == id) {
            in->src.emplace<1>(top());
          }
        }
        if (in->dest == id) {
          mir::inst::VarId xv = rename(id, 813);
          in->dest = xv;
          push(xv);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::Call) {
        shared_ptr<mir::inst::CallInst> in =
            static_pointer_cast<mir::inst::CallInst>(ins);
        for (int k = 0; k < in->params.size(); k++) {
          if (in->params[k].index() == 1) {
            mir::inst::VarId x = get<1>(in->params[k]);
            if (x == id) {
              in->params[k].emplace<1>(top());
            }
          }
        }
        if (in->dest == id) {
          mir::inst::VarId xv = rename(id, 829);
          in->dest = xv;
          push(xv);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::Op) {
        shared_ptr<mir::inst::OpInst> in =
            static_pointer_cast<mir::inst::OpInst>(ins);
        if (in->lhs.index() == 1) {
          mir::inst::VarId x = get<1>(in->lhs);
          if (x == id) {
            in->lhs.emplace<1>(top());
          }
        }
        if (in->rhs.index() == 1) {
          mir::inst::VarId x = get<1>(in->rhs);
          if (x == id) {
            in->rhs.emplace<1>(top());
          }
        }
        if (in->dest == id) {
          mir::inst::VarId xv = rename(id, 849);
          in->dest = xv;
          push(xv);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::Load) {
        shared_ptr<mir::inst::LoadInst> in =
            static_pointer_cast<mir::inst::LoadInst>(ins);
        if (in->src.index() == 1) {
          mir::inst::VarId x = get<1>(in->src);
          if (x == id) {
            in->src.emplace<1>(top());
          }
        }
        if (in->dest == id) {
          mir::inst::VarId xv = rename(id, 863);
          in->dest = xv;
          push(xv);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::Store) {
        shared_ptr<mir::inst::StoreInst> in =
            static_pointer_cast<mir::inst::StoreInst>(ins);
        if (in->val.index() == 1) {
          mir::inst::VarId x = get<1>(in->val);
          if (x == id) {
            in->val.emplace<1>(top());
          }
        }
        if (in->dest == id) {
          in->dest = top();
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::PtrOffset) {
        shared_ptr<mir::inst::PtrOffsetInst> in =
            static_pointer_cast<mir::inst::PtrOffsetInst>(ins);
        if (in->offset.index() == 1) {
          mir::inst::VarId x = get<1>(in->offset);
          if (x == id) {
            in->offset.emplace<1>(top());
          }
        }
        if (in->ptr == id) {
          in->ptr = top();
        }
        if (in->dest == id) {
          mir::inst::VarId xv = rename(id, 892);
          in->dest = xv;
          push(xv);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::Ref) {
        shared_ptr<mir::inst::RefInst> in =
            static_pointer_cast<mir::inst::RefInst>(ins);
        if (in->val.index() == 0) {
          mir::inst::VarId x = get<0>(in->val);
          if (x == id) {
            in->val.emplace<0>(top());
          }
        }
        if (in->dest == id) {
          mir::inst::VarId xv = rename(id, 906);
          in->dest = xv;
          push(xv);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::Phi) {
        shared_ptr<mir::inst::PhiInst> in =
            static_pointer_cast<mir::inst::PhiInst>(ins);
        if (in->dest == id) {
          mir::inst::VarId xv = rename(id, 914);
          in->dest = xv;
          push(xv);
        }
      } else {
        // nothing
      }
    } else if (b->inst[i].index() == 1) {
      shared_ptr<mir::inst::JumpInstruction> ins = get<1>(b->inst[i]);
      if (ins->kind == mir::inst::JumpInstructionKind::BrCond ||
          ins->kind == mir::inst::JumpInstructionKind::Return) {
        if (ins->cond_or_ret && ins->cond_or_ret == id) {
          ins->cond_or_ret = top();
        }
      }
    }
  }
  for (int j = 0; j < b->nextBlock.size(); j++) {
    map<int, BasicBlock*>::iterator iter = nodes.find(b->nextBlock[j]->id);
    BasicBlock* bb = iter->second;
    for (int i = 0; i < bb->inst.size(); i++) {
      if (bb->inst[i].index() == 0) {
        shared_ptr<mir::inst::Inst> ins = get<0>(bb->inst[i]);
        if (ins->inst_kind() == mir::inst::InstKind::Phi) {
          shared_ptr<mir::inst::PhiInst> in =
              static_pointer_cast<mir::inst::PhiInst>(ins);
          if (in->ori_var == id) {
            in->vars.push_back(top());
          }
        }
      }
    }
  }
  map<int, vector<int>>::iterator dtr = dom_tree.find(b->id);
  if (dtr != dom_tree.end()) {
    for (int i = 0; i < dtr->second.size(); i++) {
      map<int, BasicBlock*>::iterator it = nodes.find(dtr->second[i]);
      rename_var(id, it->second, nodes, dom_tree);
    }
  }
  while (top() != ve) {
    pop();
  }
}

vector<mir::inst::VarId> get_vars(
    vector<front::irGenerator::Instruction> instructions) {
  vector<mir::inst::VarId> vars;
  for (int i = 0; i < instructions.size(); i++) {
    if (instructions[i].index() == 0) {
      shared_ptr<mir::inst::Inst> ins = get<0>(instructions[i]);
      if (ins->inst_kind() == mir::inst::InstKind::Assign) {
        shared_ptr<mir::inst::AssignInst> in =
            static_pointer_cast<mir::inst::AssignInst>(ins);
        if (in->src.index() == 1) {
          if (!has_this(vars, get<1>(in->src))) {
            vars.push_back(get<1>(in->src));
          }
        }
        if (!has_this(vars, in->dest)) {
          vars.push_back(in->dest);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::Call) {
        shared_ptr<mir::inst::CallInst> in =
            static_pointer_cast<mir::inst::CallInst>(ins);
        for (int k = 0; k < in->params.size(); k++) {
          if (in->params[k].index() == 1) {
            if (!has_this(vars, get<1>(in->params[k]))) {
              vars.push_back(get<1>(in->params[k]));
            }
          }
        }
        if (!has_this(vars, in->dest)) {
          vars.push_back(in->dest);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::Op) {
        shared_ptr<mir::inst::OpInst> in =
            static_pointer_cast<mir::inst::OpInst>(ins);
        if (in->lhs.index() == 1) {
          if (!has_this(vars, get<1>(in->lhs))) {
            vars.push_back(get<1>(in->lhs));
          }
        }
        if (in->rhs.index() == 1) {
          if (!has_this(vars, get<1>(in->rhs))) {
            vars.push_back(get<1>(in->rhs));
          }
        }
        if (!has_this(vars, in->dest)) {
          vars.push_back(in->dest);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::Load) {
        shared_ptr<mir::inst::LoadInst> in =
            static_pointer_cast<mir::inst::LoadInst>(ins);
        if (in->src.index() == 1) {
          if (!has_this(vars, get<1>(in->src))) {
            vars.push_back(get<1>(in->src));
          }
        }
        if (!has_this(vars, in->dest)) {
          vars.push_back(in->dest);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::Store) {
        shared_ptr<mir::inst::StoreInst> in =
            static_pointer_cast<mir::inst::StoreInst>(ins);
        if (in->val.index() == 1) {
          if (!has_this(vars, get<1>(in->val))) {
            vars.push_back(get<1>(in->val));
          }
        }
        if (!has_this(vars, in->dest)) {
          vars.push_back(in->dest);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::PtrOffset) {
        shared_ptr<mir::inst::PtrOffsetInst> in =
            static_pointer_cast<mir::inst::PtrOffsetInst>(ins);
        if (in->offset.index() == 1) {
          if (!has_this(vars, get<1>(in->offset))) {
            vars.push_back(get<1>(in->offset));
          }
        }
        if (!has_this(vars, in->ptr)) {
          vars.push_back(in->ptr);
        }
        if (!has_this(vars, in->dest)) {
          vars.push_back(in->dest);
        }
      } else if (ins->inst_kind() == mir::inst::InstKind::Ref) {
        shared_ptr<mir::inst::RefInst> in =
            static_pointer_cast<mir::inst::RefInst>(ins);
        if (in->val.index() == 0) {
          if (!has_this(vars, get<0>(in->val))) {
            vars.push_back(get<0>(in->val));
          }
        }
        if (!has_this(vars, in->dest)) {
          vars.push_back(in->dest);
        }
      } else {
        // nothing
      }
    } else if (instructions[i].index() == 1) {
      shared_ptr<mir::inst::JumpInstruction> ins = get<1>(instructions[i]);
      if (ins->kind == mir::inst::JumpInstructionKind::BrCond ||
          ins->kind == mir::inst::JumpInstructionKind::Return) {
        if (ins->cond_or_ret && !has_this(vars, *ins->cond_or_ret)) {
          vars.push_back(*ins->cond_or_ret);
        }
      }
    }
  }
  return vars;
}

map<int, vector<int>> build_dom_tree(map<int, int> dom) {
  map<int, vector<int>> dom_tree;
  map<int, int>::iterator tr;
  for (tr = dom.begin(); tr != dom.end(); tr++) {
    map<int, vector<int>>::iterator dtr = dom_tree.find(tr->second);
    if (dtr == dom_tree.end()) {
      vector<int> v;
      v.push_back(tr->first);
      dom_tree.insert(map<int, vector<int>>::value_type(tr->second, v));
    } else {
      vector<int> v = dtr->second;
      v.push_back(tr->first);
      dom_tree[tr->second] = v;
    }
  }
  cout << endl << "*** desplay dominator tree ***" << endl;
  map<int, vector<int>>::iterator it;
  for (it = dom_tree.begin(); it != dom_tree.end(); it++) {
    cout << setw(7) << it->first << " child(s):";
    for (int i = 0; i < it->second.size(); i++) {
      cout << " " << setw(7) << it->second[i];
    }
    cout << endl;
  }
  return dom_tree;
}

void generate_SSA(map<int, BasicBlock*> nodes,
                  map<mir::inst::VarId, set<int>> global,
                  map<int, vector<int>> dom_f, vector<mir::inst::VarId> vars,
                  map<int, int> dom) {
  map<mir::inst::VarId, set<int>>::iterator it;
  map<int, set<mir::inst::VarId>> phi;
  for (it = global.begin(); it != global.end(); it++) {
    queue<int> q;
    set<int>::iterator s;
    for (s = it->second.begin(); s != it->second.end(); s++) {
      q.push(*s);
    }
    while (!q.empty()) {
      int b = q.front();
      q.pop();
      map<int, vector<int>>::iterator itt = dom_f.find(b);
      if (itt != dom_f.end()) {
        for (int i = 0; i < itt->second.size(); i++) {
          map<int, set<mir::inst::VarId>>::iterator iter =
              phi.find(itt->second[i]);
          if (itt->second[i] >= 0) {
            if (iter == phi.end()) {
              map<int, BasicBlock*>::iterator itera =
                  nodes.find(itt->second[i]);
              shared_ptr<mir::inst::PhiInst> irphi = ir_phi(it->first, 0);
              //(*irphi).display(cout);
              itera->second->inst.insert(itera->second->inst.begin() + 1,
                                         irphi);
              set<mir::inst::VarId> varset;
              varset.insert(it->first);
              phi.insert(map<int, set<mir::inst::VarId>>::value_type(
                  itt->second[i], varset));
              q.push(itt->second[i]);
            } else {
              set<mir::inst::VarId>::iterator ss;
              ss = iter->second.find(it->first);
              if (ss == iter->second.end()) {
                map<int, BasicBlock*>::iterator itera =
                    nodes.find(itt->second[i]);
                shared_ptr<mir::inst::PhiInst> irphi = ir_phi(it->first, 0);
                //(*irphi).display(cout);
                itera->second->inst.insert(itera->second->inst.begin() + 1,
                                           irphi);
                set<mir::inst::VarId> varset = iter->second;
                varset.insert(it->first);
                phi[itt->second[i]] = varset;
                q.push(itt->second[i]);
              }
            }
          }
        }
      }
    }
  }

  // step3: number the vars, don't rename the $0(return value)
  vars = vectors_difference(vars, notRename);
  map<int, vector<int>> dom_tree = build_dom_tree(dom);
  for (int i = 0; i < vars.size(); i++) {
      V.clear();
      push(rename(vars[i], 1157));
      rename_var(vars[i], find_entry(nodes), nodes, dom_tree);
    }
}

mir::inst::VarId refill(map<mir::inst::VarId, mir::inst::VarId> redundantphi,
                        mir::inst::VarId name) {
  map<mir::inst::VarId, mir::inst::VarId>::iterator it =
      redundantphi.find(name);
  if (it == redundantphi.end()) {
    return name;
  } else {
    return it->second;
  }
}

void gen_ssa(map<string, vector<front::irGenerator::Instruction>> f,
             mir::inst::MirPackage& p,
             front::irGenerator::irGenerator& irgenerator) {
  map<string, mir::inst::MirFunction>::iterator it;
  for (it = p.functions.begin(); it != p.functions.end(); it++) {
    map<string, vector<front::irGenerator::Instruction>>::iterator iter =
        f.find(it->first);
    if (iter != f.end()) {
      shared_ptr<mir::types::FunctionTy> type = it->second.type;
      int n = type->params.size();
      notRename.clear();
      for (int i = 1; i <= n; i++) {
        mir::inst::VarId s(i);
        notRename.push_back(s);
      }
      map<mir::inst::VarId, mir::inst::VarId> redundantphi;
      map<int, BasicBlock*> nodes = generate_CFG(iter->second, irgenerator);
      map<mir::inst::VarId, set<int>> blocks = active_var(nodes);
      map<int, int> dom = find_idom(nodes);
      map<int, vector<int>> df = dominac_frontier(dom, nodes);
      vector<mir::inst::VarId> vars = get_vars(iter->second);
      for (int k = 0; k < vars.size(); k++) {
        //cout << vars[k] << endl;
      }
      generate_SSA(nodes, blocks, df, vars, dom);
      map<int, BasicBlock*>::iterator iit = nodes.find(1048576);
      if (iit != nodes.end()) {
        if (iit->second->inst[iit->second->inst.size() - 1].index() == 1) {
          optional<mir::inst::VarId> var =
              get<1>(iit->second->inst[iit->second->inst.size() - 1])
                  ->cond_or_ret;
          if (var.has_value()) {
            mir::inst::VarId s0(0);
            mir::inst::VarId dest(0);
            get<1>(iit->second->inst[iit->second->inst.size() - 1])
                ->cond_or_ret = s0;
            mir::inst::Value src(var.value());
            shared_ptr<mir::inst::Inst> assign =
                shared_ptr<mir::inst::AssignInst>(
                    new mir::inst::AssignInst(dest, src));
            iit->second->inst.insert(
                iit->second->inst.begin() + iit->second->inst.size() - 1,
                assign);
          }
        }
      }
      mir::types::LabelId exitid = irgenerator.getNewLabelId();
      vector<uint32_t> defined;
      for (int i = 0; i <= n; i++) {
        mir::inst::VarId s(i);
        defined.push_back(s);
      }
      for (int i = 0; i < order.size(); i++) {
        map<int, BasicBlock*>::iterator iit = nodes.find(order[i]);
        mir::types::LabelId id = iit->first;
        for (int j = 1; j < iit->second->inst.size() - 1; j++) {
          shared_ptr<mir::inst::Inst> inst = get<0>(iit->second->inst[j]);
          if (inst->inst_kind() == mir::inst::InstKind::Assign) {
            shared_ptr<mir::inst::AssignInst> in =
                static_pointer_cast<mir::inst::AssignInst>(inst);
            defined.push_back(in->dest);
          } else if (inst->inst_kind() == mir::inst::InstKind::Call) {
            shared_ptr<mir::inst::CallInst> in =
                static_pointer_cast<mir::inst::CallInst>(inst);
            if (in->dest != 1048576) {
              defined.push_back(in->dest);
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Op) {
            shared_ptr<mir::inst::OpInst> in =
                static_pointer_cast<mir::inst::OpInst>(inst);
            defined.push_back(in->dest);
          } else if (inst->inst_kind() == mir::inst::InstKind::Load) {
            shared_ptr<mir::inst::LoadInst> in =
                static_pointer_cast<mir::inst::LoadInst>(inst);
            defined.push_back(in->dest);
          } else if (inst->inst_kind() == mir::inst::InstKind::Store) {
            shared_ptr<mir::inst::StoreInst> in =
                static_pointer_cast<mir::inst::StoreInst>(inst);
            defined.push_back(in->dest);
          } else if (inst->inst_kind() == mir::inst::InstKind::PtrOffset) {
            shared_ptr<mir::inst::PtrOffsetInst> in =
                static_pointer_cast<mir::inst::PtrOffsetInst>(inst);
            defined.push_back(in->dest);
          } else if (inst->inst_kind() == mir::inst::InstKind::Ref) {
            shared_ptr<mir::inst::RefInst> in =
                static_pointer_cast<mir::inst::RefInst>(inst);
            defined.push_back(in->dest);
          } else if (inst->inst_kind() == mir::inst::InstKind::Phi) {
            shared_ptr<mir::inst::PhiInst> in =
                static_pointer_cast<mir::inst::PhiInst>(inst);
            set<mir::inst::VarId> vars;
            vars.insert(in->dest);
            for (int k = 0; k < in->vars.size(); k++) {
              vars.insert(in->vars[k]);
            }
            vars.erase(in->dest);
            if (vars.size() == 1) {
              redundantphi.insert(
                  map<mir::inst::VarId, mir::inst::VarId>::value_type(
                      in->dest, *vars.begin()));
            } else {
              defined.push_back(in->dest);
            }
          }
        }
      }
      map<mir::inst::VarId, mir::inst::VarId>::iterator iet;
      for (iet = redundantphi.begin(); iet != redundantphi.end(); iet++) {
        map<mir::inst::VarId, mir::inst::VarId>::iterator iet1;
        for (iet1 = redundantphi.begin(); iet1 != redundantphi.end(); iet1++) {
          if (iet->first == iet1->second) {
            redundantphi[iet1->first] = iet->second;
          }
        }
      }
      // refill
      for (int i = 0; i < order.size(); i++) {
        map<int, BasicBlock*>::iterator iit = nodes.find(order[i]);
        mir::types::LabelId id = iit->first;
        for (int j = 1; j < iit->second->inst.size() - 1; j++) {
          shared_ptr<mir::inst::Inst> inst = get<0>(iit->second->inst[j]);
          if (inst->inst_kind() == mir::inst::InstKind::Assign) {
            shared_ptr<mir::inst::AssignInst> in =
                static_pointer_cast<mir::inst::AssignInst>(inst);
            in->dest = refill(redundantphi, in->dest);
            if (in->src.index() == 1) {
              in->src.emplace<1>(refill(redundantphi, get<1>(in->src)));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Call) {
            shared_ptr<mir::inst::CallInst> in =
                static_pointer_cast<mir::inst::CallInst>(inst);
            if (in->dest != 1048576) {
              in->dest = refill(redundantphi, in->dest);
            }
            for (int k = 0; k < in->params.size(); k++) {
              if (in->params[k].index() == 1) {
                in->params[k].emplace<1>(
                    refill(redundantphi, get<1>(in->params[k])));
              }
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Op) {
            shared_ptr<mir::inst::OpInst> in =
                static_pointer_cast<mir::inst::OpInst>(inst);
            in->dest = refill(redundantphi, in->dest);
            if (in->lhs.index() == 1) {
              in->lhs.emplace<1>(refill(redundantphi, get<1>(in->lhs)));
            }
            if (in->rhs.index() == 1) {
              in->rhs.emplace<1>(refill(redundantphi, get<1>(in->rhs)));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Load) {
            shared_ptr<mir::inst::LoadInst> in =
                static_pointer_cast<mir::inst::LoadInst>(inst);
            in->dest = refill(redundantphi, in->dest);
            if (in->src.index() == 1) {
              in->src.emplace<1>(refill(redundantphi, get<1>(in->src)));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Store) {
            shared_ptr<mir::inst::StoreInst> in =
                static_pointer_cast<mir::inst::StoreInst>(inst);
            in->dest = refill(redundantphi, in->dest);
            if (in->val.index() == 1) {
              in->val.emplace<1>(refill(redundantphi, get<1>(in->val)));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::PtrOffset) {
            shared_ptr<mir::inst::PtrOffsetInst> in =
                static_pointer_cast<mir::inst::PtrOffsetInst>(inst);
            in->dest = refill(redundantphi, in->dest);
            in->ptr = refill(redundantphi, in->ptr);
            if (in->offset.index() == 1) {
              in->offset.emplace<1>(refill(redundantphi, get<1>(in->offset)));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Ref) {
            shared_ptr<mir::inst::RefInst> in =
                static_pointer_cast<mir::inst::RefInst>(inst);
            in->dest = refill(redundantphi, in->dest);
            if (in->val.index() == 0) {
              in->val.emplace<0>(refill(redundantphi, get<0>(in->val)));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Phi) {
            shared_ptr<mir::inst::PhiInst> in =
                static_pointer_cast<mir::inst::PhiInst>(inst);
            // in->dest = refill(redundantphi, in->dest);
            for (int k = 0; k < in->vars.size(); k++) {
              in->vars[k] = refill(redundantphi, in->vars[k]);
            }
          }
        }
        shared_ptr<mir::inst::JumpInstruction> jump =
            get<1>(iit->second->inst[iit->second->inst.size() - 1]);
        if (jump->cond_or_ret) {
          jump->cond_or_ret = refill(redundantphi, *jump->cond_or_ret);
        }
      }
      bool change = true;
      while (change) {
        change = false;
        redundantphi.clear();
        for (int i = 0; i < order.size(); i++) {
          map<int, BasicBlock*>::iterator iit = nodes.find(order[i]);
          for (int j = 1; j < iit->second->inst.size() - 1; j++) {
            shared_ptr<mir::inst::Inst> inst = get<0>(iit->second->inst[j]);
            if (inst->inst_kind() == mir::inst::InstKind::Phi) {
              shared_ptr<mir::inst::PhiInst> in =
                  static_pointer_cast<mir::inst::PhiInst>(inst);
              vector<uint32_t>::iterator fi =
                  find(defined.begin(), defined.end(), in->dest);
              if (fi != defined.end()) {
                set<mir::inst::VarId> vars;
                vars.insert(in->dest);
                for (int k = 0; k < in->vars.size(); k++) {
                  fi = find(defined.begin(), defined.end(), in->vars[k]);

                  if (fi != defined.end()) {
                    vars.insert(in->vars[k]);
                  }
                }
                vars.erase(in->dest);
                if (vars.size() == 1) {
                  redundantphi.insert(
                      map<mir::inst::VarId, mir::inst::VarId>::value_type(
                          in->dest, *vars.begin()));
                }
              }
            }
          }
        }
        if (redundantphi.size() > 0) {
          change = true;
        }
        // delete define
        for (iet = redundantphi.begin(); iet != redundantphi.end(); iet++) {
          vector<uint32_t>::iterator fi =
              find(defined.begin(), defined.end(), iet->first);
          if (fi != defined.end()) {
            defined.erase(fi);
          }
        }
        for (iet = redundantphi.begin(); iet != redundantphi.end(); iet++) {
          map<mir::inst::VarId, mir::inst::VarId>::iterator iet1;
          for (iet1 = redundantphi.begin(); iet1 != redundantphi.end();
               iet1++) {
            if (iet->first == iet1->second) {
              redundantphi[iet1->first] = iet->second;
            }
          }
        }
        for (int i = 0; i < order.size(); i++) {
          map<int, BasicBlock*>::iterator iit = nodes.find(order[i]);
          mir::types::LabelId id = iit->first;
          for (int j = 1; j < iit->second->inst.size() - 1; j++) {
            shared_ptr<mir::inst::Inst> inst = get<0>(iit->second->inst[j]);
            if (inst->inst_kind() == mir::inst::InstKind::Assign) {
              shared_ptr<mir::inst::AssignInst> in =
                  static_pointer_cast<mir::inst::AssignInst>(inst);
              in->dest = refill(redundantphi, in->dest);
              if (in->src.index() == 1) {
                in->src.emplace<1>(refill(redundantphi, get<1>(in->src)));
              }
            } else if (inst->inst_kind() == mir::inst::InstKind::Call) {
              shared_ptr<mir::inst::CallInst> in =
                  static_pointer_cast<mir::inst::CallInst>(inst);
              if (in->dest != 1048576) {
                in->dest = refill(redundantphi, in->dest);
              }
              for (int k = 0; k < in->params.size(); k++) {
                if (in->params[k].index() == 1) {
                  in->params[k].emplace<1>(
                      refill(redundantphi, get<1>(in->params[k])));
                }
              }
            } else if (inst->inst_kind() == mir::inst::InstKind::Op) {
              shared_ptr<mir::inst::OpInst> in =
                  static_pointer_cast<mir::inst::OpInst>(inst);
              in->dest = refill(redundantphi, in->dest);
              if (in->lhs.index() == 1) {
                in->lhs.emplace<1>(refill(redundantphi, get<1>(in->lhs)));
              }
              if (in->rhs.index() == 1) {
                in->rhs.emplace<1>(refill(redundantphi, get<1>(in->rhs)));
              }
            } else if (inst->inst_kind() == mir::inst::InstKind::Load) {
              shared_ptr<mir::inst::LoadInst> in =
                  static_pointer_cast<mir::inst::LoadInst>(inst);
              in->dest = refill(redundantphi, in->dest);
              if (in->src.index() == 1) {
                in->src.emplace<1>(refill(redundantphi, get<1>(in->src)));
              }
            } else if (inst->inst_kind() == mir::inst::InstKind::Store) {
              shared_ptr<mir::inst::StoreInst> in =
                  static_pointer_cast<mir::inst::StoreInst>(inst);
              in->dest = refill(redundantphi, in->dest);
              if (in->val.index() == 1) {
                in->val.emplace<1>(refill(redundantphi, get<1>(in->val)));
              }
            } else if (inst->inst_kind() == mir::inst::InstKind::PtrOffset) {
              shared_ptr<mir::inst::PtrOffsetInst> in =
                  static_pointer_cast<mir::inst::PtrOffsetInst>(inst);
              in->dest = refill(redundantphi, in->dest);
              in->ptr = refill(redundantphi, in->ptr);
              if (in->offset.index() == 1) {
                in->offset.emplace<1>(refill(redundantphi, get<1>(in->offset)));
              }
            } else if (inst->inst_kind() == mir::inst::InstKind::Ref) {
              shared_ptr<mir::inst::RefInst> in =
                  static_pointer_cast<mir::inst::RefInst>(inst);
              in->dest = refill(redundantphi, in->dest);
              if (in->val.index() == 0) {
                in->val.emplace<0>(refill(redundantphi, get<0>(in->val)));
              }
            } else if (inst->inst_kind() == mir::inst::InstKind::Phi) {
              shared_ptr<mir::inst::PhiInst> in =
                  static_pointer_cast<mir::inst::PhiInst>(inst);
              // in->dest = refill(redundantphi, in->dest);
              for (int k = 0; k < in->vars.size(); k++) {
                in->vars[k] = refill(redundantphi, in->vars[k]);
              }
            }
          }
          shared_ptr<mir::inst::JumpInstruction> jump =
              get<1>(iit->second->inst[iit->second->inst.size() - 1]);
          if (jump->cond_or_ret) {
            jump->cond_or_ret = refill(redundantphi, *jump->cond_or_ret);
          }
        }
      }
      for (int i = 0; i < order.size(); i++) {
        map<int, BasicBlock*>::iterator iit = nodes.find(order[i]);
        mir::types::LabelId id = iit->first;
        mir::inst::BasicBlk bb(id);
        for (int j = 0; j < iit->second->preBlock.size(); j++) {
          if (iit->second->preBlock[j]->id >= 0) {
            bb.preceding.insert(iit->second->preBlock[j]->id);
          }
          if (iit->second->preBlock[j]->id == -2) {
            bb.preceding.insert(exitid);
          }
        }
        for (int j = 1; j < iit->second->inst.size() - 1; j++) {
          shared_ptr<mir::inst::Inst> inst = get<0>(iit->second->inst[j]);
          if (inst->inst_kind() == mir::inst::InstKind::Assign) {
            shared_ptr<mir::inst::AssignInst> in =
                static_pointer_cast<mir::inst::AssignInst>(inst);
            vector<uint32_t>::iterator fi =
                find(defined.begin(), defined.end(), in->dest);
            if (fi != defined.end()) {
              bb.inst.push_back(unique_ptr<mir::inst::Inst>(inst.get()));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Call) {
            shared_ptr<mir::inst::CallInst> in =
                static_pointer_cast<mir::inst::CallInst>(inst);
            vector<uint32_t>::iterator fi =
                find(defined.begin(), defined.end(), in->dest);
            if (fi != defined.end()) {
              bb.inst.push_back(unique_ptr<mir::inst::Inst>(inst.get()));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Op) {
            shared_ptr<mir::inst::OpInst> in =
                static_pointer_cast<mir::inst::OpInst>(inst);
            vector<uint32_t>::iterator fi =
                find(defined.begin(), defined.end(), in->dest);
            if (fi != defined.end()) {
              bb.inst.push_back(unique_ptr<mir::inst::Inst>(inst.get()));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Load) {
            shared_ptr<mir::inst::LoadInst> in =
                static_pointer_cast<mir::inst::LoadInst>(inst);
            vector<uint32_t>::iterator fi =
                find(defined.begin(), defined.end(), in->dest);
            if (fi != defined.end()) {
              bb.inst.push_back(unique_ptr<mir::inst::Inst>(inst.get()));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Store) {
            shared_ptr<mir::inst::StoreInst> in =
                static_pointer_cast<mir::inst::StoreInst>(inst);
            vector<uint32_t>::iterator fi =
                find(defined.begin(), defined.end(), in->dest);
            if (fi != defined.end()) {
              bb.inst.push_back(unique_ptr<mir::inst::Inst>(inst.get()));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::PtrOffset) {
            shared_ptr<mir::inst::PtrOffsetInst> in =
                static_pointer_cast<mir::inst::PtrOffsetInst>(inst);
            vector<uint32_t>::iterator fi =
                find(defined.begin(), defined.end(), in->dest);
            if (fi != defined.end()) {
              bb.inst.push_back(unique_ptr<mir::inst::Inst>(inst.get()));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Ref) {
            shared_ptr<mir::inst::RefInst> in =
                static_pointer_cast<mir::inst::RefInst>(inst);
            vector<uint32_t>::iterator fi =
                find(defined.begin(), defined.end(), in->dest);
            if (fi != defined.end()) {
              bb.inst.push_back(unique_ptr<mir::inst::Inst>(inst.get()));
            }
          } else if (inst->inst_kind() == mir::inst::InstKind::Phi) {
            shared_ptr<mir::inst::PhiInst> in =
                static_pointer_cast<mir::inst::PhiInst>(inst);
            vector<uint32_t>::iterator fi =
                find(defined.begin(), defined.end(), in->dest);
            if (fi != defined.end()) {
              bb.inst.push_back(unique_ptr<mir::inst::Inst>(inst.get()));
            }
          }
        }
        shared_ptr<mir::inst::JumpInstruction> jump =
            get<1>(iit->second->inst[iit->second->inst.size() - 1]);
        if (jump->bb_true != -2) {
          bb.jump = move(*jump);
        }
        it->second.basic_blks.insert({id, move(bb)});
      }

      map<uint32_t, mir::inst::Variable>::iterator itt;
      for (itt = it->second.variables.begin();
           itt != it->second.variables.end(); itt++) {
        map<mir::inst::VarId, vector<mir::inst::VarId>>::iterator ite =
            name_map.find(itt->first);
        if (ite != name_map.end()) {
          for (int j = 0; j < ite->second.size(); j++) {
            vector<uint32_t>::iterator fi =
                find(defined.begin(), defined.end(), ite->second[j]);
            if (fi != defined.end()) {
              it->second.variables.insert(
                  map<uint32_t, mir::inst::Variable>::value_type(ite->second[j],
                                                                 itt->second));
            }
          }
        }
      }
      int si = it->second.variables.size();
      bool flag = true;
      while (flag) {
        flag = false;
        for (itt = it->second.variables.begin();
             itt != it->second.variables.end(); itt++) {
          if (itt->first > n && itt->first < 65535 || itt->first == 1048576) {
            it->second.variables.erase(itt);
            flag = true;
            break;
          }
        }
      }

      /*cout << endl << "phi" << endl;
      for (iet = redundantphi.begin(); iet != redundantphi.end(); iet++) {
        cout << iet->first << " " << iet->second << endl;
      }*/
    }
  }
}
