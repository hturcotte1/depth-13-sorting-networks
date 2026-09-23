# A 27-input sorting network with 152 comparators and 13 layers, and exact negative results for depth 13 on 30 and 32 inputs

**Author:** [NAME], with an AI research session (Claude).
**Date:** 23 September 2026.
**Artifact:** directory `sortnet_d13/` of the repository, branch `claude/depth-13-sorting-networks-hbmyhr`. All file paths and commands in this paper are relative to that directory.

## Abstract

We report a comparator network on 27 channels with 152 comparators and 13 layers. It improves the size of the best known 13-layer 27-input network on Dobbelaere's list of sorting networks from 153 to 152 at equal depth. The network is the published 153-comparator network with one comparator, (23,26) in layer 7, deleted; the comparator is not redundant in the classical sense (it changes some binary inputs), which is why standard redundancy stripping does not find it. Correctness was established by two independently written exhaustive verifiers, each checking all 2^27 = 134,217,728 binary inputs, and confirmed by a third algorithm (output-set propagation). An exhaustive single-comparator deletion test over all 54 networks of the list finds no other removable comparator, and no pair of comparators can be removed from the new network or from the 28-input 159-comparator 13-layer network.

The network was found while attempting the open problem that motivated the work: a 13-layer sorting network on 29, 30, 31 or 32 channels, where the best known depth is 14. That attempt failed, and we document exactly how. Wang's 2025 construction of a 13-layer 28-input network (stacked reflection-symmetric block prefixes, a greedy sixth layer, SAT for layers 7 to 13) was reproduced end to end and then transferred faithfully to 30 channels (16+14 blocks) and 32 channels (16+16 blocks and the 5-cube). For all 28 six-layer prefixes tested, 22 on 30 channels from three families and 6 on 32 channels from two families, the SAT instance for a reflection-symmetric completion in seven further layers is unsatisfiable, each proof taking between 500 and 1800 seconds with MiniSat; one instance was re-proved without the optional normal-form constraints. We further show that Wang's 159 comparators are optimal for his first prefix among reflection-symmetric completions, and that a greedy seventh layer destroys completability even for Wang's own completable prefixes, so fast negatives of that kind carry no information. Depth 13 for 29 to 32 inputs remains open; we give a ranked list of what a follow-up should try.

## 1. Introduction

A comparator network on n channels is a fixed sequence of compare-exchange operations; it is a sorting network if it sorts every input. Two cost measures are classical: the size (number of comparators) and the depth (number of parallel layers, each layer a set of disjoint comparators). Optimal values are known only for small n: optimal depth up to n = 17 (Bundala and Závodný 2014 for n ≤ 16; Codish, Cruz-Filipe, Ehlers, Müller and Schneider-Kamp 2019 for n = 17) and optimal size up to n = 12 (Harder 2020 for n = 11 and 12). Beyond that, the state of the art is a table of best known upper bounds, maintained by Dobbelaere, whose entries come from constructions (Batcher, Green, Van Voorhis), from heuristic search (SorterHunter) and from SAT-based prefix completion (Ehlers 2017 for n = 24; Wang 2025 for n = 27, 28).

At the time of this work (list dated 7 November 2025) the depth entries for 25 to 32 inputs read: 25, 26, 27 and 28 inputs at depth 13, and 29, 30, 31 and 32 inputs at depth 14, with lower bounds of 10 for all of them. The depth-13 entries for 27 and 28 are credited to Wang (2025), who found a reflection-symmetric 28-input network with 159 comparators in 13 layers; the depth-14 entries for 29 to 32 come from Batcher's odd-even merge. The question we set out to answer was whether Wang's method extends to 29 to 32 inputs, which would lower four table entries by one layer at once (a 32-input network yields 31-, 30- and 29-input networks of the same depth by channel deletion).

Contributions.

1. A 27-input sorting network with 152 comparators and 13 layers (Section 5), verified by two independent exhaustive checks, improving the list entry (153, 13). This is the first size improvement at depth 13 for 27 inputs since Wang's network.
2. An exhaustive single-comparator deletion analysis of all 54 networks on the list (Section 5.4): exactly one comparator anywhere on the list is deletable, the one above. Pair deletions from the 27/152 and 28/159 networks are impossible.
3. A size-optimality statement for Wang's prefix: no reflection-symmetric 13-layer completion of his first six-layer prefix has fewer than 76 suffix comparators (Section 5.5), so 159 is optimal for that prefix under symmetry.
4. Exact negative results for the transfer of Wang's construction to 30 and 32 channels (Sections 6 and 7): 28 six-layer prefixes from five families, all with unsatisfiable seven-layer completion instances, plus a control run showing the verdict does not depend on the normal-form constraints.
5. A methodological observation (Section 4.5): adding a greedy seventh layer to Wang's own eight completable 28-channel prefixes makes all eight instances unsatisfiable in under ten seconds. Greedy minimum-output extension beyond layer 6 is therefore the wrong tool, and negatives obtained that way say nothing about the underlying six-layer prefixes.
6. A complete, dependency-light reimplementation of the pipeline (prefix generation with symmetric subsumption, stacking, greedy extension, the CCEMS SAT encoding with additional controls, decoding with untangling, stripping, re-layering), two independent verifiers with a test suite, and every run's parameters and log, so that each claim here can be rerun.

Everything labelled VERIFIED below passed both verifiers; everything labelled NEGATIVE is an exact statement about specific prefixes under a stated completion model with the solver's proof of unsatisfiability; everything that timed out or was not run is labelled INCONCLUSIVE. A timeout is never treated as evidence of nonexistence.

## 2. Preliminaries

**Networks.** Channels are numbered 0 to n−1. A comparator (i, j) with i < j replaces the values on channels i and j by their minimum on i and their maximum on j. A layer is a set of comparators with pairwise disjoint channels; a network of depth d is a sequence of d layers. Its size is the total number of comparators. Following Dobbelaere's list, a network is written one layer per line as a list of pairs, e.g. `[(0,1),(2,3)]`.

**Zero-one principle.** A network sorts all inputs from a totally ordered set if and only if it sorts all 2^n binary inputs (Knuth, TAOCP vol. 3, §5.3.4). All verification in this paper is over binary inputs.

**Output sets.** For a prefix P (the first k layers) let out(P) be the set of binary vectors P produces from all 2^n binary inputs. A suffix S completes P to a sorting network iff S sorts every vector in out(P). The size |out(P)| is the standard measure of a prefix's quality: smaller sets give smaller SAT instances and, empirically, are easier to complete. The output set of a first layer with m comparators has 3^m·2^(n−2m) elements. Two useful reference values are the Dedekind numbers: a 4-cube on 16 channels (four layers of dimension-wise comparators) has 168 outputs and a 5-cube on 32 channels has 7,581.

**Reflection symmetry.** The reflection ρ(i) = n−1−i maps a comparator (i, j) to (ρ(j), ρ(i)). A network is reflection-symmetric if every layer is mapped to itself. Symmetry halves the number of free variables in a SAT encoding; it is the restriction Wang used, and the one under which every negative result in this paper is stated. A network on an odd number of channels cannot be reflection-symmetric layer by layer, since a comparator on the middle channel has no disjoint mirror image; this is why direct search on 29 channels needs a relaxed symmetry and why 29- and 31-input networks are usually obtained from 30- and 32-input ones by channel deletion.

**Channel deletion.** Fixing the input of the last channel to +∞ (or of the first to −∞) makes every comparator on that channel a pass-through, so removing those comparators and renumbering gives a sorting network on n−1 channels of the same depth and smaller size (Knuth 5.3.4); an interior channel can be deleted in the same way after relabelling the channels along the path the fixed value takes. This is how the list's odd-n entries are usually obtained from even-n networks, and how a 32-input network of depth 13 would settle 29 to 31 inputs at once.

**Redundant and removable comparators.** A comparator is redundant if it never swaps its inputs on any output of the preceding prefix; redundant comparators are removed by standard "stripping". A comparator can, however, swap inputs on some prefix outputs and still be unnecessary for sorting, because later comparators repair the difference. We call such a comparator removable. Removability must be decided by an exact test (does the network without it still sort?), which is what Section 5 does. Completability of a prefix is not monotone under adding comparators (Knuth 5.3.4, exercise 21): adding comparators can destroy the possibility of completing a prefix within a given depth, which is the mechanism behind Section 4.5.

**Re-layering.** A network's depth is the number of layers after greedy re-layering (each comparator placed in the earliest layer after the last comparator sharing one of its channels). All depths reported here were obtained this way, and all sizes were reported after stripping.

## 3. Status of the problem (VERIFIED as a literature fact on 20 September 2026)

Before any computation, the state of the problem was established from primary sources.

- Dobbelaere's list was last updated on 7 November 2025 (page footer). The served page is byte-identical to the head of the site's repository, whose only later commit (22 February 2026) changed median-selection networks. Rows: 29 inputs (164, 15), (166, 14); 30 inputs (172, 14); 31 inputs (180, 14); 32 inputs (185, 14); depth bounds 10 to 14 for all four. Rows for 26, 27 and 28 inputs: (138, 15), (139, 14), (141, 13); (147, 16), (148, 14), (153, 13); (155, 14), (159, 13). The depth-13 entries for 27 and 28 are credited to Wang 2025; the depth-13 entry for 26 to SorterHunter.
- SorterHunter's published network directory contains, for 29 to 32 inputs, only depth-14 and depth-15 networks.
- arXiv (API and full-text search), Semantic Scholar, OpenAlex, Google Scholar and GitHub searches found no claim of depth 13 for 29 to 32 inputs. Wang's arXiv:2511.04107 (v1 6 November 2025, v2 22 November 2025, no later version) had no citations and its own Table 1 lists 29 to 32 inputs at upper bound 14. No lower-bound argument in the literature comes within three layers of excluding depth 13 for these sizes.

Conclusion: depth 13 was open for 29, 30, 31 and 32 inputs when the work started.

## 4. Tools, verification methodology and calibration

### 4.1 Environment

All computation ran on a 4-core x86_64 virtual machine with 15 GB of RAM (a shared cgroup limit of 14.3 GB), Ubuntu 24.04, gcc 13.3, Python 3.11 with numpy, MiniSat 2.2.1 (distribution package), Kissat 4.0.4 and CaDiCaL 3.0.1 (built from source). The machine was frequently shared between jobs, so all wall-clock times below are upper bounds on the time an idle machine would need; they are reported as logged, never adjusted.

### 4.2 Two independent verifiers

**Verifier A** (`src/verify_c.c`, C with OpenMP) is a bit-sliced simulator. Sixty-four binary inputs are packed into one 64-bit word per channel (the low six bits of the input index select the lane, the remaining n−6 bits are constant across a word), every comparator becomes one AND and one OR, and an output word is sorted iff for every adjacent pair of channels `w[c] & ~w[c+1]` is zero. All 2^(n−6) words are processed and the number of failing inputs and the first failing input are reported. It exits 0 if the network sorts, 1 if not, 2 on a parse error.

**Verifier B** (`src/verify_py.py`, Python 3 and numpy) has its own parser and two algorithms unrelated to Verifier A's. Mode `exhaustive` simulates all 2^n inputs as integers in chunks of 2^22 and tests each output for the form 0^a 1^b. Mode `outputset` computes the set of reachable binary vectors layer by layer (product construction after the first layer, then comparator application and deduplication) and checks that the final set consists exactly of the n+1 sorted vectors; it also reports the size of the reachable set after every layer. `--mode all` runs both.

**Tests** (`tests/test_verifiers.py`, four tests). Both verifiers accept every network of Dobbelaere's list with n ≤ 20 and reject each of those networks with its last comparator deleted; both accept Wang's 28-input network and reject it with one comparator removed; both reject malformed inputs with a parse error, accept the one-comparator 2-sorter and a three-layer 4-sorter, and reject the same 4-channel network without its last layer. A second suite (`tests/test_pipeline.py`, two tests) exercises the search pipeline of Section 4.3: it reproduces Wang's counts of non-redundant reflection-symmetric 12-channel prefixes (41 at depth 2, 1,502 at depth 3), finds three depth-7 networks on 10 channels from two-layer prefixes by SAT that pass both verifiers, and proves depth 6 unsatisfiable for the same prefixes (depth 7 is optimal for 10 inputs, Parberry 1991).

Rule of evidence: no network is called VERIFIED in this paper unless both verifiers accepted it on all 2^n binary inputs.

### 4.3 The search pipeline

The pipeline in `src/snt/` (C++20, no external dependencies) reimplements Wang's method with additional controls. Its stages, each a sub-command of the `snt` binary:

- `gen`: generate-and-prune of reflection-symmetric prefixes layer by layer, with subsumption under channel permutations and under reflect-and-complement (Bundala and Závodný's lemma), keeping the k best by output-set size per layer when a `--keep` limit is given.
- `stack`: place a symmetric prefix on an outer block of channels and another on the inner block (nested) or a block and its reflection side by side (mirrored); the output set of the stack is the product of the block output sets.
- `extend`: greedy addition of one comparator at a time (a comparator and its mirror image) to a new layer, keeping the 64 best partial layers by output-set size (Wang's greedy sixth layer).
- `cnf`: the SAT encoding of Codish, Cruz-Filipe, Ehlers, Müller and Schneider-Kamp (2019) as ported by Wang: comparator variables g[k][i][j] with mirror identification, used-channel variables, "one up / one down" propagation variables per input and channel, per-input window encoding after a channel permutation that minimises the sum of windows (leading zeros and trailing ones of each prefix output are constants), the necessary last-layer constraints (only adjacent comparators in the last layer; span at most 3 in the second-to-last layer, with the implied structure), and the optional normal-form constraints psi1 and psi3 (no two adjacent idle channels in the last layer, and the corresponding forms for the second-to-last layer). Flags added for this work: `--no_nf` drops the normal-form constraints (the "necessary constraints only" model); `--open_last` leaves the last prefix layer partially specified for the solver to complete; `--max_comps K` adds a cardinality constraint on the number of suffix comparators (Sinz sequential counter; a mirrored pair counts two); `--subnet w` produces the Bundala–Závodný subnetwork relaxation that keeps only prefix outputs of window at most w.
- `decode`: reads a model, undoes the channel permutation with Knuth's untangling (when a comparator would be reversed, swap the labels of the two channels for all later comparators; valid for a suffix because the prefix output set contains all sorted vectors), strips redundant comparators and re-layers.
- `prune`: for each comparator (or each pair with `--pairs`), delete it and decide exactly by the output-set method whether the remaining network still sorts, caching the reachable set after each layer.

`src/run_search.sh` drives a whole run (CNF generation for the k best prefixes of a library, parallel solving with a time budget, decoding, and both verifiers on every decoded network) and writes `runs/<name>/log.txt` with instance sizes and per-instance outcomes and times. Every run reported below has such a log; the CNF and solution files themselves are not kept in the repository.

Sanity checks of the cardinality option: from the best two-layer prefix on 10 channels (9 comparators), a suffix of at most 22 comparators is satisfiable (size 31, matching the best known (31, 7) entry) and at most 20 is unsatisfiable.

### 4.4 Calibration on 28 channels (VERIFIED)

Wang's repository was built from source with a plain Makefile against distribution libraries (Bazel could not fetch its dependencies through the environment's egress proxy) with one local patch: his output-set routine uses a 2^n-bit mask and needs about 15 GB at n = 28, so it was replaced by a sparse computation. His README pipeline was then run unchanged.

| step | wall time |
|---|---|
| 12-channel symmetric prefixes to depth 5 (keep 4 at the last layer) | 13 min 47 s |
| 16-channel symmetric prefixes to depth 5 (keep 1 per layer; two 83-output prefixes) | 1 min 25 s |
| stacking (16 outer, 12 inner) and greedy sixth layer (keep 64) | 11 s |
| window-minimising channel permutation, CNF generation for 8 prefixes | 13 s |
| MiniSat on the 8 instances, 4 in parallel | 20 min 47 s wall |
| decoding, stripping, re-layering | 53 s |

Solver times for the eight instances were 26.7, 39.0, 44.2, 45.8, 197.1, 549.8, 906.9 and 1202.5 s, all satisfiable. The eight decoded networks (28 channels, 13 layers, sizes 165, 167, 167, 168, 170, 170, 173, 173) are in `networks/n28d13_wang_pipeline_repro_1..8.txt` and were accepted by both verifiers on all 2^28 inputs (`networks/VERIFICATION.txt`).

Our own encoder was then validated on Wang's prefixes: his four best six-layer prefixes (928 outputs each), encoded by `snt cnf --sym --depth 13` and solved by MiniSat, are satisfiable in 102, 286, 348 and 433 s, and the decoded networks (sizes 163, 166, 170, 173) pass both verifiers (`networks/n28d13_own_encoder_on_wang_prefix_0000..0003.txt`). Deleting channel 0 or channel 27 from each of the thirteen verified 28-channel networks gives 27-channel 13-layer networks of size at least 154, so none improves the list's 153 (the best is kept as `networks/n27d13_by_channel_deletion.txt`).

Solver choice: on two of Wang's instances (about 1,000 prefix outputs, 7 layers, 50 MB CNF each) MiniSat needed 27 and 39 s, Kissat 528 and 541 s, CaDiCaL 813 and 1,184 s on a loaded machine. MiniSat was used for every run below. Our 30- and 32-channel instances (150,000 to 180,000 variables, 5 to 6 million clauses) are two to four times larger than the largest instances solved by Codish et al.

### 4.5 A greedy seventh layer destroys completability (NEGATIVE, and a warning)

Wang's eight six-layer prefixes are all completable in seven more layers (Section 4.4). We extended each by one greedy layer chosen exactly as his sixth layer (comparator pairs added while the output set shrinks, keep 64), which reduced the output sets from 928 to 530 to 541, and asked for a completion in six layers. All eight instances are unsatisfiable, in 4, 5, 5, 5, 5, 4, 8 and 9 s (`runs/calib_wang_L7_minisat`). Small output sets are therefore not a proxy for completability past layer 6, and every fast negative in this paper that involved a greedy seventh layer (runs B, D, E, K30, K32 below) is uninformative about the prefixes that precede that layer. The fair test of a six-layer prefix is Wang's configuration itself, six fixed layers plus seven SAT layers, and that is the configuration used for all NEGATIVE labels on six-layer prefixes.

## 5. A 27-input sorting network with 152 comparators and 13 layers (VERIFIED)

### 5.1 Derivation

An exact single-comparator deletion test (Section 4.3, `prune`) was run on the published depth-13 networks for 22 to 28 inputs. Exactly one comparator on these seven networks is removable: comparator (23,26) in layer 7 of the network listed as "27 inputs, 153 CEs, 13 layers". Removing it leaves a 27-channel network with 152 comparators and 13 non-empty layers. The comparator is not redundant in the classical sense, since it swaps its inputs on some outputs of the first six layers; the network after layer 7 has a different output set with and without it, and only the exact test discovers that the last six layers sort both. After the deletion no further single comparator, and no pair of comparators (all 11,476 pairs tested), can be removed.

### 5.2 The network

27 channels, 152 comparators, 13 layers (layer sizes 13, 13, 13, 13, 12, 13, 10, 10, 12, 10, 12, 10, 11). Format as on Dobbelaere's list, one layer per line, channels numbered from 0.

```
[(0,19),(1,26),(2,25),(3,24),(4,23),(5,22),(6,21),(7,20),(9,10),(11,17),(12,15),(13,14),(16,18)]
[(0,1),(2,3),(4,5),(6,7),(8,9),(11,16),(12,14),(13,15),(17,18),(19,26),(20,21),(22,23),(24,25)]
[(0,2),(1,3),(4,6),(5,7),(8,18),(9,14),(10,12),(13,17),(15,16),(19,24),(20,22),(21,23),(25,26)]
[(0,4),(1,5),(2,20),(3,21),(6,19),(7,25),(8,13),(9,17),(10,11),(12,15),(14,18),(22,24),(23,26)]
[(1,2),(3,19),(4,6),(5,22),(7,20),(8,10),(9,12),(11,13),(14,16),(15,17),(21,23),(24,25)]
[(0,8),(1,4),(2,6),(3,10),(5,7),(9,11),(12,13),(14,15),(16,17),(18,19),(20,22),(21,24),(23,25)]
[(1,9),(2,13),(4,8),(5,12),(6,10),(7,20),(14,24),(15,22),(17,25),(18,21)]
[(3,4),(6,14),(7,11),(8,15),(9,18),(10,17),(12,23),(13,21),(16,20),(19,26)]
[(1,3),(2,4),(5,6),(7,8),(10,13),(11,15),(12,16),(14,18),(19,24),(20,23),(21,22),(25,26)]
[(2,7),(4,8),(6,9),(10,11),(12,14),(13,15),(16,18),(17,21),(19,20),(23,24)]
[(2,3),(4,7),(5,6),(8,10),(9,12),(11,16),(13,14),(15,17),(18,19),(20,23),(21,22),(24,25)]
[(4,5),(6,7),(8,9),(10,12),(11,13),(14,16),(15,18),(17,19),(20,21),(22,23)]
[(3,4),(5,6),(7,8),(9,10),(11,12),(13,14),(15,16),(17,18),(19,20),(21,22),(23,24)]
```

The number of distinct reachable binary vectors after each layer is 3,188,646; 272,160; 49,200; 11,424; 2,822; 884; 659; 472; 236; 118; 62; 39; 28, the last being the n+1 = 28 sorted vectors. Files: `networks/n27d13_size152.txt` (this listing with a provenance header) and `networks/n27d13_size152.json`.

### 5.3 Verification

The network was first verified on 23 September 2026 at 02:10 UTC (Verifier A 0.24 s, Verifier B in both modes 2 min 39 s). It was re-verified from a clean rebuild of Verifier A at 14:34 to 14:46 UTC the same day; the transcript below is verbatim from `runs/final_rerun/*.log`.

```
$ gcc -O3 -march=native -fopenmp -o src/verify_c src/verify_c.c
$ time src/verify_c networks/n27d13_size152.txt
file=networks/n27d13_size152.txt n=27 size=152 depth=13 inputs=134217728
RESULT: SORTS all 2^27 binary inputs (n=27, size=152, depth=13)
real 0m0.097s   user 0m0.350s   sys 0m0.004s   exit=0

$ time python3 src/verify_py.py --mode all networks/n27d13_size152.txt
file=networks/n27d13_size152.txt n=27 size=152 depth=13 modes=exhaustive,outputset
  exhaustive: SORTS
  outputset:  SORTS (|output| per layer: [3188646, 272160, 49200, 11424, 2822, 884, 659, 472, 236, 118, 62, 39, 28])
RESULT: SORTS all 2^27 binary inputs (n=27, size=152, depth=13)
real 6m44.338s   user 3m7.000s   sys 3m32.963s   exit=0

$ sha256sum networks/n27d13_size152.txt networks/n27d13_size152.json
6344af2c06599c6215326b8cb1fd2dc67190a78fc05e00930404a39728a3963c  networks/n27d13_size152.txt
39bb3b5ba3bb5ae46faa6d22bb500410667bd7db3b6ff2fdd87b00ed65505807  networks/n27d13_size152.json
(JSON layers identical to the text layers: True; size 152, depth 13 in both.)
```

Both verifiers parse the file independently, agree on size 152 and depth 13, and accept it on all 134,217,728 binary inputs; Verifier B's output-set mode confirms the result by an algorithm unrelated to input simulation. The test suites were rerun in the same session: `tests/test_verifiers.py` 4 of 4 passed in 37.8 s and `tests/test_pipeline.py` 2 of 2 passed in 4 min 2.6 s (Appendix B).

### 5.4 No other network on the list has a removable comparator (NEGATIVE)

The same exact single-deletion test was run on all 54 networks of Dobbelaere's list (`runs/prune_all.log`; the 32-input network takes about four minutes). Fifty-three networks have no removable comparator; the only exception is the 27/153/13 network above, and after its one deletion nothing further is removable. Pair deletions were tested exhaustively for the new 27/152 network (11,476 pairs, none removable, `runs/pairs_n27_152.log`) and for Wang's 28/159 network (12,561 pairs, none removable, `runs/pairs_n28_159.log`).

### 5.5 Size at depth 13 for 27 and 28 inputs by SAT with a cardinality bound

Let P0 be Wang's first six-layer 28-channel prefix (83 comparators, 928 outputs); his 159-comparator network is P0 plus a 76-comparator suffix.

| run | model | result | label |
|---|---|---|---|
| S28 | P0 + 7 symmetric layers, at most 75 suffix comparators, with normal forms | UNSAT, 989 s | NEGATIVE under normal forms only (psi1 can force redundant comparators into the count) |
| S28b | same, necessary constraints only (`--no_nf --max_comps 75`) | UNSAT, 1,207 s | NEGATIVE: no reflection-symmetric 13-layer completion of P0 has fewer than 76 suffix comparators; 159 is optimal for P0 under symmetry |
| S28c | same with a non-symmetric suffix (no `--sym`) | TIMEOUT at 14,400 s | INCONCLUSIVE |
| S27 | first 6 layers of the 27/152 network (77 comparators, 884 outputs) + 7 non-symmetric layers, at most 74 suffix comparators (total ≤ 151), necessary constraints only | TIMEOUT at 14,400 s | INCONCLUSIVE |

The S28b statement is sound because the non-redundant form of any 13-layer completion satisfies the necessary last-two-layer constraints, is no larger, and (Section 6.4) remains reflection-symmetric. Whether a 27-input 13-layer network with 151 comparators, or a 28-input one with 158, exists remains open.

## 6. Negative results for 30 channels

### 6.1 Prefix families

All prefixes are reflection-symmetric on 30 channels. VV16 denotes the first five layers of Van Voorhis's 16-input network (a 4-cube followed by a weight-matched fifth layer, 83 outputs; two variants exist with the same output-set size). Block prefixes on 14 channels were generated with `snt gen --n 14 --sym --depth 5` under three regimes: keep 1 per layer (69 outputs, 8 s), keep 4, 4, 4, 8 (eight prefixes with 66, 69, 71 and five times 72 outputs, 9 s), and a keep-limited library (all 69 prefixes at depth 2; 2,000 of 16,119 locally pruned candidates kept at depth 3; 4,389 of 49,487 at depth 4 from 5.76 million generated candidates, about 9.6 hours of wall clock including pauses; 32 prefixes at depth 5, best 66 outputs, 6,450 s for the last step). The library's best value equals the greedy keep-4 value, so the more exhaustive generation did not improve the stacks.

| family | construction | 5 layers | + greedy 6th layer (keep 64) | + greedy 7th |
|---|---|---|---|---|
| S | VV16+VV16 nested on 32 channels, channels 0 and 31 deleted | 6,723 | 1,699 (65 prefixes) | 928 (74 prefixes) |
| D30 | first layers of the list's 30/172/14 network | 7,413 | 2,564 (its own 6th layer) | 1,033 (its own 7th) |
| C30 | 5-cube with channels 0 and 31 deleted | 7,579 | not extended | |
| N1 | VV16 (83) + keep-1 14-channel prefix (69), both nestings | 5,727 | 1,983 (82 prefixes) | |
| N4 | VV16 + keep-4 14-channel prefixes, both nestings, 32 stacks pooled | 5,478 to 5,976 | 1,853 (66 prefixes, 1,853 to 1,973) | |
| NL | VV16 + library 14-channel prefixes, both variants and nestings, 128 stacks | 5,478 to 6,059 | 1,693 (best 8: 1,693, 1,693, 1,697, 1,697, 1,729, 1,729, 1,733, 1,733) | |
| structured | N4 stacks + Ehlers-style rank-aligned cross-block 6th layer | 5,478 | 3,001 to 3,689 | |

For comparison, Wang's 28-channel prefixes have 2,905 = 83 × 35 outputs after five layers and 928 after the greedy sixth layer; according to the literature review recorded in `NOTES.md`, every published SAT completion that left seven layers to the solver started from at most about 1,200 prefix outputs (Codish et al. about 800 and 840 on 17 channels, Ehlers 1,154 on 24 channels, Wang 928 on 28 channels). The best 30-channel six-layer prefixes here have 1,693 to 1,853.

### 6.2 SAT runs

MiniSat 2.2; encoding of Section 4.3; the suffix is required to be reflection-symmetric unless stated. "SAT layers" is the number of layers left to the solver. Instance sizes are in Appendix A.

| run | prefixes (outputs) | SAT layers | budget per instance | outcome | label |
|---|---|---|---|---|---|
| B | S, 7 layers (928 to 940), best 8 | 6 | 1,800 s | 8 of 8 UNSAT, 8 to 12 s | NEGATIVE (uninformative, Section 4.5) |
| D | S, 6 layers + 3 greedy layer-7 pairs frozen (1,186 to 1,208), best 8 | 6 | 1,800 s | 8 of 8 UNSAT, 15 to 20 s | NEGATIVE (uninformative) |
| E | as D, layer 7 open for the solver on the other 24 channels (`--open_last`) | 6+ | 1,800 s | 8 of 8 UNSAT, 15 to 24 s | NEGATIVE (uninformative) |
| A | S, 6 layers (1,699, 1,735, 1,915, 1,915) | 7 | 1,800 s | 4 of 4 TIMEOUT; prefixes 5 to 8 not run | INCONCLUSIVE |
| A2 | S, 6 layers, prefixes 1 and 2 (1,699, 1,735) | 7 | 7,200 s | 2 of 2 UNSAT, 549 s and 909 s | NEGATIVE |
| F | N4, 6 layers, best 4 (1,853, 1,853, 1,857, 1,857) | 7 | 7,200 s | 4 of 4 UNSAT, 501, 512, 517, 520 s | NEGATIVE |
| F-control | N4 prefix 1 (1,853), normal-form constraints removed (`--no_nf`) | 7 | 14,400 s | UNSAT, 654 s | NEGATIVE |
| F5 | N4 prefixes 5 to 12 (1,873 to 1,893) | 7 | 1,200 s | 8 of 8 UNSAT, 841 to 1,192 s | NEGATIVE |
| F' | NL, best 8 (1,693 to 1,733) | 7 | 1,200 s | prefixes 1 to 4 UNSAT, 798 to 823 s; 5 to 8 TIMEOUT | NEGATIVE (4), INCONCLUSIVE (4) |
| F'' | NL prefixes 5 to 8 (1,729, 1,729, 1,733, 1,733) | 7 | 7,200 s | 4 of 4 UNSAT, 1,650, 1,634, 1,057, 1,078 s | NEGATIVE |
| H | N4 prefix 1 (1,853), suffix not required to be symmetric | 7 | 14,400 s | TIMEOUT | INCONCLUSIVE |
| G | N4, 5 layers (5,478, 5,478), no greedy layer, 8 free layers | 8 | 14,400 s | killed by a container restart after about 3 h | INCONCLUSIVE |
| K30 | D30, 7 layers (1,033) | 6 | 3,600 s | UNSAT, 3 s | NEGATIVE (uninformative) |

Run A's timeouts were CPU contention: the same two instances are UNSAT in 549 and 909 s when rerun with two jobs (A2). The Bundala–Závodný subnetwork relaxation was tried as a cheap pre-screen on run F's first prefix: with window at most 8 it is satisfiable in 2.3 s, with window at most 12 in 28.8 s, and with window at most 16 it did not finish in ten minutes, so it does not filter these instances (`runs/subnet_screen`).

### 6.3 Statement

For each of the 22 six-layer prefixes of runs A2, F, F5, F' and F'' (2 from family S, 12 from N4, 8 from NL) there is no reflection-symmetric comparator network of 7 layers that sorts every output of the prefix. Hence none of these prefixes extends to a reflection-symmetric 13-layer sorting network on 30 channels. Each proof of unsatisfiability took between 500 and 1,800 s.

### 6.4 Soundness of the negatives

Each NEGATIVE label rests on two points.

1. The CNF is a correct model of "a symmetric suffix of the given depth sorts every prefix output". This was validated positively: the same encoder produces satisfiable instances and verified networks on Wang's 28-channel prefixes (Section 4.4) and finds the depth optimum on 10 channels (Section 4.2).
2. The restrictions on the last two layers (last layer adjacent comparators only; second-to-last layer span at most 3 with its implications; Lemmas 4 and 6 of Codish et al.) are necessary for the non-redundant form of any sorting network of the given depth, and removing redundant comparators preserves reflection symmetry, because a comparator and its mirror image are redundant together. So they exclude no symmetric completion. The normal-form constraints psi1 and psi3 are proven in the literature for unrestricted networks; rather than argue their soundness under the symmetry restriction, the control run re-proved run F's first instance with them removed (654 s instead of 520 s). The NEGATIVE labels for the other 21 prefixes still include the normal-form constraints; a reviewer who wants to remove that dependence can rerun any of them with `--no_nf` at about the same cost.

What the negatives do not say: nothing about non-symmetric completions (run H timed out), nothing about other sixth layers on the same five-layer stacks (run G was killed; the structured sixth layers were not solved), and nothing about prefixes outside these families.

## 7. Negative results for 32 channels

Seed prefixes (five layers, reflection-symmetric): VV16+VV16 nested or mirrored (6,889 = 83² outputs; the two arrangements have identical output sets), the first five layers of the list's 32/185/14 network (6,887), the 5-cube (7,581, the Dedekind number M(5)), and two Green-16 blocks side by side (12,100). The greedy sixth layer reduces VV16+VV16 to 1,787 (64 prefixes) and a greedy seventh to 992 (83 prefixes); it reduces the 5-cube to 2,416 (51 prefixes; 69 minutes under load). Ehlers-style structured sixth layers on VV16+VV16 give 2,540 (translation or coordinate pairing); reflection-pair layers are much worse (4,699 to 6,561 across the 30- and 32-channel stacks).

| run | prefixes (outputs) | SAT layers | budget | outcome | label |
|---|---|---|---|---|---|
| K32 | first 7 layers of 32/185/14 (1,231) | 6 | 3,600 s | UNSAT, 4 s | NEGATIVE (uninformative) |
| F32 | VV16+VV16 nested, 6 layers, prefixes 1 and 2 (1,787) | 7 | 7,200 s | 2 of 2 UNSAT, 812 s and 798 s | NEGATIVE |
| F32b | same family, prefixes 3 and 4 (1,823) | 7 | 7,200 s | 2 of 2 UNSAT, 1,790 s and 1,793 s | NEGATIVE |
| C32 | 5-cube + greedy 6th layer, best 2 (2,416, 2,516) | 7 | 3,600 s | 2 of 2 UNSAT, 818 s and 779 s | NEGATIVE |

Statement: none of the four best greedy six-layer prefixes of the Van Voorhis 16+16 family and none of the two best greedy six-layer prefixes of the 5-cube family has a reflection-symmetric 13-layer completion on 32 channels. Not attempted (INCONCLUSIVE): eight free layers on the five-layer VV16+VV16 stack; non-symmetric suffixes; cube-and-conquer. Since no 32-channel network was found, nothing follows for 31, 30 or 29 channels by deletion, and a direct search on 29 channels (which needs a relaxed symmetry, Section 2) was not attempted.

## 8. Discussion

**Why the transfer fails where Wang's succeeded.** Two quantitative differences stand out. First, the starting point: after five layers Wang's stacks have 2,905 outputs, ours 5,478 to 7,581, because the 14-channel and second 16-channel blocks are far from the 35-output quality of his exhaustively optimised 12-channel block (the best five-layer 14-channel symmetric prefix we could produce has 66 outputs; the block would need fewer than 81 to beat the seed family and far fewer to match Wang's ratio). Second, the hand-over to SAT: after the greedy sixth layer we hand the solver 1,693 to 1,853 outputs where every successful depth-13 completion in the literature handed it at most about 1,200. Whether this is a barrier or an artefact of the greedy sixth layer is the central open question. Ehlers' 24-channel result is instructive: according to our reading of his 2017 dissertation (recorded in `NOTES.md`), his completable prefix had a SAT-chosen rank-aligned sixth layer (1,154 to 479 outputs) that a comparator-at-a-time greedy does not find, and he observes that greedy prefix construction typically fails because of a few bad decisions, suggesting limited-discrepancy search around the greedy choice.

**Block completability does not discriminate.** We tested whether each five-layer block prefix completes to a depth-optimal sorter of the block alone under symmetry (16 channels in depth 9, 14 in depth 9, 12 in depth 8). The two 83-output VV16 variants split (one is symmetric-completable, the other only non-symmetrically), the 14-channel prefixes split, and among Wang's eight successful prefixes the inner 16-blocks split four to four. A depth-13 network on 32 channels cannot first sort two 16-blocks (9 layers each, in parallel) and then merge them (at least 5 layers, since the middle output depends on all 32 inputs): sorting and merging must interleave, and in Wang's network the cross-block comparators begin in layer 6.

**What a follow-up should try**, ranked by expected value per CPU hour:

1. Larger budgets for the fair instances. Per our reading of the literature, Codish et al. spent up to about 97,000 s on single instances and Bundala and Závodný up to 24 hours; our 1,800 to 7,200 s budgets are small for instances two to four times larger. Run G (five-layer stack, eight free layers, 602,000 variables and 21.8 million clauses) and run H (non-symmetric suffix) are the natural candidates; on four cores that is days.
2. A SAT-scored beam or limited-discrepancy search for layer 6 instead of greedy minimum output set: keep the 200 to 500 best sixth-layer candidates, sweep them with a 900 s budget each, invest hours only in survivors.
3. Better block prefixes: a true optimum for 14 channels (our libraries were keep-limited), Green-16 blocks (110 outputs but a different structure), a beam-searched sixth layer on the 5-cube for 32, and 15+15 mirrored stacks once a fast non-symmetric 15-channel generator is available.
4. Non-symmetric suffixes throughout (the instance doubles; run H is the template).
5. Encoding: add the co-saturation constraints of Codish et al. (their Lemma 8) and the solver settings they used (probing preprocessing, clause decay 0.9999).
6. Cube-and-conquer on the layer-7 variables of the best instance, to use many cores.

## 9. Limitations

- All negatives are for reflection-symmetric suffixes and specific prefixes; they do not bound depth 13 for 30 or 32 inputs in general.
- The normal-form constraints were removed in one control instance only; the other 21 30-channel negatives and all 32-channel negatives include them.
- Wall-clock times were measured on a shared four-core machine and are not comparable to each other or to the literature at better than a factor of two; the run A timeouts show the effect.
- The 14-channel prefix libraries were keep-limited, not exhaustive; a better 14-channel block may exist.
- The list entries used as baselines are those of 7 November 2025; a later update of the list would change the "improvement" claim, not the network.

## 10. Reproducibility

```
gcc -O3 -march=native -fopenmp -o src/verify_c src/verify_c.c
src/verify_c networks/n27d13_size152.txt                              # 0.1 s
python3 src/verify_py.py --mode all networks/n27d13_size152.txt        # needs numpy; about 7 min
python3 tests/test_verifiers.py                                       # about 40 s
make -C src/snt && python3 tests/test_pipeline.py                     # needs kissat in PATH; about 4 min
src/snt/snt prune --net data/dobb_nets/N27L153D13.txt --multiline     # re-derives the deletion (about 1 min)
src/snt/snt prune --net networks/n27d13_size152.txt --multiline --pairs
src/run_search.sh <name> <prefix library> 13 0 64 minisat <budget> <count> <jobs> [--no_nf] [--max_comps K]
```

The prefix libraries of every run are under `runs/<run>/prefix_in.txt`, the per-instance sizes and times in `runs/<run>/log.txt`, the list snapshot in `data/`, Wang's eight six-layer prefixes in `data/prefixes/wang_n28d6_64.txt`, the chronological log with timestamps in `NOTES.md`, and the full run tables in `results.md`. A reviewer with limited time should run the two verifier commands, then `tests/test_verifiers.py`, then one 500 s instance of run F with and without `--no_nf`.

## 11. Confidence

The claim that the 152-comparator network sorts rests on two independent exhaustive checks of all 2^27 binary inputs by programs written separately, one of which also confirms the result by output-set propagation, and both of which were validated against every network of Dobbelaere's list with n ≤ 20 and against deliberately broken networks. The depth is the number of non-empty layers in the listing (13) and the size the number of comparators in it (152); neither can be reduced by stripping, single deletion or pair deletion. The improvement claim rests on the list entry (153, 13) for 27 inputs as of 7 November 2025. The reproductions of Wang's bound are equally solid and add nothing new. The NEGATIVE labels are exact statements under a stated model whose two premises (a correct encoding, sound last-layer restrictions) were each tested positively; the one residual dependence, on the normal-form constraints under symmetry, was removed for one instance and can be removed for the others at known cost. A skeptical reader should rerun the two verifier commands first, then the control instance.

## Credit

Suggested credit line for the list entry, following the author's instructions: "[NAME], with an AI research session (Claude)". A draft submission note for the list maintainer is in `submission_email.txt`; it has not been sent.

## References

1. D. E. Knuth, *The Art of Computer Programming*, vol. 3, *Sorting and Searching*, 2nd ed., §5.3.4.
2. B. Dobbelaere, *List of sorting networks*, https://bertdobbelaere.github.io/sorting_networks.html (version of 7 November 2025).
3. C. Wang, *Depth-13 Sorting Networks for 28 Channels*, arXiv:2511.04107 (2025); code at https://github.com/wcgbg/sorting-network-n28d13.
4. M. Codish, L. Cruz-Filipe, T. Ehlers, M. Müller, P. Schneider-Kamp, *Sorting networks: to the end and back again*, Journal of Computer and System Sciences 104 (2019).
5. D. Bundala, J. Závodný, *Optimal sorting networks*, LATA 2014, LNCS 8370.
6. T. Ehlers, *Merging almost sorted sequences yields a 24-sorter*, Information Processing Letters 118 (2017).
7. J. Harder, *An answer to the Bose–Nelson sorting problem for 11 and 12 channels*, arXiv:2012.04400 (2020).
8. I. Parberry, *A computer-assisted optimal depth lower bound for nine-input sorting networks*, Mathematical Systems Theory 24 (1991).
9. K. E. Batcher, *Sorting networks and their applications*, AFIPS Spring Joint Computer Conference (1968).
10. B. Dobbelaere, *SorterHunter*, https://github.com/bertdobbelaere/SorterHunter.

## Appendix A. Instance sizes and per-instance outcomes

All instances: MiniSat 2.2; "vars" and "clauses" from the CNF header as logged; times are wall-clock seconds as logged.

| run | instance | prefix outputs | vars | clauses | outcome |
|---|---|---|---|---|---|
| A2 | 0 | 1,699 | 147,913 | 4,998,046 | UNSAT 549 |
| A2 | 1 | 1,735 | 150,569 | 5,085,804 | UNSAT 909 |
| F | 0 | 1,853 | 159,353 | 5,246,802 | UNSAT 520 |
| F | 1 | 1,853 | 159,577 | 5,267,242 | UNSAT 501 |
| F | 2 | 1,857 | 160,153 | 5,302,918 | UNSAT 512 |
| F | 3 | 1,857 | 160,409 | 5,324,598 | UNSAT 517 |
| F-control | 0 | 1,853 | 159,353 | 5,246,717 | UNSAT 654 |
| F5 | 0 to 7 | 1,873 ×4, 1,889 ×2, 1,893 ×2 | 160,777 to 162,473 | 5,203,072 to 5,390,088 | UNSAT 841, 1,112, 1,122, 1,192, 1,032, 985, 1,000, 880 |
| F' | 0 to 3 | 1,693, 1,693, 1,697, 1,697 | 146,361 to 147,449 | 4,867,514 to 4,963,014 | UNSAT 798, 815, 807, 823 |
| F' | 4 to 7 | 1,729, 1,729, 1,733, 1,733 | 148,985 to 150,105 | 4,956,888 to 5,050,772 | TIMEOUT 1,200 |
| F'' | 0 to 3 | 1,729, 1,729, 1,733, 1,733 | 149,113 to 150,105 | 4,979,656 to 5,050,772 | UNSAT 1,650, 1,634, 1,057, 1,078 |
| H | 0 | 1,853 | about 325,000 | not logged | TIMEOUT 14,400 |
| G | 0, 1 | 5,478 | 601,982 | 21,832,822 / 21,838,246 | killed at about 3 h |
| B | 0 to 7 | 928 ×4, 940 ×4 | 64,978 to 66,602 | 1,955,238 to 2,087,270 | UNSAT 11, 11, 10, 12, 9, 10, 9, 8 |
| D | 0 to 7 | 1,186 to 1,208 | 87,924 to 91,844 | 2,873,078 to 3,126,050 | UNSAT 16, 18, 20, 18, 20, 18, 16, 15 |
| E | 0 to 7 | 1,186 to 1,208 | 100,585 to 105,065 | 3,347,772 to 3,642,742 | UNSAT 18, 17, 24, 20, 21, 19, 18, 15 |
| K30 | 0 | 1,033 | 58,398 | 1,366,654 | UNSAT 3 |
| K32 | 0 | 1,231 | 81,284 | 2,199,176 | UNSAT 4 |
| F32 | 0, 1 | 1,787 | 159,874 / 160,098 | 5,583,368 / 5,604,228 | UNSAT 812, 798 |
| F32b | 0, 1 | 1,823 | 162,562 / 162,786 | 5,662,622 / 5,682,390 | UNSAT 1,790, 1,793 |
| C32 | 0, 1 | 2,416 / 2,516 | 183,314 / 186,946 | 4,994,662 / 5,033,048 | UNSAT 818, 779 |
| S28 | 0 | 928 | 271,628 | 2,654,064 | UNSAT 989 |
| S28b | 0 | 928 | 271,628 | 2,653,985 | UNSAT 1,207 |
| S28c | 0 | 928 | 275,842 | 2,682,405 | TIMEOUT 14,400 |
| S27 | 0 | 884 | 253,910 | 2,393,272 | TIMEOUT 14,400 |
| calib (Wang prefixes, own encoder) | 0 to 3 | 928 | 73,178 to 73,306 | 2,221,386 to 2,257,966 | SAT 102, 433, 286, 348 |
| calib + greedy 7th layer | 0 to 7 | 530 ×5, 540, 541, 541 | 34,918 to 36,528 | 961,960 to 1,082,112 | UNSAT 4, 5, 5, 5, 5, 4, 8, 9 |

Run A (eight instances, 1,699 to 1,977 outputs, 147,913 to 178,249 variables, 1,800 s budget, four jobs) produced four timeouts and was superseded by A2. The H instance's variable count is from the run description in `NOTES.md` (twice the symmetric instance); its CNF header line was not preserved in the log.

## Appendix B. Test suite transcript (rerun of 23 September 2026)

```
$ time python3 tests/test_verifiers.py
PASS test_dobbelaere_accept_and_broken_reject
PASS test_parse_errors
PASS test_trivial_networks
PASS test_wang28
4/4 passed
real 0m37.817s   exit=0

$ time python3 tests/test_pipeline.py
PASS test_gen_counts_n12
PASS test_sat_chain_n10
2/2 passed
real 4m2.555s   exit=0
```

## Appendix C. Verified networks in the repository

| file | n | size | depth | origin |
|---|---|---|---|---|
| `n27d13_size152` | 27 | 152 | 13 | list's 27/153/13 with layer-7 comparator (23,26) deleted; the new result |
| `n27d13_by_channel_deletion` | 27 | 154 | 13 | best channel deletion from the 28-channel networks below |
| `n28d13_wang_pipeline_repro_1..8` | 28 | 170, 167, 167, 165, 173, 173, 170, 168 | 13 | Wang's pipeline, rebuilt and rerun |
| `n28d13_own_encoder_on_wang_prefix_0000..0003` | 28 | 170, 166, 173, 163 | 13 | our encoder on Wang's prefixes |

Every file was accepted by both verifiers on all 2^n binary inputs; the outputs are recorded in `networks/VERIFICATION.txt`.
