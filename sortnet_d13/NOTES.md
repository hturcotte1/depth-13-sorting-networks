# NOTES.md — running log

Machine: 4 cores (x86_64), 15 GB RAM, no swap, ~30 GB free disk. Ubuntu 24.04, gcc 13.3, clang, Python 3.11.
Session start: 2026-09-20 ~15:54 UTC.

## Phase 0 — status check
- Fetched https://bertdobbelaere.github.io/sorting_networks.html (saved to data/dobbelaere_sorting_networks.html).
  Page footer: "Page updated on Fri Nov 7 22:10:36 2025". Latest History entry: 2025-11-07 (Wang25, 27/28 inputs depth 13).
  Table rows for n=29..32 unchanged from the brief: 29:(164,15),(166,14); 30:(172,14); 31:(180,14); 32:(185,14). Depth bounds 10..14.
- Parsed all 54 networks on the page into data/dobbelaere_networks.json (sizes/depths cross-checked against anchor names).
- Background literature sweep (arXiv / Scholar / GitHub / wcgbg repos) launched as a workflow; result recorded below when done.

## Environment
- Installed: python-sat 1.9.dev15, numpy 2.4.6, minisat (apt 2.2.1), kissat 4.0.4 (built from git), cadical 3.0.1 (built from git), bazelisk.
- Cloned https://github.com/wcgbg/sorting-network-n28d13 to /home/user/sorting-network-n28d13 and read the paper (paper/main.tex) and all sources.
- Bazel build of Wang's code FAILS: the egress proxy returns 403 for github.com/*/archive/* and codeload.github.com
  (policy block), so Bazel cannot fetch glog/gflags/protobuf/boost. Workaround: build with a plain Makefile against
  apt packages (libgoogle-glog-dev, libgflags-dev, libprotobuf-dev, libboost-iostreams-dev).

## Verifiers
- src/verify_c.c: bit-sliced exhaustive (64 inputs/word, OpenMP). Timings: n=28 Wang network 0.1 s wall; n=30 (172/14) 0.4 s; n=32 (185/14) 1.9 s.
  Rejects Wang's network with the last comparator (23,24) removed (3199 failing inputs).
- src/verify_py.py: independent Python implementation, modes exhaustive (numpy chunks), outputset (layer-by-layer set of ints), pyset (pure Python).

## Observations on Wang's code (relevant to reuse)
- OutputType is uint32_t: n=32 needs uint64_t (ReflectAndInvert CHECKs n<32). n=30 fine.
- NetworkOutputs() for a network without cached outputs uses a full 2^n bitset plus a MaskLibrary of ~n^2/2 bitsets of 2^n bits:
  for n=28 that is ~15 GB, for n=30 ~55 GB. decode_solution_main calls it on the full 28-channel network -> will not fit in 15 GB here.
  Our own tools compute output sets sparsely (product of the first layer, then shrink) instead.
- SAT encoding (sat_generate_cnf_main.cc): variables g[k][i][j] with mirror identification; used[k][i]; one_down/one_up;
  last layer only (i,i+1); second-to-last span<=3 with implications; no adjacent unused channels in last layer;
  per-input window encoding (leading 0s / trailing 1s fixed to constants).

## Phase 0 result (2026-09-20 16:15 UTC) — all four targets OPEN
Workflow of 4 independent sweeps + 1 skeptical verifier (5 agents, 196 tool calls). Findings, all consistent:
- Dobbelaere main + extended pages: last update 2025-11-07; rows n=29..32 unchanged (depth 14, bounds 10..14); no anchor N(29|30|31|32)L*D13.
  Served page byte-identical to repo HEAD; the only later site commit (2026-02-22) touched median-selection networks only.
- SorterHunter master Networks/Sorters: for n=29..32 only Sort_29_164_15, Sort_29_166_14, Sort_30_172_14, Sort_30_172_15, Sort_30_173_14, Sort_31_180_14, Sort_32_185_14 (+ variants); no depth-13 files.
- arXiv API / search: only 2511.04107 (v1 2025-11-06, v2 2025-11-22, no v3) claims new depth bounds since Nov 2025, and only for 27/28; its own Table 1 lists 29..32 at UB 14.
- Citations of 2511.04107: 0 (Semantic Scholar, OpenAlex, Google Scholar). wcgbg has no newer sorting-network repo (last update 2025-12-05).
- GitHub code/repo searches for n29d13/n30d13/n31d13/n32d13 and "13 layers" + 29..32 channels: nothing.
Conclusion: depth 13 is OPEN for n=29,30,31,32 as of 2026-09-20. Proceed with all four.

## Seed prefix output-set sizes (src/make_seed_prefixes.py, computed with snt info)
| prefix | n | depth | |out| |
|---|---|---|---|
| 16-ch Van Voorhis first 5 layers (4-cube + weight-matched 5th layer) | 16 | 5 | 83 |
| 16-ch Green (61/9) first 5 layers | 16 | 5 | 110 |
| 16-ch 60/10 first 5 layers | 16 | 5 | 83 |
| 4-cube | 16 | 4 | 168 (= Dedekind M(4)) |
| 14-ch 52/9 first 5 layers | 14 | 5 | 109 |
| 15-ch 57/9 first 5 layers (not symmetric) | 15 | 5 | 97 |
| Wang's 28-ch network first 5 / 6 layers | 28 | 5 / 6 | 2905 / 928 (2905 = 83 x 35) |
| 5-cube minus channels 0,1,30,31 | 28 | 5 | 7246 |
| 32-ch 185/14 first 5 layers | 32 | 5 | 6887 |
| 5-cube | 32 | 5 | 7581 (= Dedekind M(5)) |
| VV16 + VV16 nested or mirrored | 32 | 5 | 6889 (= 83^2) |
| Green16 + Green16 side by side | 32 | 5 | 12100 |
| VV16+VV16 nested minus channels 0,31 | 30 | 5 | 6723 |
| 30-ch 172/14 first 5 layers | 30 | 5 | 7413 |
| 5-cube minus channels 0,31 | 30 | 5 | 7579 |
Takeaway: for n=30 the nested 16+14 route needs a 14-channel 5-layer symmetric prefix with |out| < 81 to beat 6723;
for n=32 all natural 5-layer prefixes sit at ~6900-7600 (about 2.4x Wang's 28-channel starting point of 2905).

## Own pipeline (src/snt) — status 16:45 UTC
- Built: snt gen/stack/extend/cnf/decode/info/sizes (C++20, no deps), src/solve.py (parallel solver driver), src/nettools.py.
- Calibration vs Wang's generate-and-prune counts for n=12 symmetric: depth 2 -> 41 (Wang 41), depth 3 -> 1502 (Wang 1502). Depth 4/5 running.
- Bug found and fixed: decoding a suffix found under a channel permutation must untangle reversed comparators
  (Knuth 5.3.4 ex. 16; Wang's PermuteInputChannels does this). Before the fix the decoded 10-channel networks did not sort;
  after it, 3/3 decoded n=10 depth-7 networks pass both verifiers, and depth 6 is UNSAT for the same prefixes (tests/test_pipeline.py).
