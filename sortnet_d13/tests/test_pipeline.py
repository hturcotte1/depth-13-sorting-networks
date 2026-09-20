#!/usr/bin/env python3
"""
End-to-end test of the search pipeline (src/snt) at small n, where optimal
depths are known (Knuth / Bundala-Zavodny): 10 channels need depth 7.

  * snt gen produces the 18 non-redundant reflection-symmetric 2-layer prefixes for n=10
    (and reproduces Wang's counts 4, 41, 1502 for n=12 at depths 1, 2, 3);
  * snt cnf + kissat + snt decode finds depth-7 networks from those prefixes and both
    verifiers accept the decoded networks (this exercises the window permutation and the
    untangling of the suffix);
  * depth 6 is UNSAT for every prefix (sanity check that the encoding is not too weak).

Requires: src/snt/snt built (make -C src/snt), kissat in PATH, src/verify_c built.
Run: python3 tests/test_pipeline.py
"""
import glob
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SNT = os.path.join(ROOT, "src", "snt", "snt")
VC = os.path.join(ROOT, "src", "verify_c")
VP = os.path.join(ROOT, "src", "verify_py.py")
SOLVE = os.path.join(ROOT, "src", "solve.py")


def sh(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def count_nets(path):
    return sum(1 for l in open(path) if "[" in l)


def test_gen_counts_n12():
    d = tempfile.mkdtemp()
    r = sh([SNT, "gen", "--n", "12", "--sym", "--depth", "3", "--out", os.path.join(d, "n12.txt"), "--threads", "2"])
    assert r.returncode == 0, r.stderr[-2000:]
    assert count_nets(os.path.join(d, "n12.txt.d2")) == 41
    assert count_nets(os.path.join(d, "n12.txt.d3")) == 1502
    shutil.rmtree(d)


def test_sat_chain_n10():
    d = tempfile.mkdtemp()
    lib = os.path.join(d, "n10d2.txt")
    r = sh([SNT, "gen", "--n", "10", "--sym", "--depth", "2", "--out", lib, "--threads", "1"])
    assert r.returncode == 0, r.stderr[-2000:]
    assert count_nets(lib) == 18
    # depth 7: SAT for the first 3 prefixes, decoded networks verified by both verifiers
    c7 = os.path.join(d, "cnf7")
    r = sh([SNT, "cnf", "--in", lib, "--sym", "--depth", "7", "--outdir", c7, "--limit", "3"])
    assert r.returncode == 0, r.stderr[-2000:]
    r = sh([sys.executable, SOLVE, c7, "--solver", "kissat", "--timeout", "120", "--jobs", "2", "--decode", SNT])
    assert r.returncode == 0, r.stderr[-2000:]
    nets = sorted(glob.glob(os.path.join(c7, "*.kissat.net.txt")))
    assert len(nets) == 3, r.stdout
    for p in nets:
        assert sh([VC, "-q", p]).returncode == 0, p
        assert sh([sys.executable, VP, "-q", "--mode", "all", p]).returncode == 0, p
        depth = sum(1 for l in open(p) if l.startswith("["))
        assert depth == 7, (p, depth)
    # depth 6: UNSAT
    c6 = os.path.join(d, "cnf6")
    r = sh([SNT, "cnf", "--in", lib, "--sym", "--depth", "6", "--outdir", c6, "--limit", "3"])
    assert r.returncode == 0
    r = sh([sys.executable, SOLVE, c6, "--solver", "kissat", "--timeout", "120", "--jobs", "2"])
    assert r.stdout.count(" UNSAT ") == 3, r.stdout
    shutil.rmtree(d)


if __name__ == "__main__":
    tests = [t for name, t in sorted(globals().items()) if name.startswith("test_")]
    failed = 0
    for t in tests:
        try:
            t()
            print("PASS", t.__name__)
        except AssertionError as e:
            failed += 1
            print("FAIL", t.__name__, str(e)[:500])
    print("%d/%d passed" % (len(tests) - failed, len(tests)))
    sys.exit(1 if failed else 0)
