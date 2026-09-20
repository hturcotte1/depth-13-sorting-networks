#!/usr/bin/env python3
"""
nettools.py -- independent Python utilities for comparator networks
(no dependency on the C++ pipeline).

  nettools.py info     NET.txt                    size/depth/symmetry/output-set sizes
  nettools.py simplify NET.txt -o OUT.txt         strip unused comparators, re-layer ASAP
  nettools.py delete   NET.txt --channel K -o OUT delete channel K (network on n-1 channels)
  nettools.py json     NET.txt -o OUT.json        Dobbelaere-format text -> JSON
  nettools.py oneline  NET.txt                    print one-line form

Network files: Dobbelaere format (one layer per line, '#' comments allowed).
"""
import argparse
import json
import re
import sys

import numpy as np


def parse(text):
    text = re.sub(r"#[^\n]*", "", text)
    layers = []
    for m in re.finditer(r"\[([^\]]*)\]", text):
        pairs = [(int(a), int(b)) for a, b in re.findall(r"\(\s*(\d+)\s*,\s*(\d+)\s*\)", m.group(1))]
        if pairs:
            for i, j in pairs:
                assert i < j, (i, j)
            used = set()
            for i, j in pairs:
                assert i not in used and j not in used, "channel reused in layer"
                used.update((i, j))
            layers.append(pairs)
    return layers


def fmt(layers):
    return "\n".join("[" + ",".join("(%d,%d)" % c for c in L) + "]" for L in layers) + "\n"


def n_of(layers):
    return 1 + max(max(i, j) for L in layers for i, j in L)


def output_set_after_first_layer(n, first):
    s = np.zeros(1, dtype=np.uint64)
    touched = set()
    for i, j in first:
        touched.update((i, j))
        s = np.concatenate([s, s | np.uint64(1 << j), s | np.uint64((1 << i) | (1 << j))])
    for c in range(n):
        if c not in touched:
            s = np.concatenate([s, s | np.uint64(1 << c)])
    return np.unique(s)


def apply_comparator(s, i, j):
    one = np.uint64(1)
    bi = (s >> np.uint64(i)) & one
    bj = (s >> np.uint64(j)) & one
    swap = bi > bj
    if swap.any():
        s = np.unique(np.where(swap, s ^ np.uint64((1 << i) | (1 << j)), s))
    return s, bool(swap.any())


def is_sorted_set(n, s):
    targets = set(((1 << k) - 1) << (n - k) for k in range(n + 1))
    return set(int(v) for v in s) == targets


def simplify(layers, n=None):
    """Strip comparators that never act (no inversion on any reachable vector),
    then re-layer as early as possible. Returns (layers, sorted_flag)."""
    n = n or n_of(layers)
    kept = []
    s = output_set_after_first_layer(n, layers[0])
    kept.extend(layers[0])
    for L in layers[1:]:
        for i, j in L:
            s2, acted = apply_comparator(s, i, j)
            if acted:
                kept.append((i, j))
                s = s2
    last = [-1] * n
    out = []
    for i, j in kept:
        l = max(last[i], last[j]) + 1
        while len(out) <= l:
            out.append([])
        out[l].append((i, j))
        last[i] = last[j] = l
    return out, is_sorted_set(n, s)


def delete_channel(layers, k):
    """Delete channel k: drop comparators touching it, renumber channels above k."""
    out = []
    for L in layers:
        nl = []
        for i, j in L:
            if i == k or j == k:
                continue
            nl.append((i - (i > k), j - (j > k)))
        if nl:
            out.append(nl)
    return out


def info(layers, n=None):
    n = n or n_of(layers)
    s = output_set_after_first_layer(n, layers[0])
    sizes = [len(s)]
    for L in layers[1:]:
        for i, j in L:
            s, _ = apply_comparator(s, i, j)
        sizes.append(len(s))
    sym = all(
        set((n - 1 - j, n - 1 - i) for i, j in L) == set(L) for L in layers
    )
    return {"n": n, "size": sum(len(L) for L in layers), "depth": len(layers), "symmetric": sym,
            "output_sizes": sizes, "sorts": is_sorted_set(n, s)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd", choices=["info", "simplify", "delete", "json", "oneline"])
    ap.add_argument("file")
    ap.add_argument("-o", "--out")
    ap.add_argument("-n", type=int)
    ap.add_argument("--channel", type=int)
    a = ap.parse_args()
    layers = parse(open(a.file).read())
    n = a.n or n_of(layers)
    if a.cmd == "info":
        print(json.dumps(info(layers, n)))
    elif a.cmd == "oneline":
        print(",".join("[" + ",".join("(%d,%d)" % c for c in L) + "]" for L in layers))
    elif a.cmd == "simplify":
        out, ok = simplify(layers, n)
        i = info(out, n)
        print("simplified: n=%d size=%d depth=%d sorts=%s (was size=%d depth=%d)" % (n, i["size"], i["depth"], i["sorts"], sum(len(L) for L in layers), len(layers)))
        text = "# n=%d size=%d depth=%d\n" % (n, i["size"], i["depth"]) + fmt(out)
        if a.out:
            open(a.out, "w").write(text)
        else:
            sys.stdout.write(text)
    elif a.cmd == "delete":
        out = delete_channel(layers, a.channel)
        out, ok = simplify(out, n - 1)
        i = info(out, n - 1)
        print("deleted channel %d: n=%d size=%d depth=%d sorts=%s" % (a.channel, n - 1, i["size"], i["depth"], i["sorts"]))
        text = "# n=%d size=%d depth=%d (channel %d deleted)\n" % (n - 1, i["size"], i["depth"], a.channel) + fmt(out)
        if a.out:
            open(a.out, "w").write(text)
        else:
            sys.stdout.write(text)
    elif a.cmd == "json":
        i = info(layers, n)
        d = {"n": n, "size": i["size"], "depth": i["depth"], "symmetric": i["symmetric"], "layers": [[list(c) for c in L] for L in layers]}
        text = json.dumps(d)
        if a.out:
            open(a.out, "w").write(text + "\n")
        else:
            print(text)


if __name__ == "__main__":
    main()
