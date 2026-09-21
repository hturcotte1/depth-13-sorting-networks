# results.md — depth-13 sorting networks for 29–32 channels: full account

(Working document; sections are labelled VERIFIED / NEGATIVE / INCONCLUSIVE as defined in the brief.)

## 0. Status check (Phase 0) — VERIFIED (as a literature fact, 2026-09-20)
Depth 13 is open for n = 29, 30, 31, 32: Dobbelaere's list (last updated 2025-11-07, page byte-identical to the
site repository HEAD; only later commit 2026-02-22 concerns median networks) lists depth 14 as the best known for all four;
SorterHunter's `Networks/Sorters` has no `Sort_{29..32}_*_13.json`; arXiv, Semantic Scholar, OpenAlex, Google Scholar and
GitHub searches found no depth-13 claim for these n; arXiv:2511.04107 (v2, 2025-11-22) has zero citations and covers 27/28 only.

## 1. Calibration (Phase 1) — VERIFIED
Wang's pipeline (github.com/wcgbg/sorting-network-n28d13), built from source against apt libraries with one memory patch,
reproduced his 28-channel depth-13 result: 8/8 SAT instances satisfiable with MiniSat (26.7 s … 906.9 s each),
8 decoded networks of depth 13 and sizes 165–173; each passes `verify_c` (exhaustive 2^28) and `verify_py`.
Total wall time ≈ 37 min on 4 cores. Commands: see `runs/phase1_wang/log.txt`.
My own reimplementation (`src/snt`) reproduces Wang's non-redundant symmetric 12-channel prefix counts
(41 at depth 2, 1502 at depth 3; deeper counts below) and finds verified optimal-depth networks for n = 10 with the
same SAT chain (`tests/test_pipeline.py`).

## 2. n = 30 (Phase 2)

### 2.1 Prefix families tried (all reflection-symmetric, 5 layers) and their output-set sizes
| family | 5-layer |out| | greedy 6th layer (keep 64) | greedy 7th layer |
|---|---|---|---|
| S: VV16+VV16 nested minus channels {0,31} (seed) | 6723 | 1699 (65 prefixes, 1699..2230) | 928 (74 prefixes) |
| N1: VV16 outer/inner + 14-ch keep-1 prefix (|out14|=69), both nestings | 5727 | 1983 (82 prefixes) | – |
| N4: VV16 + 14-ch keep-4 prefixes (|out14| 66..72), both nestings, pooled | 5478..5976 | 1853 (66 prefixes, 1853..1973) | – |
(16-channel block: Van Voorhis 4-cube + weight-matched 5th layer, |out| = 83, reproduced by `snt gen --n 16 --sym --depth 5 --keep 1,1,1,1`.)

### 2.2 SAT runs (minisat 2.2, encoding = Wang/CCEMS as in `src/snt/cnf.cc`, symmetric, window-permuted)
| run | prefixes | SAT layers | budget | result | label |
|---|---|---|---|---|---|
| B | S, 7 layers (928..940), best 8 | 6 | 1800 s | 8/8 UNSAT in 8–12 s | NEGATIVE |
| D | S, 6 layers + first 3 greedy pairs of layer 7 frozen (1186..1208), best 8 | 6 | 1800 s | 8/8 UNSAT in 15–20 s | NEGATIVE |
| E | as D but layer 7 open for SAT on the 24 free channels | 6 (+ open layer) | 1800 s | 8/8 UNSAT in 15–24 s | NEGATIVE |
| A | S, 6 layers (1699, 1735, 1915, 1915) | 7 | 1800 s | 4/4 TIMEOUT (instances 5–8 not run) | INCONCLUSIVE |
| F | N4, 6 layers (1853, 1853, 1857, 1857) | 7 | 7200 s | 4/4 UNSAT in 501–520 s | NEGATIVE |
Reproduce: `src/run_search.sh <name> <prefix library> 13 0 64 minisat <budget> <count> 4 [--open_last]`; prefix libraries are in `runs/`.
Calibration of this negative pattern: Wang's own eight 28-channel 6-layer prefixes (all SAT with 7 layers) become 8/8 UNSAT in
4–5 s after a greedy 7th layer (|out| 928 → 530). Hence runs B/D/E only show that greedy layer-7 choices are incompatible with
completion, not that the underlying 6-layer prefixes are.
Negative results are for the stated prefixes only: they show those specific 7-layer prefixes cannot be completed to a 13-layer
sorting network (the encoder is complete for symmetric completions of the given prefix with the CCEMS last-two-layer restrictions,
and it reproduces SAT on Wang's prefixes; see §1).

## 3. n = 32 (Phase 3)
Seed VV16+VV16 nested (|out| 6889; the 5-cube has 7581, the first 5 layers of the 185/14 network 6887): greedy 6th layer 1787
(64 prefixes), 7th layer 992 (83 prefixes). SAT runs: not yet started (waiting for the n=30 outcome).

## 4. Negative / inconclusive results
(to be filled)

## 5. What a follow-up should try
(to be filled)
