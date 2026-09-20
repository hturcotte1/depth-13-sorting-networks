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
(to be filled)

## 3. n = 32 (Phase 3)
(to be filled)

## 4. Negative / inconclusive results
(to be filled)

## 5. What a follow-up should try
(to be filled)
