#include <fstream>
#include <vector>
#include <iostream>

using namespace std;

const int NAMELEN = 4;
const char *KERNAL_NAME = "kernal.cpp";

class Builder {
  public:
    int var_cnt = 0, clause_cnt = 0;
    // -1 (unassigned), 0 (false), 1 (true)
    vector<int> vars = {};
    vector<vector<int>> clauses = {};

    void read(string infile);
    void genC();
};

void Builder::read(string infile) {
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

static int mod(int x) {
  return x < 0? -x : x;
}

static string varn(int i) {
  return "v" + to_string(i);
}

void Builder::genC() {
  
  ofstream fout(KERNAL_NAME);
  if(!fout.is_open()) {
    cerr << "Error: couldn't open kernal file" << endl;
    exit(0);
  }

  for(int i = 1; i <= var_cnt; i++) {
    fout << "bool circuit_" << varn(i) << "(";
    bool isFirst = true;
    for(int j = 1; j <= var_cnt; j++) {
      if(i != j) {
        if(!isFirst) fout << ", ";
        else isFirst = false;
        fout << "bool " << varn(j);
      }
    }
    fout << ") {\n\treturn ";
    for(auto clause : clauses) {
      bool clauseHasVari = false;
      for(auto var : clause)
        if(mod(var) == i) {
          clauseHasVari = true;
          break;
        }
      if(!clauseHasVari) continue;
      isFirst = true;
      for(int j = 0; j < clause.size(); j++) {
        int var = clause[j];
        if(mod(var) != i) {
          if(!isFirst) fout << " && ";
          else {
            fout << "(";
            isFirst = false;
          }
          if(var < 0) fout << varn(-var);
          else fout << "!" << varn(var);
        }
        if(j && j % 10 == 0) fout << "\n\t\t";
      }
      if(!isFirst) fout << ") || ";
    }
    fout << "false;\n}\n";
  }
}

int main(int argc, char *argv[]) {
  if(argc != 2) {
    cerr << "Error: incorrect usage. Expected: ./a.out filename.cnf" << endl;
    exit(0);
  }
  Builder build;
  build.read(argv[1]);
  build.genC();
}
