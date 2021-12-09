#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <unistd.h>

#include <CL/cl2.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

std::vector<cl::Device> get_xilinx_devices() {
  size_t i;
  cl_int err;
  std::vector<cl::Platform> platforms;
  err = cl::Platform::get(&platforms);
  cl::Platform platform;
  for (i = 0; i < platforms.size(); i++) {
    platform = platforms[i];
    std::string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err);
    if (platformName == "Xilinx") {
      std::cout << "INFO: Found Xilinx Platform" << std::endl;
      break;
    }
  }
  if (i == platforms.size()) {
    std::cout << "ERROR: Failed to find Xilinx platform" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Getting ACCELERATOR Devices and selecting 1st such device
  std::vector<cl::Device> devices;
  err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
  return devices;
}

char *read_binary_file(const std::string &xclbin_file_name, unsigned &nb) {
  if (access(xclbin_file_name.c_str(), R_OK) != 0) {
    printf("ERROR: %s xclbin not available please build\n",
           xclbin_file_name.c_str());
    exit(EXIT_FAILURE);
  }
  // Loading XCL Bin into char buffer
  std::cout << "INFO: Loading '" << xclbin_file_name << "'\n";
  std::ifstream bin_file(xclbin_file_name.c_str(), std::ifstream::binary);
  bin_file.seekg(0, bin_file.end);
  nb = bin_file.tellg();
  bin_file.seekg(0, bin_file.beg);
  char *buf = new char[nb];
  bin_file.read(buf, nb);
  return buf;
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

  int *in, *out;
  cl::Kernel *krnl;
  cl::Buffer *in_buf, *out_buf;
  cl::CommandQueue *q;

  void runKrnl();
  void runStuff();

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

static int mod(int x) { return x < 0 ? -x : x; }

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
  clauses.resize(clause_cnt);
  int var;
  for (int i = 0; i < clause_cnt; i++)
    for (fin >> var; var != 0; fin >> var) clauses[i].push_back(var);
}

Status SATInstance::solve() {
  for (int i = 1; i <= var_cnt; i++) vars[i] = -1;
  return backtrack();
}

Status SATInstance::backtrack() {
  runStuff();
  vector<int> implied = resolveImplications();
  if (conflictExists()) {
    // Current (partial) assignment causes conflict, undo implications and
    // backtrack
    for (auto var : implied) vars[var] = -1;
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
      vars[var] = -1;
      for (auto var : implied) vars[var] = -1;
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

vector<int> SATInstance::resolveImplications() {
  vector<int> implied;
  for (int impliedVar = getImpliedVar(); impliedVar != 0;
       impliedVar = getImpliedVar()) {
    vars[mod(impliedVar)] = impliedVar < 0 ? 0 : 1;
    implied.push_back(mod(impliedVar));
  }
  return implied;
}

int SATInstance::getImpliedVar() {
  for (auto clause : clauses) {
    int unassigned_cnt = 0, unassigned_i;
    bool clause_val = false;
    for (auto i : clause) {
      if (vars[mod(i)] == -1) {
        unassigned_cnt++;
        unassigned_i = i;
      } else
        clause_val |= (i < 0 ? !vars[-i] : vars[i]);
    }
    // Exactly one unassigned var, and rest of the clause evals to false =>
    // found implied var
    if (unassigned_cnt == 1 && !clause_val) return unassigned_i;
  }
  return 0;
}

bool SATInstance::conflictExists() {
  for (auto clause : clauses) {
    bool clause_val = false;
    for (auto var : clause) {
      if (vars[mod(var)] == -1)
        clause_val = true;  // No point in evaluating this clause further
      else
        clause_val |= var < 0 ? !vars[-var] : vars[var];
      if (clause_val) break;
    }
    if (!clause_val) return true;
  }
  return false;
}

void SATInstance::printSol() {
  cout << "s SATISFIABLE" << endl;
  cout << "v ";
  for (int i = 1; i <= var_cnt; i++) cout << (vars[i] ? i : -i) << " ";
  cout << endl;
}

void SATInstance::printClauses() {
  for (auto clause : clauses) {
    for (auto var : clause) cout << var << " ";
    cout << endl;
  }
}

void SATInstance::runKrnl() {
  // Schedule transfer of inputs to device memory, execution of kernel, and
  // transfer of outputs back to host memory
  q->enqueueMigrateMemObjects({*in_buf}, 0 /* 0 means from host*/);
  q->enqueueTask(*krnl);
  q->enqueueMigrateMemObjects({*out_buf}, CL_MIGRATE_MEM_OBJECT_HOST);

  // Wait for all scheduled operations to finish
  q->finish();
}

void SATInstance::runStuff() {
  for (unsigned i = 0, e = vars.size(); i < e; ++i) in[i] = vars[i];
  for (unsigned i = 0, e = vars.size(); i < e; ++i) cout << in[i] << " ";
  cout << "\n";
  runKrnl();
  for (unsigned i = 0, e = vars.size(); i < e; ++i) cout << out[i] << " ";
  cout << "\n";
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << "Error: incorrect usage. Expected: ./a.out kernal_file kernal_name "
            "filename.cnf"
         << endl;
    exit(0);
  }

  SATInstance s;
  s.read(argv[3]);
  cerr << "Loaded SAT\n";

  // ------------------------------------------------------------------------------------
  // Step 1: Initialize the OpenCL environment
  // ------------------------------------------------------------------------------------
  cl_int err;
  std::string binaryFile = argv[1];
  unsigned fileBufSize;
  std::vector<cl::Device> devices = get_xilinx_devices();
  devices.resize(1);
  cl::Device device = devices[0];
  cl::Context context(device, NULL, NULL, NULL, &err);
  char *fileBuf = read_binary_file(binaryFile, fileBufSize);
  cl::Program::Binaries bins{{fileBuf, fileBufSize}};
  cl::Program program(context, devices, bins, NULL, &err);
  cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
  cl::Kernel krnl(program, argv[2], &err);

  // ------------------------------------------------------------------------------------
  // Step 2: Create buffers and initialize test values
  // ------------------------------------------------------------------------------------
  // Create the buffers and allocate memory
  cl::Buffer in_buf(context, CL_MEM_READ_ONLY, sizeof(int) * s.vars.size(),
                    NULL, &err);
  cl::Buffer out_buf(context, CL_MEM_WRITE_ONLY, sizeof(int) * s.vars.size(),
                     NULL, &err);

  // Map buffers to kernel arguments, thereby assigning them to specific device
  // memory banks
  krnl.setArg(0, in_buf);
  krnl.setArg(1, out_buf);

  // Map host-side buffer memory to user-space pointers
  int *in = (int *)q.enqueueMapBuffer(in_buf, CL_TRUE, CL_MAP_READ, 0,
                                      sizeof(int) * s.vars.size());
  int *out = (int *)q.enqueueMapBuffer(out_buf, CL_TRUE, CL_MAP_WRITE, 0,
                                       sizeof(int) * s.vars.size());

  // ------------------------------------------------------------------------------------
  // Step 3: Run the kernel
  // ------------------------------------------------------------------------------------
  // Set kernel arguments
  krnl.setArg(0, in_buf);
  krnl.setArg(1, out_buf);
  krnl.setArg(2, s.var_cnt);

  s.in = in;
  s.out = out;
  s.krnl = &krnl;
  s.q = &q;
  s.in_buf = &in_buf;
  s.out_buf = &out_buf;

  cerr << "solving now\n";

  if (s.solve() == Solved)
    s.printSol();
  else
    cout << "UNSATISFIABLE" << endl;
  return 0;
}
