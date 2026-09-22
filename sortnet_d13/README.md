# Depth-13 sorting networks for 29–32 channels: research session record

**Status (2026-09-22): no 13-layer sorting network on 29, 30, 31 or 32 channels was found; the best known depth for these sizes remains 14.**
This directory contains a verified reproduction of Wang's 28-channel depth-13 result, an independent dependency-free reimplementation of
his pipeline (prefix generation with symmetric subsumption pruning, prefix stacking in nested and mirrored layouts, greedy layer extension,
the CCEMS SAT encoding, decoding with untangling, comparator stripping and re-layering), two independent exhaustive verifiers with tests,
and a documented set of negative and inconclusive results for 30 and 32 channels. Every network that is called verified passed both verifiers
on all 2^n binary inputs. Details, exact parameters and labels (VERIFIED / NEGATIVE / INCONCLUSIVE) are in `results.md`; the running log is
`NOTES.md`.

What was established: (i) depth 13 for n = 29..32 was still open on 2026-09-20 (Dobbelaere's list, arXiv, Scholar, GitHub checked);
(ii) Wang's pipeline reproduces (8/8 SAT instances, 8 verified 28-channel depth-13 networks) and our reimplementation reproduces his
prefix counts and finds SAT on his prefixes; (iii) for n = 30, the direct analog of his construction (Van Voorhis 16-channel prefix stacked
with the best available 14-channel prefixes, greedy sixth layer, SAT for layers 7–13) is provably not completable for the four best prefixes
(UNSAT in about 500 s each, also without the normal-form constraints), and every greedy seventh-layer commitment is UNSAT within seconds,
including on Wang's own completable 28-channel prefixes, which shows that greedy min-output extension beyond layer 6 is the wrong tool;
(iv) for n = 32 the same recipe (two Van Voorhis blocks, 1787 outputs after 6 layers) was given a 2-hour budget per instance (see `results.md`
for the outcome). What remains open is exactly what was open before: whether depth 13 is achievable for 29–32. The follow-up section of
`results.md` lists what to try next and why, with the literature calibration that all known depth-13 completions started from prefixes with
at most about 1200 outputs and 7 layers to go, whereas the best 30-channel prefixes reachable here had about 1850.
