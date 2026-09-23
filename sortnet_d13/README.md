# Depth-13 sorting networks for 29–32 channels: research session record

**Outcome (2026-09-23): no 13-layer sorting network on 29, 30, 31 or 32 channels was found; the best known depth for these sizes remains
14. One entry of Dobbelaere's list was improved: 27 inputs at depth 13, from 153 to 152 comparators (VERIFIED by both verifiers on all
2^27 inputs; `networks/n27d13_size152.txt`). It is the published 27-input network with one comparator deleted; an exhaustive single-deletion
sweep over all 54 networks of the list finds no other removable comparator.** Beyond that, the session produced a set of exact negative
results about the most natural constructions for 30 and 32 channels. First, Wang's 2025 recipe for 28 channels (stack a Van Voorhis 16-channel prefix with a small optimal prefix, add a
sixth layer greedily by minimal output set, solve layers 7–13 by SAT) was reproduced end to end (8/8 SAT, 8 verified networks) and then
transferred faithfully to 30 channels (16+14) and 32 channels (16+16): the twelve best 30-channel prefixes and the four best 32-channel
prefixes it produces have **no** reflection-symmetric 13-layer completion (MiniSat UNSAT in 500–1800 s each, confirmed without the
normal-form constraints). Second, the size of Wang's 28-channel depth-13 network (159) is optimal for its prefix among symmetric completions
(UNSAT with a cardinality bound). Third, a calibration nobody had
reported: adding a greedy seventh layer even to Wang's own completable prefixes makes them UNSAT in seconds, so greedy min-output extension
beyond layer 6 is the wrong tool, and the fast negatives of that kind carry no information.

Everything here is reproducible: `src/` holds two independent exhaustive verifiers (`verify_c.c`, `verify_py.py`, tested in `tests/`
against every network on Dobbelaere's page with n ≤ 20 and against broken networks), a dependency-free reimplementation of the whole
pipeline (`src/snt`: generate-and-prune prefixes with symmetric subsumption, nested/mirrored stacking, greedy extension, the CCEMS SAT
encoding with optional open last layer, cardinality bound and control flags, decoding with untangling, stripping, re-layering) and the run
driver; `networks/` holds the 13 verified networks produced (all reproductions of the known 28-channel bound plus one derived 27-channel
network, with provenance and `VERIFICATION.txt`); `results.md` gives every run with parameters, timings and a VERIFIED / NEGATIVE /
INCONCLUSIVE label, plus what a follow-up should try (longer budgets, SAT-scored beam search for layer 6, better block prefixes, unrestricted
suffixes); `NOTES.md` is the chronological log including the literature review; `submission_email.txt` is a template marked not to be sent.

**Paper manuscript:** `manuscript.md` is the full write-up (abstract, methods, the 27/152/13 network with its verification transcript, the size-optimality statement for Wang's prefix, the 30- and 32-channel negative results with instance sizes, discussion, limitations, reproducibility and confidence).
