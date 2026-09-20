#!/bin/bash
# run_search.sh -- one reproducible search run: greedy extension -> CNF -> SAT -> decode -> verify.
#
# usage: run_search.sh NAME PREFIX_LIB DEPTH GREEDY_LAYERS KEEP SOLVER TIMEOUT LIMIT [JOBS] [EXTRA_CNF_ARGS...]
#   NAME          run directory name under runs/
#   PREFIX_LIB    prefix library (one network per line, '# n=..' header)
#   DEPTH         target total depth (13)
#   GREEDY_LAYERS number of layers to add greedily one comparator at a time (0, 1 or 2)
#   KEEP          number of prefixes kept after each greedy step (Wang: 64)
#   SOLVER        minisat | kissat | cadical
#   TIMEOUT       per-instance wall-clock budget in seconds
#   LIMIT         number of best prefixes to encode/solve
#   JOBS          parallel solver jobs (default 4)
# Symmetric mode is always on (all our prefixes are reflection-symmetric).
set -u
NAME=$1; LIB=$2; DEPTH=$3; GL=$4; KEEP=$5; SOLVER=$6; TIMEOUT=$7; LIMIT=$8; JOBS=${9:-4}; shift 9 2>/dev/null || shift $#
EXTRA="$*"
HERE=$(cd "$(dirname "$0")/.." && pwd)
SNT=$HERE/src/snt/snt
RUN=$HERE/runs/$NAME
mkdir -p "$RUN"
LOG=$RUN/log.txt
exec > >(tee -a "$LOG") 2>&1
echo "=== run $NAME started $(date -u +%FT%TZ) on $(hostname)"
echo "args: lib=$LIB depth=$DEPTH greedy_layers=$GL keep=$KEEP solver=$SOLVER timeout=$TIMEOUT limit=$LIMIT jobs=$JOBS extra='$EXTRA'"
cp "$LIB" "$RUN/prefix_in.txt"
CUR=$RUN/prefix_in.txt
if [ "$GL" -gt 0 ]; then
  T0=$(date +%s)
  $SNT extend --in "$CUR" --sym --keep "$KEEP" --layers "$GL" --out "$RUN/prefix_greedy.txt" --threads 4
  echo "greedy extension took $(( $(date +%s) - T0 )) s"
  CUR=$RUN/prefix_greedy.txt
fi
$SNT sizes --in "$CUR" --limit "$LIMIT" > "$RUN/prefix_sizes.txt"
echo "output-set sizes of the prefixes to be solved:"; cut -d' ' -f1,2 "$RUN/prefix_sizes.txt" | tr '\n' ';'; echo
T0=$(date +%s)
$SNT cnf --in "$CUR" --sym --depth "$DEPTH" --outdir "$RUN/cnf" --limit "$LIMIT" $EXTRA
echo "cnf generation took $(( $(date +%s) - T0 )) s"
T0=$(date +%s)
python3 "$HERE/src/solve.py" "$RUN/cnf" --solver "$SOLVER" --timeout "$TIMEOUT" --jobs "$JOBS" --decode "$SNT"
echo "solving took $(( $(date +%s) - T0 )) s"
shopt -s nullglob
for f in "$RUN"/cnf/*."$SOLVER".net.txt; do
  echo "--- verifying $f"
  head -1 "$f"
  "$HERE/src/verify_c" "$f" | tail -1
  python3 "$HERE/src/verify_py.py" "$f" | tail -1
done
echo "=== run $NAME finished $(date -u +%FT%TZ)"
python3 - "$RUN" <<'PY'
import glob, json, os, sys
run = sys.argv[1]
rows = []
for j in sorted(glob.glob(os.path.join(run, "cnf", "*.json"))):
    d = json.load(open(j)); rows.append((d["cnf"], d["solver"], d["status"], d["time"]))
sat = sum(1 for r in rows if r[2] == "SAT"); unsat = sum(1 for r in rows if r[2] == "UNSAT"); to = sum(1 for r in rows if r[2] == "TIMEOUT")
print("summary: %d instances: %d SAT, %d UNSAT, %d TIMEOUT" % (len(rows), sat, unsat, to))
for r in rows: print("  %s %s %s %.0fs" % r)
PY
