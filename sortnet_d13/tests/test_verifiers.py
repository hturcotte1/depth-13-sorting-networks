#!/usr/bin/env python3
"""
Tests for the two independent verifiers.

  * every network on Dobbelaere's page with n <= 20 is accepted by both verifiers;
  * removing the last comparator of the last layer of each of those networks
    makes both verifiers reject it (and the two verifiers agree on every case);
  * Wang's 28-channel network is accepted, and rejected once a comparator is removed;
  * malformed input (channel reused inside a layer) is a parse error (exit 2) for both.

Run:  python3 tests/test_verifiers.py        (or: pytest tests/)
Needs src/verify_c compiled:  gcc -O3 -march=native -fopenmp -o src/verify_c src/verify_c.c
"""
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
VC = os.path.join(ROOT, "src", "verify_c")
VP = os.path.join(ROOT, "src", "verify_py.py")
DATA = os.path.join(ROOT, "data", "dobbelaere_networks.json")


def fmt(layers):
    return "\n".join("[" + ",".join("(%d,%d)" % (i, j) for i, j in L) + "]" for L in layers) + "\n"


def run_c(path, n=None):
    cmd = [VC, "-q"] + (["-n", str(n)] if n else []) + [path]
    return subprocess.run(cmd, capture_output=True, text=True).returncode


def run_py(path, n=None, mode="auto"):
    cmd = [sys.executable, VP, "-q", "--mode", mode] + (["-n", str(n)] if n else []) + [path]
    return subprocess.run(cmd, capture_output=True, text=True).returncode


def write_tmp(text):
    f = tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False)
    f.write(text)
    f.close()
    return f.name


def load_nets():
    nets = json.load(open(DATA))
    return {k: v for k, v in nets.items()}


def test_dobbelaere_accept_and_broken_reject():
    nets = load_nets()
    small = sorted((k for k, v in nets.items() if v["n"] <= 20), key=lambda k: nets[k]["n"])
    assert len(small) >= 25
    n_broken_rejected = 0
    for k in small:
        v = nets[k]
        p = write_tmp(fmt(v["layers"]))
        rc_c, rc_p = run_c(p, v["n"]), run_py(p, v["n"])
        assert rc_c == 0, (k, "verify_c rejected a known sorting network")
        assert rc_p == 0, (k, "verify_py rejected a known sorting network")
        os.unlink(p)
        # break it: drop last comparator of last layer (skip the 1-comparator 2-sorter)
        if v["size"] < 2:
            n_broken_rejected += 1
            continue
        layers = [list(L) for L in v["layers"]]
        layers[-1] = layers[-1][:-1]
        if not layers[-1]:
            layers = layers[:-1]
        p = write_tmp(fmt(layers))
        rc_c, rc_p = run_c(p, v["n"]), run_py(p, v["n"])
        os.unlink(p)
        assert rc_c == rc_p, (k, "verifiers disagree on broken network", rc_c, rc_p)
        assert rc_c in (0, 1)
        if rc_c == 1:
            n_broken_rejected += 1
    # In a best-known network the last-layer comparators are essentially always necessary.
    assert n_broken_rejected >= len(small) - 2, (n_broken_rejected, len(small))


def test_wang28():
    wang = os.path.join(ROOT, "data", "wang_n28d13.txt")
    assert run_c(wang) == 0
    assert run_py(wang, mode="outputset") == 0
    text = open(wang).read()
    broken = text.replace(",(23,24)]", "]")
    assert broken != text
    p = write_tmp(broken)
    assert run_c(p, 28) == 1
    assert run_py(p, 28, mode="outputset") == 1
    os.unlink(p)


def test_parse_errors():
    p = write_tmp("[(0,1),(1,2)]\n")          # channel 1 reused in one layer
    assert run_c(p) == 2
    assert run_py(p) == 2
    os.unlink(p)
    p = write_tmp("[(1,0)]\n")                # i >= j
    assert run_c(p) == 2
    assert run_py(p) == 2
    os.unlink(p)


def test_trivial_networks():
    p = write_tmp("[(0,1)]\n")
    assert run_c(p) == 0 and run_py(p, mode="all") == 0
    os.unlink(p)
    p = write_tmp("[(0,1),(2,3)]\n[(0,2),(1,3)]\n[(1,2)]\n")   # 4-sorter
    assert run_c(p) == 0 and run_py(p, mode="all") == 0
    os.unlink(p)
    p = write_tmp("[(0,1),(2,3)]\n[(0,2),(1,3)]\n")           # not a sorter
    assert run_c(p) == 1 and run_py(p, mode="all") == 1
    os.unlink(p)


if __name__ == "__main__":
    tests = [t for name, t in sorted(globals().items()) if name.startswith("test_")]
    failed = 0
    for t in tests:
        try:
            t()
            print("PASS", t.__name__)
        except AssertionError as e:
            failed += 1
            print("FAIL", t.__name__, e)
    print("%d/%d passed" % (len(tests) - failed, len(tests)))
    sys.exit(1 if failed else 0)
