#!/usr/bin/env python3
"""Build seed prefixes (library format, one network per line) for 16/28/30/32 channels:
hypercube layers, Van Voorhis 16-channel 5-layer prefix, stacked/mirrored/deleted variants,
and the first k layers of Dobbelaere's best networks."""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__))
from nettools import parse, delete_channel

def oneline(layers): return ",".join("[" + ",".join("(%d,%d)" % c for c in L) + "]" for L in layers)

def hypercube(k):
    n = 1 << k
    return [[(i, i | (1 << b)) for i in range(n) if not (i >> b) & 1] for b in range(k)]

def vv16():
    L = hypercube(4)
    L.append([(1,2),(4,8),(3,12),(5,10),(6,9),(7,11),(13,14)])
    return L

def stack_nested(A, na, B, nb):  # A outer symmetric block, B inner
    n = na + nb
    pa = lambda i: i if i < na // 2 else i + nb
    pb = lambda i: i + na // 2
    d = max(len(A), len(B)); out = [[] for _ in range(d)]
    for l, L in enumerate(A): out[l] += [tuple(sorted((pa(i), pa(j)))) for i, j in L]
    for l, L in enumerate(B): out[l] += [tuple(sorted((pb(i), pb(j)))) for i, j in L]
    return [sorted(L) for L in out]

def stack_mirrored(A, na):
    n = 2 * na
    return [sorted(L + [(n - 1 - j, n - 1 - i) for i, j in L]) for L in A]

def side_by_side(A, na, B, nb):
    d = max(len(A), len(B)); out = [[] for _ in range(d)]
    for l, L in enumerate(A): out[l] += [tuple(c) for c in L]
    for l, L in enumerate(B): out[l] += [(i + na, j + na) for i, j in L]
    return [sorted(L) for L in out]

nets = json.load(open("data/dobbelaere_networks.json"))
for v in nets.values(): v["layers"] = [[tuple(c) for c in L] for L in v["layers"]]
lib = {}
lib["n16_vv5"] = vv16()
lib["n16_green5"] = [[tuple(c) for c in L] for L in nets["N16L61D9"]["layers"][:5]]
lib["n16_60d10_5"] = nets["N16L60D10"]["layers"][:5]
lib["n16_cube4"] = hypercube(4)
lib["n32_cube5"] = hypercube(5)
lib["n32_vv_nested"] = stack_nested(vv16(), 16, vv16(), 16)
lib["n32_vv_mirrored"] = stack_mirrored(vv16(), 16)
lib["n32_green_side"] = side_by_side(nets["N16L61D9"]["layers"][:5], 16, nets["N16L61D9"]["layers"][:5], 16)
lib["n32_185d14_first5"] = nets["N32L185D14"]["layers"][:5]
lib["n30_cube5_del_0_31"] = delete_channel(delete_channel(hypercube(5), 31), 0)
lib["n30_172d14_first5"] = nets["N30L172D14"]["layers"][:5]
lib["n30_vv_nested_del"] = delete_channel(delete_channel(stack_nested(vv16(), 16, vv16(), 16), 31), 0)
lib["n28_cube5_del_0_1_30_31"] = delete_channel(delete_channel(delete_channel(delete_channel(hypercube(5), 31), 30), 1), 0)
lib["n28_wang_first5"] = parse(open("data/wang_n28d13.txt").read())[:5]
lib["n28_wang_first6"] = parse(open("data/wang_n28d13.txt").read())[:6]
lib["n14_cube_del"] = delete_channel(delete_channel(hypercube(4), 15), 0)
lib["n14_52d9_first5"] = nets["N14L52D9"]["layers"][:5]
lib["n15_57d9_first5"] = nets["N15L57D9"]["layers"][:5]
for k, L in lib.items():
    n = int(k.split("_")[0][1:])
    with open("data/prefixes/%s.txt" % k, "w") as f:
        f.write("# n=%d\n%s\n" % (n, oneline(L)))
    print(k, "n=%d depth=%d size=%d" % (n, len(L), sum(len(l) for l in L)))
