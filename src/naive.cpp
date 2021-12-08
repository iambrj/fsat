#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>

using namespace std;

int mod(int x) {
  return x < 0? -x : x;
}

enum Status {
  Solved,
  Unsolvable,
};

class SATInstance {
  public:
    int var_cnt = 0, clause_cnt = 0;
    // -1 (unassigned), 0 (false), 1 (true)
    vector<int> vars = {};
    vector<vector<int>> clauses = {};

    void read(string infile);
    Status solve();
    Status backtrack();
    vector<int> resolveImplications();
    int getImpliedVar();
    bool conflictExists();
    int selectVar();
    void printSol();
    void printClauses();
};

void SATInstance::read(string infile) {
  ifstream fin(infile);
  if(!fin.is_open()) {
    cerr << "Error: couldn't open file " << infile << endl;
    exit(0);
  }
  char c; // check if line is comment
  string s;
  while(true) {
    fin >> c;
    if(c == 'c') getline(fin, s);
    else break;
  }
  fin >> s;
  if(s != "cnf") {
    cerr << "Error: expected cnf input file, given " << s << endl;
    exit(1);
  }
  fin >> var_cnt >> clause_cnt;
  vars.clear();
  vars.resize(var_cnt + 1);
  clauses.clear();
  clauses.resize(clause_cnt);
  int var;
  for(int i = 0; i < clause_cnt; i++)
    for(fin >> var; var != 0; fin >> var)
      clauses[i].push_back(var);
}

Status SATInstance::solve() {
  for(int i = 1; i <= var_cnt; i++) vars[i] = -1;
  return backtrack();
}

Status SATInstance::backtrack() {
  vector<int> implied = resolveImplications();
  if(conflictExists()) {
    // Current (partial) assignment causes conflict, undo implications and
    // backtrack
    for(auto var : implied) vars[var] = -1;
    return Unsolvable;
  }
  int var = selectVar();
  if(var == var_cnt + 1) return Solved; // All variables are assigned with no conflict, we are done
  // Try to recurse by assigning current var false
  vars[var] = 0;
  Status s = backtrack();
  if(s == Solved) return Solved; // Yay! False for current var worked!
  else {
    // False didn't work, try if true works
    vars[var] = 1;
    s = backtrack();
    if(s == Solved) return Solved; // Yay! True for current var worked!
    else {
      // Both didn't work, backtrack by leaving current var unassigned
      vars[var] = -1;
      for(auto var : implied) vars[var] = -1;
      return Unsolvable;
    }
  }
}

// Select next variable to try, insert any heuristics if desired
int SATInstance::selectVar() {
  for(int i = 1; i <= var_cnt; i++)
    if(vars[i] == -1) return i;
  return var_cnt + 1;
}

vector<int> SATInstance::resolveImplications() {
  vector<int> implied;
  for(int impliedVar = getImpliedVar(); impliedVar != 0; impliedVar = getImpliedVar()) {
    vars[mod(impliedVar)] = impliedVar < 0 ? 0 : 1;
    implied.push_back(mod(impliedVar));
  }
  return implied;
}

int SATInstance::getImpliedVar() {
  for(auto clause : clauses) {
    int unassigned_cnt = 0, unassigned_i;
    bool clause_val = false;
    for(auto i : clause) {
      if(vars[mod(i)] == -1) {
        unassigned_cnt++;
        unassigned_i = i;
      } else clause_val |= (i < 0 ? !vars[-i] : vars[i]);
    }
    // Exactly one unassigned var, and rest of the clause evals to false =>
    // found implied var
    if(unassigned_cnt == 1 && !clause_val) return unassigned_i;
  }
  return 0;
}

bool SATInstance::conflictExists() {
  for(auto clause : clauses) {
    bool clause_val = false;
    for(auto var : clause) {
      if(vars[mod(var)] == -1) clause_val = true; // No point in evaluating this clause further
      else clause_val |= var < 0 ? !vars[-var] : vars[var];
      if(clause_val) break;
    }
    if(!clause_val) return true;
  }
  return false;
}

void SATInstance::printSol() {
  cout << "s SATISFIABLE" << endl;
  cout << "v ";
  for(int i = 1; i <= var_cnt; i++)
    cout << (vars[i] ? i : -i) << " ";
  cout << endl;
}

void SATInstance::printClauses() {
  for(auto clause : clauses) {
    for(auto var : clause) cout << var << " ";
    cout << endl;
  }
}

int main(int argc, char* argv[]) {
  if(argc != 2) {
    cerr << "Error: incorrect usage. Expected: ./a.out filename.cnf" << endl;
    exit(0);
  }
  SATInstance s;
  s.read(argv[1]);
  if(s.solve() == Solved)
    s.printSol();
  else cout << "UNSATISFIABLE" << endl;
  return 0;
}
