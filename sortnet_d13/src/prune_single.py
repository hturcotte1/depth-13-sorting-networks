#!/usr/bin/env python3
"""For each comparator of a sorting network, test whether deleting it leaves a sorting network
(output-set method, exact). Reports removable comparators and, greedily, a smaller network."""
import sys, json
sys.path.insert(0, 'src')
from nettools import parse, fmt, info, simplify, output_set_after_first_layer, apply_comparator, is_sorted_set

def sorts(n, layers):
    s = output_set_after_first_layer(n, layers[0])
    for L in layers[1:]:
        for i, j in L:
            s, _ = apply_comparator(s, i, j)
    return is_sorted_set(n, s)

def try_all(n, layers):
    removable = []
    for li, L in enumerate(layers):
        for ci, c in enumerate(L):
            cand = [list(x) for x in layers]
            del cand[li][ci]
            cand = [x for x in cand if x]
            if sorts(n, cand):
                removable.append((li, c))
    return removable

def greedy(n, layers):
    layers = [list(x) for x in layers]
    while True:
        rem = try_all(n, layers)
        if not rem:
            return layers
        li, c = rem[0]
        layers[li].remove(c)
        layers = [x for x in layers if x]
        print("  removed layer %d comparator %s -> size %d" % (li + 1, c, sum(len(x) for x in layers)))

if __name__ == "__main__":
    src = sys.argv[1]
    if src.endswith('.json'):
        key = sys.argv[2]
        v = json.load(open(src))[key]; n = v['n']; layers = [[tuple(c) for c in L] for L in v['layers']]
    else:
        layers = parse(open(src).read()); n = 1 + max(max(c) for L in layers for c in L)
    size = sum(len(L) for L in layers)
    print("%s: n=%d size=%d depth=%d sorts=%s" % (sys.argv[-1], n, size, len(layers), sorts(n, layers)))
    rem = try_all(n, layers)
    print("  single-comparator removals that keep it sorting: %d %s" % (len(rem), rem[:10]))
    if rem:
        g = greedy(n, layers)
        print("  greedy result: size %d depth %d" % (sum(len(L) for L in g), len(g)))
        out = sys.argv[-1] + "_pruned.txt" if not src.endswith('.json') else "runs/pruned_%s.txt" % key
        open(out, 'w').write(fmt(g)); print("  written", out)
