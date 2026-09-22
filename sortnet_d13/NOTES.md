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

## Phase 1 — Wang n=28 d=13 reproduced end to end (VERIFIED) — finished 16:42 UTC
Built Wang's tools from source with a plain Makefile (Makefile.local in the clone) against apt libraries, with one local patch
(NetworkOutputs computed sparsely; see above). Pipeline exactly as in his README (4 cores, contended part of the time by my own jobs):
| step | wall time |
|---|---|
| add_layers_main n=12 sym depth 5 keep ,,,4 | 13m47s (Wang: 7 min on M2) |
| add_layers_main n=16 sym depth 5 keep 1,1,1,1 | 1m25s (Wang: 1 min) |
| stack_main 12+16 | <1 s |
| add_comparators_main keep 64 (6th layer) | 11 s |
| optimize_window_size_main | 2 s |
| sat_generate_cnf_main depth 13 limit 8 | 11 s |
| sat_solve_main.py minisat (8 instances, 4 parallel) | 20m47s; per-instance 26.7, 39.0, 44.2, 45.8, 197.1, 549.8, 906.9 s + one more; all 8 SAT |
| decode_solution_main --simplify | 53 s |
Total ~37 min. Output: 8 networks, 28 channels, 13 layers, sizes 165,167,167,168,170,170,173,173 (Wang's published 159 is a further-reduced one).
All 8 pass src/verify_c (exhaustive 2^28) and src/verify_py --mode outputset. Files: runs/phase1_wang/.
Calibration conclusion: the encoding/decoding chain and both verifiers agree with Wang's published result.

## Memory limit
All tool processes share one cgroup with memory.limit_in_bytes = 14,345,912,320 (14.3 GB). At 16:41 the sum of Wang's job
(~9 GB during n=12 depth-4 pruning) + my calibration (4.7 GB) + n=16 greedy hit it and the kernel killed my two snt processes.
Mitigation implemented in src/snt: outputs moved (not copied) into the pruner, survivors compacted after every pass and their
output sets recomputed, per-worker candidate buffers flushed with a fast prune when they exceed a memory budget.

## 16-channel prefix (own tool) — 16:52 UTC
`snt gen --n 16 --sym --depth 5 --keep 1,1,1,1` (1 thread, 141 s): depth 2/3/4/5 best |out| = 1296/400/168/83, two depth-5 prefixes
with |out|=83 (the first is Van Voorhis's 4-cube + weight-matched 5th layer, identical to data/prefixes/n16_vv5.txt), matching Wang's
"best 2 prefixes for 16 channels". Saved: runs/n16/n16d5_greedy1.txt.
Dobbelaere's 28/155/14, 30/172/14, 32/185/14 networks are reflection-symmetric; 29/166/14 is not.

## Solver comparison on Wang's n=28 CNFs (runs/solver_calib) — 17:02 UTC
Instances 0001 and 0003 from the Phase 1 run (50 MB each, ~1000 inputs, 7 layers). Machine oversubscribed (2 generators + 4 solvers on 4 cores).
| solver | 0003 | 0001 |
|---|---|---|
| minisat 2.2 (Phase 1 run, 4 parallel) | 26.7 s | 39.0 s |
| kissat 4.0.4 | 528 s | 541 s |
| cadical 3.0.1 | 813 s | 1184 s |
MiniSat is >10x faster on these instances (consistent with Wang's choice). Plan: MiniSat primary, Kissat secondary on stragglers.

## First n=30 experiment: seed prefix VV16+VV16 nested minus channels {0,31} — 17:30 UTC
- 5 layers, |out| = 6723. Greedy 6th layer (snt extend, keep 64, 1 thread, 10m23s): rounds 0..6 add 7 mirrored comparator pairs,
  best |out| 5146 -> 3968 -> 3136 -> 2499 -> 2193 -> 1935 -> 1699; rounds 7..14 add nothing (layer saturated). 65 prefixes kept,
  |out| range 1699..2230. (Wang's 28-channel 6-layer prefix: ~900.) Saved runs/n30_seed/vv_del_L6.txt.
- n=12 calibration (exact generate-and-prune to depth 5) stopped after 35 min at depth 4 to free CPU; depth-2/3 counts matched Wang
  exactly (41, 1502). Depth 4/5 counts: INCONCLUSIVE (not needed for the search; can be rerun when the machine is idle).
- Launching run A: seed 6-layer prefixes, best 8, depth 13 (7 SAT layers), minisat, 1800 s each, 4 jobs.

## Run B result — NEGATIVE (17:40 UTC)
runs/n30_B_seed_L7_minisat: 8 prefixes (7 layers, |out| 928,928,928,928,940,940,940,940), 6 SAT layers, minisat: all 8 UNSAT in 8-12 s
(CNFs ~65k vars, ~2.0M clauses). The greedy 7th layer over-commits; with these 7-layer prefixes no 13-layer completion exists.
Next: run A (6-layer prefixes, |out| 1699..1977, 7 SAT layers) and an encoder calibration on Wang's own 28-channel 6-layer prefixes.

## Encoder calibration at n=28 — VERIFIED (17:43 UTC)
runs/calib_wang_n28d6_minisat: my `snt cnf` (own window permutation, own encoding) on Wang's own 6-layer 28-channel prefixes
(extracted from his generated/n28d6.pb; |out| = 928 each; CNFs 73k vars / 2.25M clauses): prefix 0000 SAT with minisat in 102 s,
decoded (untangling 45 reversed comparators) to a reflection-symmetric 28-channel depth-13 sorting network (raw size 176).
So the whole own chain (cnf -> minisat -> decode) reproduces Wang's result independently of his code; negatives from this encoder
are meaningful. Note the coincidence: Wang's 6-layer 28-channel prefixes and our greedy 7-layer 30-channel prefixes both have |out| = 928,
yet the former are completable in 7 more layers and the latter are not in 6.
Calibration complete: 4/4 SAT (102, 433, 286, 348 s with 2 parallel jobs on a loaded machine); decoded networks n=28 depth 13,
sizes 170/166/173/163 after stripping+relayering; each passes verify_c and verify_py (runs/calib_wang_n28d6_minisat/log.txt).

## Run A result — INCONCLUSIVE (18:12 UTC)
runs/n30_A_seed_L6_minisat: 6-layer seed prefixes, 7 SAT layers (CNFs 148k-178k vars, 5.0-6.2M clauses). Instances 0000-0003
(|out| 1699, 1735, 1915, 1915): minisat TIMEOUT at 1800 s each (machine shared with 3 other jobs). Instances 0004-0007 not run
(killed to free CPU for the partial-layer experiments). Follow-up: longer budget for 0000/0001 later.
Partial 7th layer (greedy, stopped after 2/3/4 mirrored pairs): best |out| 1320 / 1186 / 1080 (runs/n30_seed/vv_del_L6p{2,3,4}.txt).
Run D (18:15): p3 prefixes (7th layer frozen with 3 pairs), 6 SAT layers, minisat 1800 s, 4 jobs.

## n=32 seed greedy extension (VV16+VV16 nested, |out| 6889) — 18:15 UTC
snt extend keep 64, 2 layers, 1 thread (31 min, loaded machine): layer 6 best |out| 5312 -> ... -> 1787 (7 pairs, saturated);
layer 7: 1581 -> ... -> 992 (7 pairs). 83 seven-layer prefixes with |out| 992..1026 (runs/n32_seed/vv_nested_L7.txt; six-layer set in .L1).
Same shape as n=30 (6723 -> 1699 -> 928).

## Run E result — NEGATIVE (18:18 UTC)
runs/n30_E_seed_L6p3_open_minisat: same p3 prefixes with the 7th layer OPEN (3 greedy pairs fixed, SAT may add comparators on the
other 24 channels of layer 7, then 6 layers; CNFs ~100k vars / 3.3M clauses): all 8 UNSAT in 15-24 s.
Conclusion for the seed family (VV16+VV16 nested minus {0,31}, greedy 6th layer): any prefix containing the first 3 greedy
layer-7 pairs is not completable; whether the 6-layer prefixes are completable at all is open (run A: 1800 s timeouts).

## 16+14 nested route (the faithful analog of Wang's 16+12) — 18:25 UTC
Quick 14-channel prefixes: `snt gen --n 14 --sym --depth 5 --keep 1,1,1,1` -> depth 2/3/4/5 best |out| 648/220/110/69 (1 prefix, 8 s).
Stacked with the two VV16 prefixes (83): 16-outer/14-inner and 14-outer/16-inner, |out| = 83*69 = 5727 each (4 stacked prefixes).
Greedy 6th layer (keep 64 across all 4) running -> run F.
- keep-4 14-channel prefixes: 8 with |out| 66,69,71,72x5 (9 s). Stacked with VV16 both nestings: 32 prefixes, |out| 5478..5976.
  Greedy 6th layer over the whole pool (keep 64, 7 min): best 4224 -> 2385 -> 1853 (saturated after 6 pairs); 66 prefixes, 1853..1973.
  The g1 stacks alone gave 1983. Run F (18:45): top 4 of the pool, 7 SAT layers, minisat 7200 s each, 4 jobs.

## Diagnostic: greedy 7th layer on Wang's completable 28-channel prefixes — 18:31 UTC
runs/calib_wang_L7: Wang's 8 six-layer prefixes (|out| 928, all SAT in 7 layers) + greedy 7th layer (keep 64; |out| -> 530..541),
then 6 SAT layers: 8/8 UNSAT in 4-5 s. Conclusion: a greedy min-|out| 7th layer generically destroys completability, so the fast
UNSATs of runs B/D/E are NOT evidence about the 6-layer 30-channel prefixes. Only "6 greedy layers + 7 SAT layers" (Wang's
configuration) is a fair test; for n=30 those instances have ~1700-1900 inputs (2x Wang's) and did not finish in 1800 s (run A).
Run F gives 7200 s to the 4 best 16+14 prefixes.

## Run F result — NEGATIVE (18:39 UTC)
runs/n30_F_16_14_g4_L6_minisat: the 4 best 16+14 six-layer prefixes (|out| 1853,1853,1857,1857; CNFs ~160k vars, 5.3M clauses),
7 SAT layers, minisat: 4/4 UNSAT in 501-520 s. So Wang's exact recipe (stack, greedy 6th layer by min |out|, SAT for the rest)
fails for these four 30-channel prefixes. Compare: run A's seed prefixes (1699..1915) did not resolve in 1800 s.
Next: run G = 8 SAT layers directly on the two best 5-layer stacks (|out| 5478; greedy 6th layer removed), 14400 s each, 2 jobs;
the n=14 exhaustive-ish library (keep ,2000,4000,24) resumed with 2 threads; n=15 generator kept paused.
Run G (01:36 UTC, 21 Sep): 2 instances, 602k vars / 21.8M clauses each, minisat 14400 s, 2 jobs (n=14 library resumed on the other 2 cores).
Session note: the harness paused between 18:40 and 01:30 UTC; the generators were SIGSTOPped during that time (no compute lost or gained).
Planned next if G is UNSAT: (a) other 14-channel prefixes from the exhaustive-ish library; (b) 15+15 mirrored; (c) sweep all 66
greedy-6 prefixes with 900 s each (variance play); (d) n=32 analog (VV16+VV16, |out| 6889 -> greedy 1787 -> 7 SAT layers).

## Diagnostic: block completability (02:11 UTC) — heuristic only, does NOT discriminate
Tested with the SAT encoder whether each 5-layer block prefix can be completed (reflection-symmetrically) to a depth-optimal
sorter of the block alone (16 channels: depth 9; 14: depth 9; 12: depth 8), all instances < 0.1 s:
- VV16 variant 0000 (4-cube + weight-matched layer): symmetric depth-9 completion UNSAT; non-symmetric completion SAT (Van Voorhis's own network).
  VV16 variant 0001 (the other 83-output prefix): symmetric completion SAT.
- 14-channel keep-4 prefixes (|out| 66,69,71,72,72,72,72,72) and keep-1 (69): SAT for 66, 69, 72, 72, 69; UNSAT for 71, 72, 72, 72.
- Wang's 8 successful 28-channel prefixes: all 8 outer 12-blocks symmetric-depth-8 completable; inner 16-blocks 4 UNSAT / 4 SAT
  (both VV variants occur among his completable prefixes). So block completability is not necessary for the stacked network, as expected
  (sorting and merging interleave), and cannot explain runs A/F.
- Run F's 4 prefixes all use the 16-outer nesting with VV variant 0001 (symmetric-completable) and the |out14|=66 prefix (completable).

## 22 Sep 22:50 UTC — after a container restart (all processes lost between ~05:00 21 Sep and now)
- Run G (8 SAT layers on the two 5-layer 16+14 stacks, 602k vars / 21.8M clauses): killed by the restart after ~3 h without a verdict -> INCONCLUSIVE.
- The research agent's own side experiments (Ehlers 24-ch prefix sym/non-sym, run-F prefix with window<=20/24) were also killed; no results.
- n=14 library: depth 4 finished (4389 prefixes, best |out| 110, 9.6 h wall incl. pauses); depth 5 restarted from the .d4 file with keep 24 (2 threads).
- Launched: control run (run F prefix 0, symmetric, WITHOUT the normal-form constraints psi1/psi3; `snt cnf --no_nf`) and run H (same prefix,
  non-symmetric suffix), 14400 s each, 1 core each.

## Research workflow (5 reports, ~1.5M tokens; synthesis agent failed on usage credits) — key actionable points
1. Encoding audit (report 0): we use CCEMS phi1-phi4 (necessary) + psi1, psi3a/b (normal forms) + oneUp/oneDown + window encoding + permutation.
   Not used: psi2a-c (Lemma 8 co-saturation of layer d-1), Theorem 2 (k-block adjacency) beyond the last two layers, sigma1-3 of arXiv:1412.5302,
   MiniSat probing/cla-decay 0.9999. CAVEAT: psi normal forms are proven for unrestricted networks; soundness under the reflection-symmetry
   restriction is argued, not proven -> control run without psi launched (above). Our instances (5-6M clauses) are 2-4x CCEMS's largest.
2. Ehlers' 24-sorter (from his 2017 Kiel thesis, sec. 4.4): 12-ch 5-layer prefix with 34 outputs (beam search, keep 32), two copies SIDE BY SIDE
   (= mirrored, since the block is symmetric) with the two idle channels joined across ((0,12),(11,23)) -> 1154 outputs; the SAT-found layer 6 is a
   rank-aligned cross-half merge layer (1,13),(2,14),(3,15),(4,18),(5,19),(6,16),(7,17),(8,20),(9,21),(10,22) -> 479 outputs, whereas greedy
   comparator-at-a-time gives 572/585 and is structurally different. Completable trajectory 1154->479->235->137->83->54->34->25.
   Ehlers: greedy prefix construction "often fails because of only a few bad decisions"; suggests limited discrepancy search around greedy.
3. Calibration (reports 2-4): every known depth-13 completion had <= ~1200 outputs with 7 layers to go (CCEMS 800/840; Ehlers 1154; Wang 928);
   our 16+14 prefixes have ~1850. Dobbelaere's depth-14 networks: 32/185/14 = two 4-cubes + VV 5th layer + (0,16),(15,31): 28224->6887->1787->1231->
   547->418->221->129->79->46->33; 30/172/14 = 3-cube | 14-block | 3-cube: 7413->2564->1033 then 7 more layers. A network that fully sorts a
   16-block before merging needs >= 9+5 = 14 layers, so depth 13 needs cross-block comparators before layer 9 (Wang: layer 6).
4. No documented attempt at depth 13 for 29-32 exists anywhere (report 4); no lower-bound argument comes within 3 layers of excluding it.
5. Sound cheap pre-screen (Bundala-Zavodny subnetwork relaxation): SAT with only the prefix outputs of window <= w; UNSAT there implies UNSAT
   overall. `snt cnf --subnet w` implements it; timing test launched on run F prefix 0 (w = 8, 12, 16, 20).
6. Completability is not monotone under adding comparators (Knuth ex. 5.3.4-21) -> explains why greedy layer 7 kills completable prefixes.

## Runs K30/K32 — NEGATIVE (23:02 UTC)
First 7 layers of Dobbelaere's 30/172/14 network (|out| 1033) + 6 SAT layers: UNSAT in 3.2 s. First 7 layers of 32/185/14 (|out| 1231) + 6 SAT
layers: UNSAT in 4.2 s. (Those networks need their remaining 7 layers.)
Structured rank-aligned 6th layers (Ehlers-style cross-block matchings) on the 5-layer stacks: 30-ch 16+14 -> 3001..3689 outputs (greedy: 1853);
32-ch VV+VV translate/coordinate pairing -> 2540 (greedy: 1787); reflection-pair layers are much worse (4699..6561). Files: runs/structured6/.
15-channel generators stopped (mirrored 15+15 deprioritised: a 15-ch prefix needs |out| <= ~60 to beat the 16+14 stacks and the exhaustive
generation is too slow here).
Run F32 (23:10): the 2 best VV16+VV16 nested 6-layer prefixes (|out| 1787), 7 SAT layers, minisat 7200 s, 2 jobs — Wang's recipe for n=32.

## Control run — the run F negative does not depend on the normal-form constraints (23:11 UTC)
runs/n30_F0_control_no_nf_minisat: run F prefix 0 (|out| 1853), 7 SAT layers, symmetric, with psi1/psi3 removed (`--no_nf`; only the
necessary constraints of CCEMS Lemma 4/6 kept): UNSAT in 654 s (vs 520 s with them). Redundancy removal preserves reflection symmetry
(a comparator and its mirror are redundant together), so Lemma 4/6 apply to symmetric completions; hence: NO reflection-symmetric 13-layer
completion of that 6-layer prefix exists. Run H (non-symmetric suffix, same prefix) still running.
Subnet pre-screen on the same prefix: w=8 SAT 2 s, w=12 SAT 29 s, w=16 > 10 min -> not a useful quick filter here; stopped.
`snt cnf --max_comps K` implemented (Sinz sequential counter over suffix comparators; mirrored pairs count 2). n=10 check: from the
best 2-layer prefix (9 comparators), suffix <= 22 -> SAT (size 31 = best known (31,7)), suffix <= 20 -> UNSAT.

## Run F32 — NEGATIVE (23:25 UTC)
runs/n32_F_vv_L6_minisat: the 2 best VV16+VV16 nested 6-layer prefixes for n=32 (|out| 1787; CNFs 160k vars / 5.6M clauses), 7 SAT layers,
minisat: UNSAT in 798 s and 812 s. Wang's recipe transferred to 32 channels fails for these prefixes just as for 30. Next: prefixes 3-4.
