// in -> read only
// clauses -> read only
// out -> read/write
// var_cnt -> read_only
// clause_cnt -> read_only
// conflict -> write only
void kernal(int *in, int *clauses, int *out, bool *conflict, int var_cnt,
            int clause_cnt) {
  // Make out same as in.
  for (int i = 0; i <= var_cnt; ++i) out[i] = in[i];

  // Resolve implications.
  for (unsigned i = 0; i < clause_cnt; ++i) {
    // Get variables.
    int var[3], v[3], sign[3];
    for (unsigned j = 0; j < 3; ++j) {
#pragma HLS UNROLL
      var[j] = clauses[3 * i + j];
      sign[j] = var[j] >= 0;
      if (!sign[j])
        v[j] = in[-var[j]];
      else
        v[j] = in[var[j]];
    }

    // If atleast 2 unassigned variables, skip this clause.
    if ((v[0] == -1 && v[1] == -1) || (v[1] == -1 && v[2] == -1) ||
        (v[2] == -1 && v[0] == -1))
      continue;

    // If a single unassigned variable, check for implications.
    // Im hoping the compiler will resolve mods after unroll (it should
    // otherwise Xilinx needs to reconsider a lot of things).
    for (unsigned j = 0; j < 3; ++j) {
#pragma HLS UNROLL
      if (v[j] != -1) continue;
      int other1 = v[(j + 1) % 3] ^ !sign[(j + 1) % 3];
      int other2 = v[(j + 2) % 3] ^ !sign[(j + 2) % 3];

      // Implication yay.
      if (!other1 && !other2) out[j] = 1 ^ !sign[j];
    }
  }

  // Check for conflicts.
  for (unsigned i = 0; i < clause_cnt; ++i) {
    // Get variables.
    int var[3], v[3], sign[3];
    for (unsigned j = 0; j < 3; ++j) {
#pragma HLS UNROLL
      var[j] = clauses[3 * i + j];
      sign[j] = var[j] >= 0;
      if (!sign[j])
        v[j] = in[-var[j]];
      else
        v[j] = in[var[j]];
    }

    // If any unassigned, move on.
    if (v[0] == -1 || v[1] == -1 || v[2] == -1) continue;

    // Get value variables at this clause.
    for (unsigned j = 0; j < 3; ++j)
#pragma HLS UNROLL
      if (!sign[j])
        v[j] = !v[j];

    // Check conflict.
    if (!(v[0] || v[1] || v[2])) {
      *conflict = true;
      return;
    }
  }
}
