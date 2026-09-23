# A 27-input sorting network with 152 comparators and 13 layers, and negative results for depth 13 on 30 and 32 inputs

**Author:** [NAME], with an AI research session (Claude). **Date:** 23 September 2026.
**Repository:** `sortnet_d13/` on branch `claude/depth-13-sorting-networks-hbmyhr` (all commands below are relative to that directory).

## Abstract

We report a comparator network on 27 channels with 152 comparators and 13 layers. It improves the size of the best known 13-layer
27-input network on Dobbelaere's list from 153 to 152 at equal depth. The network is obtained from the published 153-comparator network
by deleting a single comparator, (23,26) in layer 7, which acts on some binary inputs but is not needed for sorting. Correctness was
established by two independently written verifiers, each checking all 2^27 = 134,217,728 binary inputs (zero-one principle); their
outputs, run times and file checksums are reproduced verbatim in Section 4. An exhaustive single-comparator deletion test over all 54
networks of the list finds no other removable comparator, and no pair of comparators is removable from the new network or from the
28-input 159-comparator 13-layer network. We also report that the natural extension of Wang's 2025 construction to 30 and 32 channels
fails: for 28 six-layer prefixes across five prefix families, the SAT instance for the remaining seven layers is unsatisfiable. The
question whether depth 13 is achievable for 29, 30, 31 or 32 inputs remains open.

## 1. Background

A comparator network on n channels is a sequence of layers, each a set of disjoint comparators (i, j), i < j, which places the minimum
of the two values on channel i. Its size is the number of comparators and its depth the number of layers. By the zero-one principle
(Knuth, TAOCP vol. 3, §5.3.4) a network sorts all inputs iff it sorts all 2^n binary inputs. Dobbelaere's "List of sorting networks"
(last updated 7 November 2025) records, per n, the best known (size, depth) pairs. For n = 27 it lists (147, 16), (148, 14) and (153, 13),
the last one credited to Wang (arXiv:2511.04107), whose reflection-symmetric 28-channel 13-layer network with 159 comparators yields a
27-channel network by deleting one channel. For n = 29, 30, 31, 32 the best known depth is 14 (Batcher odd-even merge and derived networks).

## 2. Methods

**Verifier A** (`src/verify_c.c`, C, OpenMP). Bit-sliced simulation: 64 binary inputs are packed per 64-bit word (the low six bits of the
input index select the lane, the remaining bits are constant across a word), every comparator becomes one AND and one OR, and an output
word is sorted iff every adjacent pair of channels satisfies `w[c] & ~w[c+1] == 0`. All 2^(n-6) words are processed; the program
reports the number of failing inputs and the first one.

**Verifier B** (`src/verify_py.py`, Python 3 + numpy, separate parser and separate algorithms). Mode `exhaustive` simulates all 2^n inputs in
chunks of 2^22 integers and tests each output for the form 0^a 1^b. Mode `outputset` computes the set of reachable binary vectors layer
by layer (the product construction after layer 1, then comparator application and deduplication) and checks that the final set is exactly
the n + 1 sorted vectors. `--mode all` runs both.

Both verifiers were tested (`tests/test_verifiers.py`) on every network of Dobbelaere's list with n ≤ 20 (all accepted), on each of those
networks with its last comparator deleted (all rejected by both), on Wang's 28-input network with one comparator removed (rejected), and
on malformed inputs. A second suite (`tests/test_pipeline.py`) checks the search pipeline used in the negative results: it reproduces
Wang's counts of non-redundant symmetric 12-channel prefixes (41 at depth 2, 1502 at depth 3), finds three verified depth-7 networks on
10 channels from 2-layer prefixes by SAT, and proves depth 6 unsatisfiable for the same prefixes (depth 7 is optimal for 10 inputs).

**Deletion test** (`src/snt prune`, C++). For each comparator of a network, delete it and decide exactly, by the output-set method with the
reachable set cached after every layer, whether the remaining network still sorts; `--pairs` does the same for every pair of comparators.

## 3. The network

27 channels, 152 comparators, 13 layers (layer sizes 13, 13, 13, 13, 12, 13, 10, 10, 12, 10, 12, 10, 11). Format as on Dobbelaere's list:
one layer per line, channels numbered from 0.

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

**Derivation.** This is the network listed as "27 inputs, 153 CEs, 13 layers" on Dobbelaere's page with the comparator (23,26) removed from
layer 7; no other change. The comparator is not redundant in the usual sense (it swaps its inputs on some binary vectors), which is why
standard stripping of unused comparators does not find it. After its removal no further single comparator, and no pair of comparators,
can be deleted (11,476 pairs tested). The number of distinct reachable binary vectors after each layer is
3,188,646; 272,160; 49,200; 11,424; 2,822; 884; 659; 472; 236; 118; 62; 39; 28 (the final 28 = n + 1 sorted vectors).

Files: `networks/n27d13_size152.txt` (this listing with a provenance header) and `networks/n27d13_size152.json`.

## 4. Verification (rerun 23 September 2026, 14:34–14:46 UTC, 4-core VM)

The C verifier was recompiled from source immediately before the run. Outputs are verbatim from `runs/final_rerun/*.log`.

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

Both verifiers parse the file independently, agree on size 152 and depth 13, and accept it on all 134,217,728 binary inputs. Verifier B
additionally confirms the result by the output-set method, an algorithm unrelated to input simulation.

## 5. Test suites (same rerun)

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

## 6. Related negative results from the same session

All statements below are for reflection-symmetric completions of the stated prefixes, decided by MiniSat 2.2 on the encoding of
Codish, Cruz-Filipe, Ehlers, Müller and Schneider-Kamp (JCSS 2019) as implemented by Wang; full tables with parameters and timings are
in `results.md`.

| statement | evidence |
|---|---|
| Wang's 28-input 13-layer construction reproduces | 8/8 SAT instances, 8 verified networks (sizes 165–173); our reimplementation gives SAT on his prefixes too |
| Its transfer to 30 inputs (Van Voorhis 16-channel prefix + best available 14-channel prefixes, greedy 6th layer, SAT for layers 7–13) fails for every prefix tested | 22 six-layer prefixes from three families, all UNSAT in 500–1800 s; one confirmed UNSAT without the normal-form constraints |
| Its transfer to 32 inputs (two Van Voorhis blocks; the 5-cube) fails for every prefix tested | 6 six-layer prefixes from two families, all UNSAT in 780–1800 s |
| Greedy min-output extension beyond layer 6 destroys completability | Wang's own eight completable 28-input prefixes become UNSAT in 4–9 s after a greedy 7th layer |
| Wang's 159 is optimal for his first prefix among symmetric completions | SAT with a cardinality bound of 75 suffix comparators: UNSAT (1207 s, necessary constraints only) |
| No other network on the list has a removable comparator | exhaustive single-deletion test over all 54 networks; pair test negative for 27/152 and 28/159 |
| Inconclusive (4-hour timeouts) | 8 free layers on the 5-layer 30-input stack; a non-symmetric suffix on the best 30-input prefix; 27 inputs with ≤ 151 comparators and 28 inputs with ≤ 158 by SAT with non-symmetric suffixes |

## 7. Reproducibility

```
gcc -O3 -march=native -fopenmp -o src/verify_c src/verify_c.c
src/verify_c networks/n27d13_size152.txt
python3 src/verify_py.py --mode all networks/n27d13_size152.txt        # needs numpy; ~7 min
python3 tests/test_verifiers.py                                       # ~40 s
make -C src/snt && python3 tests/test_pipeline.py                     # needs kissat in PATH; ~4 min
src/snt/snt prune --net data/dobb_nets/N27L153D13.txt --multiline     # re-derives the deletion from the published network (~1 min)
```

## 8. Confidence

The claim that the 152-comparator network sorts rests on two independent exhaustive checks of all 2^27 binary inputs by programs written
separately, validated against every network of Dobbelaere's list with n ≤ 20 and against deliberately broken networks. The depth is the
number of non-empty layers in the listing (13); the size is the count of comparators in it (152). The claim of improvement rests on the
list entry (153, 13) for 27 inputs as of 7 November 2025. A skeptical reader should rerun the two verifier commands of Section 7 first.

## References

1. D. E. Knuth, *The Art of Computer Programming*, vol. 3, §5.3.4.
2. B. Dobbelaere, *List of sorting networks*, https://bertdobbelaere.github.io/sorting_networks.html (updated 7 November 2025).
3. C. Wang, *Depth-13 Sorting Networks for 28 Channels*, arXiv:2511.04107 (2025); code at github.com/wcgbg/sorting-network-n28d13.
4. M. Codish, L. Cruz-Filipe, T. Ehlers, M. Müller, P. Schneider-Kamp, *Sorting networks: to the end and back again*, JCSS 104 (2019).
5. D. Bundala, J. Závodný, *Optimal sorting networks*, LATA 2014.
6. T. Ehlers, *Merging almost sorted sequences yields a 24-sorter*, Information Processing Letters 118 (2017).
