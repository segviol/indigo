#include "optim_mir.hpp"
#include <iostream>

using namespace std;
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
    for (int i = 0; i < instructions.size(); ) {
        BasicBlock* block;
        if (instructions[i].index() == 2) {  // first instruction is JumpLableId
            block = new BasicBlock(get<2>(instructions[i])->_jumpLabelId, preid);
            preid = get<2>(instructions[i])->_jumpLabelId;
            i++;
        }
        else {
            block = new BasicBlock(id, preid);
            preid = id--;
        }
        // br is the ending, JumpLableId is the beginning.
        // The next sentence of br must be JumpLableId.
        while (i < instructions.size() && instructions[i].index() != 2) {
            block->inst.push_back(instructions[i]);
            i++;
        }
        nodes.insert(map<int, BasicBlock*>::value_type(preid, block));
    }
    BasicBlock* exit = new BasicBlock(-2, preid);
    nodes.insert(map<int, BasicBlock*>::value_type(-2, exit));
    // walk each node and generate control flow graph
    map<int, BasicBlock*>::iterator iter;
    for (iter = nodes.begin(); iter != nodes.end(); iter++) {
        map<int, BasicBlock*>::iterator it = nodes.find(iter->second->pre_id);
        if (it != nodes.end() && iter->second->id != -1) {  // find the pre node and not the entry node
            if (it->second->inst.size() > 0) { //has instruction 
                if (it->second->inst[it->second->inst.size() - 1].index() == 0) { //the last instruction isn't br
                    iter->second->preBlock.push_back(it->second);
                    it->second->nextBlock.push_back(iter->second);
                }
                else if (it->second->inst[it->second->inst.size() - 1].index() == 1) { //the last instruction is br
                    BasicBlock* b = it->second;
                    int l1 = get<1>(b->inst[b->inst.size() - 1])->bb_true;
                    map<int, BasicBlock*>::iterator itt = nodes.find(l1);
                    if (itt != nodes.end()) {
                        b->nextBlock.push_back(itt->second);
                        itt->second->preBlock.push_back(b);
                    }
                    if (get<1>(b->inst[b->inst.size() - 1])->kind ==
                        mir::inst::JumpInstructionKind::BrCond
                        ) {
                        l1 = get<1>(b->inst[b->inst.size() - 1])->bb_false;
                        itt = nodes.find(l1);
                        if (itt != nodes.end()) {
                            b->nextBlock.push_back(itt->second);
                            itt->second->preBlock.push_back(b);
                        }
                    }
                }
            }
            else {//no instruction 
                iter->second->preBlock.push_back(it->second);
                it->second->nextBlock.push_back(iter->second);
            }
        }
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
    set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(v));
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

vector<mir::inst::VarId> vectors_set_union(vector<mir::inst::VarId> v1, vector<mir::inst::VarId> v2) {
    vector<mir::inst::VarId> v;
    sort(v1.begin(), v1.end());
    sort(v2.begin(), v2.end());
    set_union(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(v));
    return v;
}

vector<mir::inst::VarId> vectors_difference(vector<mir::inst::VarId> v1, vector<mir::inst::VarId> v2) {
    vector<mir::inst::VarId> v;
    sort(v1.begin(), v1.end());
    sort(v2.begin(), v2.end());
    set_difference(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(v));
    return v;
}

vector<mir::inst::VarId> vectors_intersection(vector<mir::inst::VarId> v1, vector<mir::inst::VarId> v2) {
    vector<mir::inst::VarId> v;
    sort(v1.begin(), v1.end());
    sort(v2.begin(), v2.end());
    set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(), back_inserter(v));
    return v;
}

vector<mir::inst::VarId> calcu_out(BasicBlock* b, map<int, vector<mir::inst::VarId>> in) {
    if (b->nextBlock.size() > 0) {
        map<int, vector<mir::inst::VarId>>::iterator it = in.find(b->nextBlock[0]->id);
        vector<mir::inst::VarId> v1 = it->second;

        for (int i = 1; i < b->nextBlock.size(); i++) {
            it = in.find(b->nextBlock[i]->id);
            vector<mir::inst::VarId> v2 = it->second;
            v1 = vectors_set_union(v1, v2);
        }
        return v1;
    }
}

vector<mir::inst::VarId> calcu_in(int n, map<int, vector<mir::inst::VarId>> use,
    map<int, vector<mir::inst::VarId>> out,
    map<int, vector<mir::inst::VarId>> def) {
    map<int, vector<mir::inst::VarId>>::iterator it;
    it = out.find(n);
    vector<mir::inst::VarId> v1 = it->second;
    it = def.find(n);
    v1 = vectors_difference(v1, it->second);
    it = use.find(n);
    return vectors_set_union(v1, it->second);
}

map<int, vector<mir::inst::VarId>> active_var(map<int, BasicBlock*> nodes) {
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
                for (int j = 0; j < p->inst.size(); j++) {
                    if (p->inst[j].index() == 0) {
                        shared_ptr<mir::inst::Inst> ins = get<0>(p->inst[j]);
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
                            if (!has_this(use1, in->dest) &&
                                !has_this(def1, in->dest)) {
                                def1.push_back(in->dest);
                            }
                        }
                        else if (ins->inst_kind() == mir::inst::InstKind::Op) {
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
                            if (!has_this(use1, in->dest) &&
                                !has_this(def1, in->dest)) {
                                def1.push_back(in->dest);
                            }
                        }
                        else if (ins->inst_kind() == mir::inst::InstKind::Load) {
                            shared_ptr<mir::inst::LoadInst> in =
                                std::static_pointer_cast<mir::inst::LoadInst>(ins);
                            if (in->src.index() == 1) {
                                if (!has_this(use1, get<1>(in->src)) &&
                                    !has_this(def1, get<1>(in->src))) {
                                    use1.push_back(get<1>(in->src));
                                }
                            }
                            if (!has_this(use1, in->dest) &&
                                !has_this(def1, in->dest)) {
                                def1.push_back(in->dest);
                            }
                        }
                        else if (ins->inst_kind() == mir::inst::InstKind::Store) {
                            shared_ptr<mir::inst::StoreInst> in =
                                std::static_pointer_cast<mir::inst::StoreInst>(ins);
                            if (in->val.index() == 1) {
                                if (!has_this(use1, get<1>(in->val)) &&
                                    !has_this(def1, get<1>(in->val))) {
                                    use1.push_back(get<1>(in->val));
                                }
                            }
                            if (!has_this(use1, in->dest) &&
                                !has_this(def1, in->dest)) {
                                def1.push_back(in->dest);
                            }
                        }
                        else if (ins->inst_kind() == mir::inst::InstKind::PtrOffset) {
                            shared_ptr<mir::inst::PtrOffsetInst> in =
                                std::static_pointer_cast<mir::inst::PtrOffsetInst>(ins);
                            if (in->offset.index() == 1) {
                                if (!has_this(use1, get<1>(in->offset)) &&
                                    !has_this(def1, get<1>(in->offset))) {
                                    use1.push_back(get<1>(in->offset));
                                }
                            }
                            if (!has_this(use1, in->ptr) &&
                                !has_this(def1, in->ptr)) {
                                use1.push_back(in->ptr);
                            }
                            if (!has_this(use1, in->dest) &&
                                !has_this(def1, in->dest)) {
                                def1.push_back(in->dest);
                            }
                        }
                        else if (ins->inst_kind() == mir::inst::InstKind::Ref) {
                            shared_ptr<mir::inst::RefInst> in =
                                std::static_pointer_cast<mir::inst::RefInst>(ins);
                            if (in->val.index() == 0) {
                                if (!has_this(use1, get<0>(in->val)) &&
                                    !has_this(def1, get<0>(in->val))) {
                                    use1.push_back(get<0>(in->val));
                                }
                            }
                            if (!has_this(use1, in->dest) &&
                                !has_this(def1, in->dest)) {
                                def1.push_back(in->dest);
                            }
                        }
                        else {
                            // nothing
                        }
                    }
                }
                def.insert(
                    map<int, vector<mir::inst::VarId>>::value_type(p->nextBlock[i]->id, def1));
                use.insert(
                    map<int, vector<mir::inst::VarId>>::value_type(p->nextBlock[i]->id, use1));
                visit.push_back(p->nextBlock[i]->id);
                q.push(p->nextBlock[i]->id);
            }
        }
    }

    /*Construct special data for testing
    vector<mir::inst::VarId> y = { "y" };
    vector<mir::inst::VarId> xy = { "x", "y" };
    vector<mir::inst::VarId> z = { "z" };
    vector<mir::inst::VarId> x = { "x" };
    vector<mir::inst::VarId> w = { "w" };
    vector<mir::inst::VarId> p;
    def[-3] = y;
    def[1] = xy;
    def[2] = x;
    def[3] = w;
    def[4] = z;
    use[-3] = z;
    use[1] = z;
    use[3] = x;
    use[4] = x;*/

    // step2: fill the in and out
    bool flag = true;
    while (flag) {
        flag = false;
        for (iter = nodes.begin(); iter != nodes.end(); iter++) {
            vector<mir::inst::VarId> _out = calcu_out(iter->second, in);
            out[iter->first] = _out;
            vector<mir::inst::VarId> _in = calcu_in(iter->first, use, out, def);
            it = in.find(iter->first);
            if (vectors_difference(_in, it->second).size() != 0) {
                flag = true;
                in[iter->first] = _in;
            }
        }
        /*output for test
        for (it = in.begin(); it != in.end(); it++) {
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
    }
    // step3: find the global var
    map<int, vector<mir::inst::VarId>> global;
    for (it = def.begin(); it != def.end(); it++) {
        map<int, vector<mir::inst::VarId>>::iterator iter = out.find(it->first);
        global.insert(map<int, vector<mir::inst::VarId>>::value_type(
            it->first, vectors_intersection(it->second, iter->second)));
    }
    /*output for test
    for (it = global.begin(); it != global.end(); it++) {
        cout << it->first;
        for (int i = 0; i < it->second.size(); i++) {
            cout << " " << it->second[i];
        }
        cout << endl;
    }*/
    return global;
}

void output(vector<int> v) {
    for (int i = 0; i < v.size(); i++) {
        cout << v[i] << " ";
    }
    cout << endl;
}

vector<int> pred_intersection(map<int, BasicBlock*> nodes, int n,
    map<int, vector<int>> dom) {
    //intersect dom of all direct precursor nodes of n and merged with dom of n
    map<int, BasicBlock*>::iterator it = nodes.find(n);
    if (it != nodes.end()) {
        BasicBlock* b = it->second;
        map<int, vector<int>>::iterator iter = dom.find(n);
        vector<int> v1 = iter->second;
        if (b->preBlock.size() == 1) {
            iter = dom.find(b->preBlock[0]->id);
            v1 = vectors_set_union(v1, iter->second);
            return v1;
        }
        else if (b->preBlock.size() >= 2) {
            iter = dom.find(b->preBlock[0]->id);
            vector<int> v2 = iter->second;
            for (int i = 1; i < b->preBlock.size(); i++) {
                iter = dom.find(b->preBlock[i]->id);
                v2 = vectors_intersection(v2, iter->second);
            }
            return vectors_set_union(v1, v2);
        }
        else {
            // error
        }
    }
    vector<int> v;
    return v;
}

map<int, int> find_idom(map<int, BasicBlock*> nodes) {
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
    /*map<int, vector<int>>::iterator iter;
    for (iter = dom.begin(); iter != dom.end(); iter++) {
        cout << iter->first;
        for (int i = 0; i < iter->second.size(); i++) {
            cout << " " << iter->second[i];
        }
        cout << endl;
    }*/
    // step1.2: Loop until dom does not change
    bool flag = true;
    while (flag) {
        flag = false;
        // BFS
        vector<int> visit;
        std::queue<int> q;
        vector<int> v = pred_intersection(nodes, exit->id, dom);
        map<int, vector<int>>::iterator iter = dom.find(exit->id);
        vector<int> prev = iter->second;
        if (v.size() > prev.size()) {
            flag = true;
            dom[exit->id] = v;
        }
        visit.push_back(exit->id);
        q.push(exit->id);
        while (!q.empty()) {
            map<int, BasicBlock*>::iterator itt = nodes.find(q.front());
            BasicBlock* p = itt->second;
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
                        dom[prep->id] = v;
                    }
                    visit.push_back(prep->id);
                    q.push(prep->id);
                }
            }
        }
    }
    /*map<int, vector<int>>::iterator iter;
    for (iter = dom.begin(); iter != dom.end(); iter++) {
        cout << iter->first;
        for (int i = 0; i < iter->second.size(); i++) {
            cout << " " << iter->second[i];
        }
        cout << endl;
    }*/
    // step2: Calculate the immediate domin nodes of each block
    // step2.1: delete node itself;
    map<int, vector<int>>::iterator iter;
    for (iter = dom.begin(); iter != dom.end(); iter++) {
        //vector<int>::iterator it = iter->second.begin();
        iter->second.erase(std::remove(iter->second.begin(), iter->second.end(), iter->first), iter->second.end());
        //iter->second.erase(it);
    }
    /*for (iter = dom.begin(); iter != dom.end(); iter++) {
        cout << iter->first;
        for (int i = 0; i < iter->second.size(); i++) {
            cout << " " << iter->second[i];
        }
        cout << endl;
    }*/
    // step2.2: BFS
    vector<int> visit;
    std::queue<int> q;
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
    /*map<int, int>::iterator i;
    for (i = dom1.begin(); i != dom1.end(); i++) {
        cout << i->first << " " << i->second << endl;
    }*/
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
                    }
                    else {
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
    /*map<int, vector<int>>::iterator i;
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

map<int, BasicBlock*> generate_SSA(map<int, BasicBlock*> nodes,
    map<int, vector<mir::inst::VarId>> global,
    map<int, vector<int>> dom_f,
    vector<Instruction> instructions) {
    map<phi_index, vector< phi_info>> info;
    //step1: for every global var, builds the map named info
    map<int, vector<mir::inst::VarId>>::iterator it;
    map<mir::inst::VarId, int> note;
    for (it = global.begin(); it != global.end(); it++) {
        for (int j = 0; j < it->second.size(); j++) {
            note.insert(map<mir::inst::VarId, int>::value_type(it->second[j], 0));
        }
    }
    for (it = global.begin(); it != global.end(); it++) {
        for (int j = 0; j < it->second.size(); j++) {
            map<int, vector<int>>::iterator ite = dom_f.find(it->first);
            for (int i = 0; i < ite->second.size(); i++) {
                phi_index ind(ite->second[i], it->second[j]);
                phi_info inf;
                inf.m = ite->second[i];
                inf.name = it->second[j];
                inf.n = it->first;
                map<phi_index, vector< phi_info>>::iterator ii = info.find(ind);
                if (ii == info.end()) {
                    vector< phi_info> v;
                    v.push_back(inf);
                    info.insert(map<phi_index, vector< phi_info>>::value_type(ind, v));
                }
                else {
                    vector< phi_info> v = ii->second;
                    v.push_back(inf);
                    info[ind] = v;
                }
            }
        }
    }
    /*vector<phi_info> finfo;
    for (int i = 0; i < info.size(); i++) {
        bool has_same = false;
        for (int j = 0; j < info.size(); j++) {
            if (j != i && info[i].m == info[j].m && info[i].name == info[j].name) {
                has_same = true;
            }
        }
        if (has_same) {
            finfo.push_back(info[i]);
        }
    }
    map<phi_index, vector< phi_info>> phi;
    for (int i = 0; i < finfo.size(); i++) {
        //cout << finfo[i].m << " " << finfo[i].n << " " << finfo[i].name << endl;
        phi_index ind(finfo[i].m, finfo[i].name);
        map<phi_index, vector< phi_info>>::iterator ii = phi.find(ind);
        if (ii == phi.end()) {
            vector< phi_info> v;
            v.push_back(finfo[i]);
            phi.insert(map<phi_index, vector< phi_info>>::value_type(ind, v));
        }
        else {
            vector< phi_info> v = ii->second;
            v.push_back(finfo[i]);
            phi[ind] = v;
        }
    }*/
    map<phi_index, vector< phi_info>>::iterator ii;
    vector < phi_index> del;
    for (ii = info.begin(); ii != info.end(); ii++) {
        if (ii->second.size() < 2) {
            del.push_back(ii->first);
        }
    }
    for (int i = 0; i < del.size(); i++) {
        ii = info.find(del[i]);
        info.erase(ii);
    }
    /*for (ii = info.begin(); ii != info.end(); ii++) {
        cout << ii->first.m << " " << ii->first.name;
        for (int i = 0; i < ii->second.size(); i++) {
            cout << " " << ii->second[i].n;
        }
        cout << endl;
    }*/
    //step2: insert phi
    for (ii = info.begin(); ii != info.end(); ii++) {
        map<int, BasicBlock*>::iterator iter = nodes.find(ii->first.m);
        iter->second->inst.insert(iter->second->inst.begin(), ir_phi(ii->first.name, ii->second.size()));
    }
    //step3: number the vars
    //step3.1: find all vars
    vector<mir::inst::VarId> vars;
    for (int i = 0; i < instructions.size(); i++) {
        if (instructions[i].index() == 0) {
            shared_ptr<mir::inst::Inst> ins = get<0>(instructions[i]);
            if (ins->inst_kind() == mir::inst::InstKind::Assign) {
                shared_ptr<mir::inst::AssignInst> in =
                    std::static_pointer_cast<mir::inst::AssignInst>(ins);
                if (in->src.index() == 1) {
                    if (!has_this(vars, get<1>(in->src))) {
                        vars.push_back(get<1>(in->src));
                    }
                }
                if (!has_this(vars, in->dest)) {
                    vars.push_back(in->dest);
                }
            }
            else if (ins->inst_kind() == mir::inst::InstKind::Op) {
                shared_ptr<mir::inst::OpInst> in =
                    std::static_pointer_cast<mir::inst::OpInst>(ins);
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
            }
            else if (ins->inst_kind() == mir::inst::InstKind::Load) {
                shared_ptr<mir::inst::LoadInst> in =
                    std::static_pointer_cast<mir::inst::LoadInst>(ins);
                if (in->src.index() == 1) {
                    if (!has_this(vars, get<1>(in->src))) {
                        vars.push_back(get<1>(in->src));
                    }
                }
                if (!has_this(vars, in->dest)) {
                    vars.push_back(in->dest);
                }
            }
            else if (ins->inst_kind() == mir::inst::InstKind::Store) {
                shared_ptr<mir::inst::StoreInst> in =
                    std::static_pointer_cast<mir::inst::StoreInst>(ins);
                if (in->val.index() == 1) {
                    if (!has_this(vars, get<1>(in->val))) {
                        vars.push_back(get<1>(in->val));
                    }
                }
                if (!has_this(vars, in->dest)) {
                    vars.push_back(in->dest);
                }
            }
            else if (ins->inst_kind() == mir::inst::InstKind::PtrOffset) {
                shared_ptr<mir::inst::PtrOffsetInst> in =
                    std::static_pointer_cast<mir::inst::PtrOffsetInst>(ins);
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
            }
            else if (ins->inst_kind() == mir::inst::InstKind::Ref) {
                shared_ptr<mir::inst::RefInst> in =
                    std::static_pointer_cast<mir::inst::RefInst>(ins);
                if (in->val.index() == 1) {
                    if (!has_this(vars, get<1>(in->val))) {
                        vars.push_back(get<1>(in->val));
                    }
                }
                if (!has_this(vars, in->dest)) {
                    vars.push_back(in->dest);
                }
            }
            else {
                // nothing
            }
        }
    }
    //step3.2: rename
    //for (int i = 0; i < vars.size(); i++) {

    //}
    return nodes;
}