#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

void kernal(int *clauses, int *out, int var_cnt,
            int clause_cnt);

enum Status {
  Solved,
  Unsolvable,
};

class SATInstance {
 public:
  int var_cnt = 0, clause_cnt = 0;
  // -1 (unassigned), 0 (false), 1 (true)
  vector<int> vars = {};
  vector<int> clauses = {};

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

static int mod(int x) {
  return x < 0? -x : x;
}

void SATInstance::read(string infile) {
  ifstream fin(infile);
  if (!fin.is_open()) {
    cerr << "Error: couldn't open file " << infile << endl;
    exit(0);
  }
  char c;  // check if line is comment
  string s;
  while (true) {
    fin >> c;
    if (c == 'c')
      getline(fin, s);
    else
      break;
  }
  fin >> s;
  if (s != "cnf") {
    cerr << "Error: expected cnf input file, given " << s << endl;
    exit(1);
  }
  fin >> var_cnt >> clause_cnt;
  vars.clear();
  vars.resize(var_cnt + 1);
  clauses.clear();
  int var;
  for (int i = 0; i < clause_cnt; i++)
    for (fin >> var; var != 0; fin >> var) clauses.push_back(var);
}

Status SATInstance::solve() {
  for (int i = 1; i <= var_cnt; i++) vars[i] = -1;
  return backtrack();
}

Status SATInstance::backtrack() {
  vector<int> curr = vars;
  kernal(clauses.data(), vars.data(), var_cnt, clause_cnt);
  if (vars[0]) {
    // Current (partial) assignment causes conflict, undo implications and
    // backtrack
    vars = curr;
    return Unsolvable;
  }
  int var = selectVar();
  if (var == var_cnt + 1)
    return Solved;  // All variables are assigned with no conflict, we are done
  // Try to recurse by assigning current var false
  vars[var] = 0;
  Status s = backtrack();
  if (s == Solved)
    return Solved;  // Yay! False for current var worked!
  else {
    // False didn't work, try if true works
    vars[var] = 1;
    s = backtrack();
    if (s == Solved)
      return Solved;  // Yay! True for current var worked!
    else {
      // Both didn't work, backtrack by leaving current var unassigned
      vars = curr;
      return Unsolvable;
    }
  }
}

// Select next variable to try, insert any heuristics if desired
int SATInstance::selectVar() {
  for (int i = 1; i <= var_cnt; i++)
    if (vars[i] == -1) return i;
  return var_cnt + 1;
}

void SATInstance::printSol() {
  cout << "s SATISFIABLE" << endl;
  cout << "v ";
  for (int i = 1; i <= var_cnt; i++) cout << (vars[i] ? i : -i) << " ";
  cout << endl;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    cerr << "Error: incorrect usage. Expected: ./a.out filename.cnf" << endl;
    exit(0);
  }

  SATInstance s;
  s.read(argv[1]);
  if (s.solve() == Solved)
    s.printSol();
  else
    cout << "UNSATISFIABLE" << endl;
  return 0;
}
