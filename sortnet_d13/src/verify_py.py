#!/usr/bin/env python3
"""
verify_py.py -- independent Python verifier for comparator networks.

Reads a network in Dobbelaere's bracket format (layers in "[...]", comparators
"(i,j)" with i<j; '#' starts a comment; layers may be one per line or several
per line) and decides whether it sorts every binary input (zero-one principle).

Two independent methods are implemented:

  exhaustive : run all 2^n binary inputs through the network in numpy chunks
               and check each output is of the form 0...01...1
               (default for n <= 26).
  outputset  : compute the set of reachable binary vectors layer by layer as
               a set of integers (numpy uint64 with np.unique) and check the
               final set is exactly the n+1 sorted vectors
               (default for n > 26; works for any n <= 62 whose first layer
               has enough comparators to keep the set manageable).
  pyset      : pure-Python exhaustive check with int sets, no numpy
               (n <= 20 only; slow but dependency free).

Convention: comparator (i,j), i<j, puts the minimum on channel i.
Bit c of an integer = value on channel c. Sorted vector with k ones is
((1<<k)-1) << (n-k).

Usage: verify_py.py [--mode exhaustive|outputset|pyset|all] [-n N] file
Exit:  0 sorts, 1 does not sort, 2 parse/usage error.
"""
import argparse
import re
import sys


def parse_network(text):
    """Return list of layers; each layer is a list of (i,j) tuples. Raises ValueError."""
    layers = []
    text = re.sub(r"#[^\n]*", "", text)
    pos = 0
    depth_stack = 0
    cur = None
    while pos < len(text):
        ch = text[pos]
        if ch == "[":
            if cur is not None:
                raise ValueError("nested '['")
            cur = []
            pos += 1
        elif ch == "]":
            if cur is None:
                raise ValueError("stray ']'")
            if cur:
                layers.append(cur)
            cur = None
            pos += 1
        elif ch == "(":
            m = re.match(r"\(\s*(\d+)\s*,\s*(\d+)\s*\)", text[pos:])
            if not m:
                raise ValueError("bad comparator at offset %d" % pos)
            if cur is None:
                raise ValueError("comparator outside a layer")
            i, j = int(m.group(1)), int(m.group(2))
            if i >= j:
                raise ValueError("comparator (%d,%d) must have i<j" % (i, j))
            cur.append((i, j))
            pos += m.end()
        elif ch in " \t\r\n,":
            pos += 1
        else:
            raise ValueError("unexpected character %r at offset %d" % (ch, pos))
    if cur is not None:
        raise ValueError("unterminated layer")
    # each channel at most once per layer
    for li, layer in enumerate(layers):
        used = set()
        for i, j in layer:
            if i in used or j in used:
                raise ValueError("layer %d reuses a channel in (%d,%d)" % (li + 1, i, j))
            used.add(i)
            used.add(j)
    return layers


def sorted_targets(n):
    return set((((1 << k) - 1) << (n - k)) for k in range(n + 1))


def check_pyset(n, layers):
    if n > 20:
        raise ValueError("pyset mode limited to n<=20")
    targets = sorted_targets(n)
    comps = [c for L in layers for c in L]
    for x in range(1 << n):
        y = x
        for i, j in comps:
            bi = (y >> i) & 1
            bj = (y >> j) & 1
            if bi > bj:
                y ^= (1 << i) | (1 << j)
        if y not in targets:
            return False, x
    return True, None


def check_exhaustive(n, layers):
    import numpy as np
    comps = [c for L in layers for c in L]
    chunk_bits = min(n, 22)
    chunk = 1 << chunk_bits
    base = np.arange(chunk, dtype=np.uint64)
    m = np.uint64((1 << n) - 1)
    one = np.uint64(1)
    for hi in range(1 << (n - chunk_bits)):
        x = base | np.uint64(hi << chunk_bits)
        for i, j in comps:
            bi = (x >> np.uint64(i)) & one
            bj = (x >> np.uint64(j)) & one
            swap = (bi > bj)
            if swap.any():
                x = np.where(swap, x ^ np.uint64((1 << i) | (1 << j)), x)
        y = m ^ x                     # sorted iff y is of the form 2^t - 1
        bad = (y & (y + one)) != 0
        if bad.any():
            idx = int(np.argmax(bad))
            return False, int((hi << chunk_bits) + idx)
    return True, None


def check_outputset(n, layers):
    import numpy as np
    one = np.uint64(1)
    # outputs after layer 1: product of {00,01,11} per comparator and {0,1} per free channel
    s = np.zeros(1, dtype=np.uint64)
    first = layers[0]
    touched = set()
    for i, j in first:
        touched.add(i); touched.add(j)
        s = np.concatenate([s, s | np.uint64(1 << j), s | np.uint64((1 << i) | (1 << j))])
    for c in range(n):
        if c not in touched:
            s = np.concatenate([s, s | np.uint64(1 << c)])
    s = np.unique(s)
    sizes = [len(s)]
    for L in layers[1:]:
        for i, j in L:
            bi = (s >> np.uint64(i)) & one
            bj = (s >> np.uint64(j)) & one
            swap = bi > bj
            if swap.any():
                s = np.where(swap, s ^ np.uint64((1 << i) | (1 << j)), s)
        s = np.unique(s)
        sizes.append(len(s))
    final = set(int(v) for v in s)
    ok = (final == sorted_targets(n))
    return ok, sizes, final


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("-n", type=int, default=0)
    ap.add_argument("--mode", choices=["exhaustive", "outputset", "pyset", "all", "auto"], default="auto")
    ap.add_argument("-q", action="store_true")
    a = ap.parse_args()
    try:
        layers = parse_network(open(a.file).read())
    except (OSError, ValueError) as e:
        print("PARSE ERROR:", e)
        sys.exit(2)
    if not layers:
        print("PARSE ERROR: no comparators")
        sys.exit(2)
    maxch = max(max(i, j) for L in layers for i, j in L)
    n = a.n or (maxch + 1)
    if n <= maxch:
        print("PARSE ERROR: -n too small")
        sys.exit(2)
    size = sum(len(L) for L in layers)
    depth = len(layers)
    modes = []
    if a.mode == "auto":
        modes = ["exhaustive"] if n <= 26 else ["outputset"]
    elif a.mode == "all":
        modes = ["exhaustive", "outputset"] + (["pyset"] if n <= 20 else [])
    else:
        modes = [a.mode]
    if not a.q:
        print("file=%s n=%d size=%d depth=%d modes=%s" % (a.file, n, size, depth, ",".join(modes)))
    all_ok = True
    for mode in modes:
        if mode == "exhaustive":
            ok, bad = check_exhaustive(n, layers)
            print("  exhaustive: %s%s" % ("SORTS" if ok else "FAILS", "" if ok else " (first failing input 0x%x)" % bad))
        elif mode == "pyset":
            ok, bad = check_pyset(n, layers)
            print("  pyset:      %s%s" % ("SORTS" if ok else "FAILS", "" if ok else " (first failing input 0x%x)" % bad))
        else:
            ok, sizes, final = check_outputset(n, layers)
            print("  outputset:  %s (|output| per layer: %s)" % ("SORTS" if ok else "FAILS", sizes))
        all_ok = all_ok and ok
    print("RESULT: %s (n=%d, size=%d, depth=%d)" % ("SORTS all 2^%d binary inputs" % n if all_ok else "DOES NOT SORT", n, size, depth))
    sys.exit(0 if all_ok else 1)


if __name__ == "__main__":
    main()
