#!/usr/bin/env python3
"""
solve.py -- run SAT solvers on a directory of CNF files in parallel with a
per-instance time budget, recording results as JSON next to each CNF.

  python3 solve.py CNF_DIR --solver kissat --timeout 1800 --jobs 4 [--limit K] [--files a.cnf b.cnf]

For each XXXX.cnf writes XXXX.<solver>.sol (solver output) and
XXXX.<solver>.json {"status": "SAT"|"UNSAT"|"TIMEOUT"|"ERROR", "time": s, ...}.
Instances already SAT/UNSAT for that solver are skipped unless --force.
With --decode SNT, a SAT instance is decoded immediately (snt decode) into
XXXX.<solver>.net.txt and the summary is printed.
"""
import argparse
import glob
import json
import multiprocessing as mp
import os
import shutil
import subprocess
import sys
import time


def run_one(task):
    cnf, solver, timeout, decode = task
    base = cnf[:-4] if cnf.endswith(".cnf") else cnf
    sol = "%s.%s.sol" % (base, solver)
    js = "%s.%s.json" % (base, solver)
    exe = os.environ.get(solver.upper(), solver)
    if solver == "minisat":
        cmd = [exe, "-cpu-lim=%d" % int(timeout + 5), cnf, sol]
    elif solver == "kissat":
        cmd = [exe, "--time=%d" % int(timeout + 5), "-q", cnf]
    elif solver == "cadical":
        cmd = [exe, "-t", str(int(timeout + 5)), "-q", cnf]
    else:
        raise ValueError(solver)
    t0 = time.time()
    status = "ERROR"
    out = ""
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        out = p.stdout
        rc = p.returncode
        if rc == 10:
            status = "SAT"
        elif rc == 20:
            status = "UNSAT"
        elif rc == 0 and ("s UNKNOWN" in out or "INDETERMINATE" in out):
            status = "TIMEOUT"
        else:
            status = "TIMEOUT" if (time.time() - t0) >= timeout - 1 else "ERROR"
            if status == "ERROR":
                out += "\n" + p.stderr
    except subprocess.TimeoutExpired as e:
        status = "TIMEOUT"
        out = (e.stdout or b"").decode() if isinstance(e.stdout, bytes) else (e.stdout or "")
    elapsed = time.time() - t0
    if solver != "minisat":
        with open(sol, "w") as f:
            f.write(out)
    elif status != "SAT" and status != "UNSAT":
        with open(sol, "w") as f:
            f.write("INDET\n")
    res = {"cnf": os.path.basename(cnf), "solver": solver, "status": status, "time": round(elapsed, 1),
           "timeout": timeout, "host": os.uname().nodename}
    if status == "SAT" and decode:
        net_txt = "%s.%s.net.txt" % (base, solver)
        d = subprocess.run([decode, "decode", "--cnf", cnf, "--sol", sol, "--out", net_txt], capture_output=True, text=True)
        res["decode_rc"] = d.returncode
        res["decode_head"] = "\n".join(d.stdout.splitlines()[:2])
        res["net"] = net_txt
    with open(js, "w") as f:
        json.dump(res, f)
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cnf_dir")
    ap.add_argument("--solver", default="kissat", choices=["minisat", "kissat", "cadical"])
    ap.add_argument("--timeout", type=float, default=1800)
    ap.add_argument("--jobs", type=int, default=max(1, mp.cpu_count()))
    ap.add_argument("--limit", type=int)
    ap.add_argument("--files", nargs="*")
    ap.add_argument("--force", action="store_true")
    ap.add_argument("--decode", help="path to snt binary for immediate decoding")
    ap.add_argument("--stop_on_sat", action="store_true")
    a = ap.parse_args()
    files = a.files or sorted(glob.glob(os.path.join(a.cnf_dir, "*.cnf")))
    if a.limit:
        files = files[: a.limit]
    todo = []
    for f in files:
        base = f[:-4]
        js = "%s.%s.json" % (base, a.solver)
        if os.path.exists(js) and not a.force:
            st = json.load(open(js))["status"]
            if st in ("SAT", "UNSAT") or (st == "TIMEOUT" and json.load(open(js)).get("timeout", 0) >= a.timeout):
                print("skip %s (%s)" % (os.path.basename(f), st))
                continue
        todo.append((f, a.solver, a.timeout, a.decode))
    print("solving %d instances with %s, timeout %.0fs, %d jobs" % (len(todo), a.solver, a.timeout, a.jobs), flush=True)
    t0 = time.time()
    n_sat = 0
    with mp.Pool(a.jobs) as pool:
        for r in pool.imap_unordered(run_one, todo):
            print("%s %s %s %.1fs %s" % (time.strftime("%H:%M:%S"), r["cnf"], r["status"], r["time"], r.get("decode_head", "")), flush=True)
            if r["status"] == "SAT":
                n_sat += 1
                if a.stop_on_sat:
                    pool.terminate()
                    break
    print("done: %d SAT of %d in %.0fs wall" % (n_sat, len(todo), time.time() - t0))


if __name__ == "__main__":
    main()
