#pragma once
#include <array>
#include <map>
#include <string>
#include <vector>

#include "sn.h"

struct Cnf {
  int nvars = 0;
  std::vector<std::vector<int>> clauses;
  std::vector<std::pair<int, std::string>> names;  // (var, name) for g vars
  int newvar() { return ++nvars; }
  void add(std::vector<int> c) { clauses.push_back(std::move(c)); }
  // x <-> (l1 v l2 v ...)
  void iff_or(int x, const std::vector<int> &lits) {
    std::vector<int> c = {-x};
    for (int l : lits) {
      c.push_back(l);
      add({x, -l});
    }
    add(c);
  }
};

struct CnfMeta {
  int n = 0, depth = 0, prefix_depth = 0;
  bool sym = false;
  bool open_last = false;
  std::vector<int> perm;
  std::string prefix_str;
  std::map<int, std::array<int, 3>> var2comp;
};

// forbid0: channels (in the permuted numbering) that may not be used in suffix layer 0
// (used when the prefix's last layer is only partially filled and SAT may complete it).
// max_comps >= 0: at most that many comparators in the suffix (mirror pairs count 2), via a
// Sinz sequential counter.
// If no_normal_forms is set, only the NECESSARY restrictions are kept (last layer adjacent-only,
// second-to-last span<=3 with its implications: CCEMS Lemma 4/6 for non-redundant networks) and the
// normal-form constraints psi1 (no two adjacent idle channels in the last layer) and psi3 (Lemma 9)
// are dropped. Used as a control for UNSAT verdicts under the reflection-symmetry restriction.
Cnf build_cnf(int n, int d, const std::vector<Out> &outs, bool sym, int subnet_channels,
              const std::vector<int> &forbid0 = {}, bool no_normal_forms = false, int max_comps = -1);
void write_cnf(const Cnf &f, const std::string &path, const std::vector<std::string> &header);
CnfMeta read_cnf_meta(const std::string &path);
std::vector<int> read_solution(const std::string &path, bool *sat);
Net simplify_and_relayer(const Net &net);
