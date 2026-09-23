# results.md — depth-13 sorting networks for 29–32 channels: full account

Session: 2026-09-20 15:54 UTC to 2026-09-23 (with two harness pauses/restarts; all compute times are wall-clock on a 4-core, 15 GB VM,
frequently shared between jobs). Labels follow the brief: **VERIFIED** (both verifiers passed, commands given), **NEGATIVE** (a search ran to
completion without finding a network, exact parameters given), **INCONCLUSIVE** (timed out, killed, or not attempted).

**Headline: no 13-layer sorting network on 29, 30, 31 or 32 channels was found; the best known depth for these sizes remains 14.
One table entry was improved: 27 inputs, depth 13, size 153 → 152 (VERIFIED, §4.1).** What follows documents exactly what was tried, what is proven not to work, and what remains open.

## 0. Status check (Phase 0) — VERIFIED as a literature fact on 2026-09-20
Dobbelaere's list (https://bertdobbelaere.github.io/sorting_networks.html) was last updated 2025-11-07 (page footer; byte-identical to the
site repository HEAD, whose only later commit, 2026-02-22, changed median networks). Rows: 29: (164,15),(166,14); 30: (172,14); 31: (180,14);
32: (185,14); depth bounds 10…14 for all four. SorterHunter's `Networks/Sorters` holds no `Sort_{29..32}_*_13.json`. arXiv (API and search),
Semantic Scholar, OpenAlex, Google Scholar and GitHub searches found no depth-13 claim for 29–32; arXiv:2511.04107 (Wang; v1 2025-11-06,
v2 2025-11-22, no v3) has zero citations and covers 27/28 only. Conclusion: all four targets were open. (Workflow of 4 sweeps + 1 verifier;
details in `NOTES.md`.)

## 1. Calibration (Phase 1) — VERIFIED
* **Wang's pipeline reproduced.** His repository was built from source with a plain Makefile against apt libraries (Bazel cannot fetch
  dependencies through the egress proxy) with one local patch (sparse output-set computation; his bitset needs ~15 GB at n=28).
  Steps and wall times (README commands): 12-channel prefixes 13m47s, 16-channel prefixes 1m25s, stacking <1 s, greedy 6th layer 11 s,
  window permutation 2 s, CNF generation 11 s, MiniSat on 8 instances 20m47s (26.7 s … 906.9 s each, all SAT), decoding 53 s.
  Result: 8 networks, 28 channels, 13 layers, sizes 165–173, every one accepted by `src/verify_c` (all 2^28 inputs) and `src/verify_py`.
  Files: `networks/n28d13_wang_pipeline_repro_*.txt|json`, log `runs/phase1_wang/log.txt`.
* **Own reimplementation validated.** `src/snt` reproduces Wang's non-redundant symmetric 12-channel prefix counts (41 at depth 2,
  1502 at depth 3; depth 4/5 not completed for lack of CPU) and his two 83-output 16-channel prefixes; on his own 6-layer 28-channel prefixes
  my encoder + MiniSat gives SAT (102, 286, 348, 433 s) and the decoded networks (sizes 163–173) pass both verifiers
  (`networks/n28d13_own_encoder_on_wang_prefix_*`). At n=10 it finds depth 7 (optimal) and proves depth 6 impossible (`tests/test_pipeline.py`).
* **Solvers.** On two of Wang's 28-channel CNFs: MiniSat 27/39 s, Kissat 528/541 s, CaDiCaL 813/1184 s (loaded machine). MiniSat was used throughout.

## 2. n = 30 (Phase 2)
### 2.1 Prefix families (all reflection-symmetric; |out| = number of distinct binary outputs)
| family | 5 layers | + greedy 6th layer (keep 64) | + greedy 7th layer |
|---|---|---|---|
| S: VV16+VV16 nested minus channels {0,31} | 6723 | 1699 (65 prefixes) | 928 (74) |
| D30: first layers of Dobbelaere's 30/172/14 | 7413 | 2564 (its own 6th layer) | 1033 (its own 7th) |
| C30: 5-cube minus channels {0,31} | 7579 | – | – |
| N1: VV16 (83) + 14-channel keep-1 prefix (69), both nestings | 5727 | 1983 (82) | – |
| N4: VV16 + 14-channel keep-4 prefixes (66,69,71,72×5), both nestings, pooled | 5478–5976 | 1853 (66 prefixes, 1853…1973) | – |
| NL: VV16 + 14-channel keep-limited library (32 prefixes, best 66) | see §2.2 run F' | | |
| structured rank-aligned 6th layers on N4 (Ehlers-style) | 5478 | 3001–3689 | – |
14-channel prefixes: `snt gen --n 14 --sym --depth 5 --keep 1,1,1,1` (69), `--keep 4,4,4,8` (66…72), `--keep ,2000,4000,24` (32 prefixes, best 66,
2.6 h + 1.8 h). 16-channel: Van Voorhis 4-cube + weight-matched layer (83), two variants.

### 2.2 SAT runs (MiniSat 2.2; encoding `src/snt/cnf.cc` = Wang's port of CCEMS 2019; symmetric suffix unless stated)
| run | prefixes (|out|) | SAT layers | budget/instance | result | label |
|---|---|---|---|---|---|
| B | S, 7 layers (928…940), best 8 | 6 | 1800 s | 8/8 UNSAT, 8–12 s | NEGATIVE |
| D | S, 6 layers + 3 greedy layer-7 pairs frozen (1186…1208), best 8 | 6 | 1800 s | 8/8 UNSAT, 15–20 s | NEGATIVE |
| E | as D, layer 7 open for SAT on the other 24 channels | 6+ | 1800 s | 8/8 UNSAT, 15–24 s | NEGATIVE |
| A | S, 6 layers (1699, 1735, 1915, 1915) | 7 | 1800 s | 4/4 TIMEOUT; #5–#8 not run | INCONCLUSIVE |
| F | N4, 6 layers (1853, 1853, 1857, 1857) | 7 | 7200 s | 4/4 UNSAT, 501–520 s | NEGATIVE |
| F-control | N4 #1 (1853), normal-form constraints psi1/psi3 removed | 7 | 14400 s | UNSAT 654 s | NEGATIVE |
| H | N4 #1 (1853), suffix not required to be symmetric | 7 | 14400 s | RESULT_H | LABEL_H |
| G | N4, 5 layers (5478, 5478), no greedy layer | 8 | 14400 s | killed by container restart after ~3 h (602k vars, 21.8M clauses) | INCONCLUSIVE |
| K30 | D30, 7 layers (1033) | 6 | 3600 s | UNSAT 3.2 s | NEGATIVE |
| F5 | N4 #5–#12 (1857…1973) | 7 | 1200 s | 8/8 UNSAT, 841–1193 s | NEGATIVE |
| F' | NL pool, greedy 6th layer, best 8 | 7 | 1200 s | RESULT_FP | LABEL_FP |
Reproduce any row: `src/run_search.sh <name> <prefix library> 13 0 64 minisat <budget> <count> <jobs> [--open_last] [--no_nf] [--max_comps K]`;
the prefix libraries are under `runs/` (`n30_seed/vv_del_L6.txt`, `n30_16_14/stack_g4_L6.txt`, …), the CNF headers record the prefix and the
channel permutation, and each run's `log.txt` records sizes and timings.

Interpretation. The negatives are exact for the stated prefixes: with the CCEMS necessary constraints (last layer adjacent comparators only;
second-to-last layer span ≤ 3 with its implications), which hold for the non-redundant form of any network of the same depth and are compatible
with reflection symmetry (a comparator and its mirror are redundant together), UNSAT means no reflection-symmetric 13-layer completion exists.
The control run shows the verdict does not hinge on the normal-form constraints. The fast negatives B/D/E are uninformative about the 6-layer
prefixes: on Wang's own eight completable 28-channel prefixes, adding a greedy 7th layer (|out| 928 → 530) also gives 8/8 UNSAT in 4–9 s
(`runs/calib_wang_L7_minisat`), i.e. greedy min-|out| extension beyond layer 6 destroys completability (completability is not monotone under
adding comparators, Knuth 5.3.4 ex. 21).

## 3. n = 32 (Phase 3)
Seeds: VV16+VV16 nested or mirrored (6889; identical output sets), first 5 layers of 32/185/14 (6887), 5-cube (7581, Dedekind M(5)),
Green16 side by side (12100). VV16+VV16 greedy 6th layer → 1787 (64 prefixes), 7th → 992 (83).
| run | prefixes (|out|) | SAT layers | budget | result | label |
|---|---|---|---|---|---|
| K32 | first 7 layers of 32/185/14 (1231) | 6 | 3600 s | UNSAT 4.2 s | NEGATIVE |
| F32 | VV16+VV16 nested, 6 layers, #1–#2 (1787) | 7 | 7200 s | 2/2 UNSAT, 798 s and 812 s | NEGATIVE |
| F32b | same family, #3–#4 (1787) | 7 | 7200 s | 2/2 UNSAT, 1790 s and 1793 s | NEGATIVE |
Not attempted (INCONCLUSIVE): 5-cube prefix + greedy/SAT 6th layer; 8 free layers on the 5-layer VV16+VV16 stack; cube-and-conquer.
Consequences for 31/30/29 by channel deletion: none, since no 32-channel network was found.

## 4. Secondary targets (Phase 4)
### 4.1 Size at depth 13 for 27/28 (table: 153 and 159)
| test | result | label |
|---|---|---|
| single-comparator deletion from 28/159/13 (exact output-set check for each of the 159 comparators) | none removable | NEGATIVE |
| S28: Wang prefix #0 (83 comparators) + 7 layers with ≤ 75 suffix comparators (total ≤ 158), symmetric, with normal forms | UNSAT 989 s | NEGATIVE (under normal forms) |
| S28b: same, necessary constraints only (`--no_nf --max_comps 75`) | UNSAT 1207 s | NEGATIVE: from this prefix 159 is optimal among symmetric completions |
| 27 channels by deleting channel 0 or 27 from the 13 verified 28-channel networks | best 154 (> 153) | no improvement |
Not attempted: non-symmetric suffix with a size bound; other prefixes of Wang's family; SorterHunter-style local search (INCONCLUSIVE).
**Single-comparator deletion on the published 27/153/13 network: comparator (23,26) of layer 7 is removable, giving a 27-channel network
with 152 comparators and 13 layers — VERIFIED by `src/verify_c` (all 2^27 inputs) and `src/verify_py --mode all` (exhaustive and output-set);
`networks/n27d13_size152.txt|json`. This improves the (153, 13) entry for 27 inputs.** The same test finds nothing removable in the
26/141, 25/131, 24/120, 23/115 and 22/106 depth-13 networks. Follow-ups S27 (total ≤ 151 via SAT with a cardinality bound, non-symmetric suffix)
and S28c (28 channels, total ≤ 158, non-symmetric suffix): RESULT_S27S28C.
### 4.2 Direct n = 29
Not attempted (INCONCLUSIVE). Note that an odd-n network cannot be reflection-symmetric layer by layer (comparators on the middle channel
have no disjoint mirror), so the encoding would need a relaxed symmetry (middle-channel comparators unconstrained); see `NOTES.md`.

## 5. What a follow-up should try
Ranked by expected value per CPU-hour, based on the runs above and the literature review (`NOTES.md`, research section):
1. **Give the 6-layer 30/32-channel prefixes the budgets the literature used.** CCEMS spent up to 97,000 s per instance and Bundala et al.
   24 h; our 1800–7200 s budgets are small for instances 2–4x larger than theirs. Run G (5-layer stack, 8 free layers, 21.8M clauses) and the
   run A seed prefixes (1699/1735 outputs) are the natural candidates; on 4 cores that is days.
2. **Replace greedy min-|out| for layer 6 by a beam / limited-discrepancy search scored by SAT.** Every fast UNSAT came from a greedy
   commitment (layer 7 of the seed family; layer 6 of the 16+14 and 16+16 families, whose UNSAT proofs take 500–1800 s). Ehlers' completable
   24-channel prefix had a SAT-chosen layer 6 that greedy does not find. Concretely: keep the 200–500 best layer-6 candidates, sweep them with
   a 900 s budget each, and only then invest hours in survivors.
3. **Better block prefixes.** Wang's success used the true optimum (34/35 outputs) of an exhaustive 12-channel library; our 14-channel prefixes
   (66 outputs) came from keep-limited libraries. Untried: Green-16 blocks (110 outputs, different structure), the 5-cube for 32 with a
   beam-searched layer 6, 15+15 mirrored with a good 15-channel prefix (the non-symmetric 15-channel generation was too slow here).
4. **Drop the symmetry restriction for the suffix** (run H tests this on one prefix; the instance doubles).
5. **Encoding**: add the CCEMS psi2 (Lemma 8) constraints and the MiniSat settings they used (probing preprocessing, cla-decay 0.9999).
6. **Cube-and-conquer** on the layer-7 variables of the best instance to use many cores.

## 6. Confidence
The verified networks in `networks/` are reproductions of a known bound (28 channels, depth 13; sizes 163–173) and one derived 27-channel
network (154); each passed the exhaustive C verifier and the independent Python verifier (`networks/VERIFICATION.txt`), and the two verifiers
were themselves tested on all of Dobbelaere's networks with n ≤ 20 and on deliberately broken networks (`tests/`). A skeptical reviewer should
first rerun `tests/test_verifiers.py` and `tests/test_pipeline.py`, then `src/verify_c` on any file in `networks/`. The NEGATIVE labels are
claims about specific prefixes under the symmetric completion model and the stated depth; they rest on (i) the CNF encoding being a correct
model of "suffix sorts every prefix output" (validated by SAT on Wang's prefixes and by the n=10 optimum), and (ii) the CCEMS last-two-layer
restrictions being sound for symmetric completions (argued above; the control run removes the dependence on the normal-form part).
The reviewer should rerun one 500 s instance of run F with `--no_nf` and, if desired, without `--sym` (run H) to check both points.
