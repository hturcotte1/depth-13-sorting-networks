- Greedy 7th layer on the 65 six-layer prefixes (keep 64, 1 thread, 4m38s): best |out| 1493 -> 1320 -> 1186 -> 1080 -> 1014 -> 930 -> 928
  after 7 mirrored pairs; 74 prefixes with |out| 928..964. Saved runs/n30_seed/vv_del_L7.txt. This is Wang's instance size with 6 layers left.
- Run A aborted by a bug (64-byte filename buffer truncated the absolute CNF path; fixed in snt.cc). Rerun as run A after run B.
- Run B (17:45 UTC): 7-layer seed prefixes, best 8 (|out| 928..~950), depth 13 = 6 SAT layers, minisat, 1800 s each, 4 jobs.

## Run D result — NEGATIVE (18:14 UTC)
runs/n30_D_seed_L6p3_minisat: 7-layer prefixes whose 7th layer holds only the first 3 greedy mirrored pairs (frozen; |out| 1186..1208),
6 SAT layers: all 8 UNSAT in 15-20 s. So even the first three greedy layer-7 choices are incompatible with any 13-layer completion
of these 6-layer prefixes (or the 6-layer prefixes themselves are not completable; run A left that open).
New mode `snt cnf --open_last`: SAT may also place comparators on the free channels of the prefix's last (partial) layer
(extra suffix layer 0 restricted to those channels, merged at decode time). Smoke-tested at n=10.
Run E (18:22): open-last on the p3 prefixes (3 fixed pairs + free completion of layer 7 + 6 layers), minisat 1800 s.
n=14/n=15 generators restarted with keep limits at every depth (,2000,4000,24 and ,1500,3000,16): the unlimited depth-3 prune of
millions of candidates is quadratic and was not going to finish.
