# src/ — tools

Everything here is self-contained (C++20 + Python 3 with numpy; SAT solvers are external binaries).

## Verifiers (independent implementations)
- `verify_c.c` — bit-sliced exhaustive check of all 2^n binary inputs, 64 inputs per word, OpenMP.
  `gcc -O3 -march=native -fopenmp -o verify_c verify_c.c && ./verify_c network.txt`
- `verify_py.py` — Python: `--mode exhaustive` (numpy chunks over all 2^n inputs, default n<=26),
  `--mode outputset` (layer-by-layer reachable-set of integers, default n>26), `--mode pyset` (pure Python, n<=20), `--mode all`.

Both read Dobbelaere's format (one layer per line, `[(i,j),(k,l),...]`, `#` comments), report size and depth,
and exit 0 iff the network sorts.

## Search pipeline (`snt/`, build with `make -C snt`)
Reimplementation of Wang 2025 (arXiv:2511.04107) with 64-bit output vectors (n up to 62), no external dependencies.
- `snt gen`    generate-and-prune of reflection-symmetric (or plain) prefixes, layer by layer, with the symmetric
               subsumption test (Bundala–Závodný lemma with permutation and reflect-and-complement), optional keep-best-K per depth.
- `snt stack`  stack two prefix libraries: `--mode nested` (outer/inner symmetric blocks) or `--mode mirrored` (block A and its reflection).
- `snt extend` Wang's greedy layer extension: one comparator (plus mirror) at a time, prune, keep best K by |output set|.
- `snt cnf`    SAT encoding of the remaining layers (CCEMS 2019 improvements as implemented by Wang: oneUp/oneDown,
               last layer span 1, second-to-last span <= 3 with implications, no adjacent idle channels in the last layer,
               per-input window encoding after a channel permutation minimising window sizes). Writes DIMACS with
               provenance comments (prefix, permutation).
- `snt decode` reads CNF + solver output, rebuilds the suffix, un-permutes it with Knuth's untangling, appends to the prefix,
               checks the output set, strips comparators that never act, re-layers ASAP, prints the network.
- `snt info` / `snt sizes`  output-set sizes per layer / per prefix.
- `solve.py`   parallel solver driver (minisat / cadical / kissat) with per-instance timeouts and JSON results; `--decode` decodes SAT instances.
- `run_search.sh NAME LIB DEPTH GREEDY_LAYERS KEEP SOLVER TIMEOUT LIMIT [JOBS]` — one logged, reproducible run.
- `nettools.py` — independent Python utilities: `info`, `simplify` (strip + re-layer), `delete --channel k`, `json`, `oneline`.
- `make_seed_prefixes.py` — hypercube / Van Voorhis / stacked / channel-deleted seed prefixes.

Library file format: `# n=N` header, then one network per line, layers comma-separated: `[(0,1),(2,3)],[(0,2),(1,3)]`.
