- Greedy 7th layer on the 65 six-layer prefixes (keep 64, 1 thread, 4m38s): best |out| 1493 -> 1320 -> 1186 -> 1080 -> 1014 -> 930 -> 928
  after 7 mirrored pairs; 74 prefixes with |out| 928..964. Saved runs/n30_seed/vv_del_L7.txt. This is Wang's instance size with 6 layers left.
- Run A aborted by a bug (64-byte filename buffer truncated the absolute CNF path; fixed in snt.cc). Rerun as run A after run B.
- Run B (17:45 UTC): 7-layer seed prefixes, best 8 (|out| 928..~950), depth 13 = 6 SAT layers, minisat, 1800 s each, 4 jobs.
