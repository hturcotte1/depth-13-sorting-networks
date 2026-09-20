// cnf.cc -- SAT encoding of the remaining layers (port of Wang 2025's
// sat_generate_cnf_main.cc, which implements the CCEMS 2019 improvements),
// and decoding of solutions back into networks.
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

#include "cnf.h"
#include "iso.h"
#include "sn.h"

// a -> (b <-> (c v d))
static void a_implies_b_eq_c_or_d(Cnf &f, int a, int b, int c, int d) {
  f.add({-a, b, -c});
  f.add({-a, b, -d});
  f.add({-a, -b, c, d});
}
// a -> (b <-> (c ^ d))
static void a_implies_b_eq_c_and_d(Cnf &f, int a, int b, int c, int d) {
  f.add({-a, -b, c});
  f.add({-a, -b, d});
  f.add({-a, b, -c, -d});
}

// Build the CNF for a suffix of d layers on n channels that must sort every
// vector in `outs` (given in the permuted channel order).
// last_span: allowed spans for the last layers (default Wang/CCEMS: last layer
// span 1, second-to-last span <= 3).
Cnf build_cnf(int n, int d, const std::vector<Out> &outs, bool sym, int subnet_channels) {
  if (sym) CHECK(n % 2 == 0);
  Cnf f;
  const int INVALID = 0;
  int TRUE_ = f.newvar();
  f.add({TRUE_});
  int FALSE_ = f.newvar();
  f.add({-FALSE_});

  // g[k][i][j]
  std::vector<std::vector<std::vector<int>>> g(d, std::vector<std::vector<int>>(n, std::vector<int>(n, INVALID)));
  for (int k = 0; k < d; k++)
    for (int i = 0; i < n; i++)
      for (int j = i + 1; j < n; j++) {
        if (sym) {
          int is = n - 1 - i, js = n - 1 - j;
          if (js < i) {
            g[k][i][j] = g[k][js][is];
            continue;
          }
        }
        g[k][i][j] = f.newvar();
        f.names.push_back({g[k][i][j], "g_" + std::to_string(k) + "_" + std::to_string(i) + "_" + std::to_string(j)});
      }
  // each channel used at most once per layer
  for (int k = 0; k < d; k++)
    for (int i = 0; i < n; i++)
      for (int j0 = 0; j0 < n; j0++) {
        if (j0 == i) continue;
        for (int j1 = j0 + 1; j1 < n; j1++) {
          if (j1 == i) continue;
          int a = g[k][std::min(i, j0)][std::max(i, j0)];
          int b = g[k][std::min(i, j1)][std::max(i, j1)];
          f.add({-a, -b});
        }
      }
  // used[k][i]
  std::vector<std::vector<int>> used(d, std::vector<int>(n, INVALID));
  for (int k = 0; k < d; k++)
    for (int i = 0; i < n; i++) {
      if (sym && n - 1 - i < i) {
        used[k][i] = used[k][n - 1 - i];
        continue;
      }
      used[k][i] = f.newvar();
    }
  for (int k = 0; k < d; k++)
    for (int i = 0; i < n; i++) {
      std::vector<int> lits;
      for (int j = 0; j < n; j++) {
        if (i < j) lits.push_back(g[k][i][j]);
        if (i > j) lits.push_back(g[k][j][i]);
      }
      f.iff_or(used[k][i], lits);
    }
  // one_down[k][i][j]: exists g[k][i][l], i<l<=j ; one_up[k][i][j]: exists g[k][l][j], i<=l<j
  std::vector<std::vector<std::vector<int>>> one_down(d, std::vector<std::vector<int>>(n, std::vector<int>(n, INVALID)));
  std::vector<std::vector<std::vector<int>>> one_up = one_down;
  for (int k = 0; k < d; k++)
    for (int i = 0; i < n; i++)
      for (int j = i; j < n; j++) {
        one_down[k][i][j] = f.newvar();
        std::vector<int> lits;
        for (int l = i + 1; l <= j; l++) lits.push_back(g[k][i][l]);
        f.iff_or(one_down[k][i][j], lits);
        if (sym) {
          one_up[k][n - 1 - j][n - 1 - i] = one_down[k][i][j];
          continue;
        }
        one_up[k][i][j] = f.newvar();
        lits.clear();
        for (int l = i; l < j; l++) lits.push_back(g[k][l][j]);
        f.iff_or(one_up[k][i][j], lits);
      }
  // last layer: only (i,i+1)
  for (int i = 0; i < n; i++)
    for (int j = i + 2; j < n; j++) f.add({-g[d - 1][i][j]});
  if (d >= 2) {
    // second-to-last: span <= 3
    for (int i = 0; i < n; i++)
      for (int j = i + 4; j < n; j++) f.add({-g[d - 2][i][j]});
    // (i,i+3) in layer d-2 implies (i,i+1) and (i+2,i+3) in layer d-1
    for (int i = 0; i + 3 < n; i++) {
      f.add({-g[d - 2][i][i + 3], g[d - 1][i][i + 1]});
      f.add({-g[d - 2][i][i + 3], g[d - 1][i + 2][i + 3]});
    }
    // (i,i+2) in layer d-2 implies (i,i+1) or (i+1,i+2) in layer d-1
    for (int i = 0; i + 2 < n; i++) f.add({-g[d - 2][i][i + 2], g[d - 1][i][i + 1], g[d - 1][i + 1][i + 2]});
  }
  // no two adjacent unused channels in the last layer
  for (int i = 0; i + 1 < n; i++) f.add({used[d - 1][i], used[d - 1][i + 1]});
  if (d >= 2) {
    for (int i = 0; i + 2 < n; i++) {
      f.add({-g[d - 1][i][i + 1], used[d - 1][i + 2], used[d - 2][i], used[d - 2][i + 1]});
      f.add({-g[d - 1][i + 1][i + 2], used[d - 1][i], used[d - 2][i + 1], used[d - 2][i + 2]});
    }
  }
  // sort each input
  for (size_t m = 0; m < outs.size(); m++) {
    Out x = outs[m];
    int num0 = n - std::popcount(x);
    int lead0 = 0;
    while (lead0 < n && !((x >> lead0) & 1)) lead0++;
    int trail1 = 0;
    while (trail1 < n && ((x >> (n - 1 - trail1)) & 1)) trail1++;
    int cb = lead0, ce = n - trail1;
    if (ce <= cb) continue;  // already sorted
    if (subnet_channels >= 0 && ce - cb > subnet_channels) continue;
    std::vector<std::vector<int>> v(d + 1, std::vector<int>(n, INVALID));
    for (int k = 0; k <= d; k++)
      for (int i = 0; i < n; i++) {
        if (i < cb) v[k][i] = FALSE_;
        else if (i < ce) v[k][i] = f.newvar();
        else v[k][i] = TRUE_;
      }
    for (int i = cb; i < ce; i++) f.add({((x >> i) & 1) ? v[0][i] : -v[0][i]});
    for (int k = 0; k < d; k++)
      for (int i = cb; i < ce; i++) {
        f.add({v[k][i], one_up[k][cb][i], -v[k + 1][i]});
        for (int j = cb; j < i; j++) a_implies_b_eq_c_or_d(f, g[k][j][i], v[k + 1][i], v[k][j], v[k][i]);
        f.add({-v[k][i], one_down[k][i][ce - 1], v[k + 1][i]});
        for (int j = i + 1; j < ce; j++) a_implies_b_eq_c_and_d(f, g[k][i][j], v[k + 1][i], v[k][i], v[k][j]);
      }
    for (int i = cb; i < ce; i++) f.add({(i < num0) ? -v[d][i] : v[d][i]});
  }
  for (auto &c : f.clauses)
    for (int l : c) CHECK(l != 0);
  return f;
}

void write_cnf(const Cnf &f, const std::string &path, const std::vector<std::string> &header) {
  FILE *fp = fopen(path.c_str(), "w");
  CHECK(fp);
  for (auto &h : header) fprintf(fp, "c %s\n", h.c_str());
  for (auto &nm : f.names) fprintf(fp, "c var %d : %s\n", nm.first, nm.second.c_str());
  fprintf(fp, "p cnf %d %zu\n", f.nvars, f.clauses.size());
  std::string buf;
  buf.reserve(1 << 20);
  for (auto &c : f.clauses) {
    for (int l : c) {
      buf += std::to_string(l);
      buf += ' ';
    }
    buf += "0\n";
    if (buf.size() > (1 << 20) - 4096) {
      fwrite(buf.data(), 1, buf.size(), fp);
      buf.clear();
    }
  }
  fwrite(buf.data(), 1, buf.size(), fp);
  fclose(fp);
}

// ---- decoding -------------------------------------------------------------


CnfMeta read_cnf_meta(const std::string &path) {
  std::ifstream f(path);
  CHECK(f.is_open());
  CnfMeta m;
  std::string line;
  while (std::getline(f, line)) {
    if (line.rfind("p cnf", 0) == 0) break;
    if (line.rfind("c n ", 0) == 0) m.n = atoi(line.c_str() + 4);
    else if (line.rfind("c sym ", 0) == 0) m.sym = atoi(line.c_str() + 6);
    else if (line.rfind("c depth ", 0) == 0) m.depth = atoi(line.c_str() + 8);
    else if (line.rfind("c prefix_depth ", 0) == 0) m.prefix_depth = atoi(line.c_str() + 15);
    else if (line.rfind("c perm ", 0) == 0) {
      std::istringstream ss(line.substr(7));
      int x;
      while (ss >> x) m.perm.push_back(x);
    } else if (line.rfind("c prefix ", 0) == 0) m.prefix_str = line.substr(9);
    else if (line.rfind("c var ", 0) == 0) {
      int var, k, i, j;
      if (sscanf(line.c_str(), "c var %d : g_%d_%d_%d", &var, &k, &i, &j) == 4) m.var2comp[var] = {k, i, j};
    }
  }
  CHECK(m.n > 0 && m.depth > 0 && !m.prefix_str.empty());
  return m;
}

// returns true literals (positive var ids); empty + sat=false if UNSAT
std::vector<int> read_solution(const std::string &path, bool *sat) {
  std::ifstream f(path);
  CHECK(f.is_open());
  std::string line;
  std::vector<int> lits;
  *sat = false;
  bool minisat = false;
  while (std::getline(f, line)) {
    if (line.empty()) continue;
    if (line == "UNSAT" || line == "s UNSATISFIABLE") return {};
    if (line == "SAT") { *sat = true; minisat = true; continue; }
    if (line == "s SATISFIABLE") { *sat = true; continue; }
    if (line[0] == 'c') continue;
    std::string body = line;
    if (line.rfind("v ", 0) == 0) body = line.substr(2);
    else if (!minisat) continue;
    std::istringstream ss(body);
    int x;
    while (ss >> x)
      if (x > 0) lits.push_back(x);
  }
  return lits;
}

// Strip comparators that never act (first input always <= second) and
// re-layer as early as possible. Returns network with outputs computed.
Net simplify_and_relayer(const Net &net) {
  int n = net.n;
  // sequential strip: keep a comparator iff it has an inversion at its position
  Net stripped(n, 0);
  stripped.outputs.clear();
  std::vector<std::pair<int, int>> kept;  // in order
  std::vector<Out> outs;
  {
    // outputs of empty network = all vectors: use the product trick layer by layer instead
    Net tmp(n, 0);
    for (int l = 0; l < net.depth(); l++) {
      tmp.add_layer();
      if (l == 0) {
        for (int i = 0; i < n; i++)
          if (net.layers[0][i] > i) tmp.layers[0][i] = net.layers[0][i], tmp.layers[0][net.layers[0][i]] = i;
        tmp.outputs = compute_outputs(tmp);
        for (int i = 0; i < n; i++)
          if (net.layers[0][i] > i) kept.push_back({i, net.layers[0][i]});
        continue;
      }
      for (int i = 0; i < n; i++) {
        int j = net.layers[l][i];
        if (j > i && has_inverse(tmp.outputs, i, j)) {
          tmp.add_comp(i, j);
          kept.push_back({i, j});
        }
      }
    }
    outs = tmp.outputs;
  }
  // ASAP re-layering
  std::vector<int> last(n, -1);
  Net r(n, 0);
  for (auto [i, j] : kept) {
    int l = std::max(last[i], last[j]) + 1;
    while (r.depth() <= l) r.add_layer();
    r.layers[l][i] = j;
    r.layers[l][j] = i;
    last[i] = last[j] = l;
  }
  r.outputs = outs;
  return r;
}
