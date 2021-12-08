# fsat

Experiments in accelerating a SAT solver using FPGAs

# Solvers implemented

- NOTE: all solvers expected DIMACS CNF format for inputs.

## naive

- Only unit propagation, no other heuristics used.
- Literals tried in increasing order.
- Accelerate by offloading unit propagation and conflict checking to the FPGA.

## dpll

- Also perform pure literal elimination.

## dpll_f

- dpll + choose literals using the highest frequency first heuristic.

# Tests

- ./tests/ has handwritten tests useful for debugging, not useful for
  benchmarking.
- Tests from https://www.cs.ubc.ca/~hoos/SATLIB/benchm.html to be used for
  benchmarking.
