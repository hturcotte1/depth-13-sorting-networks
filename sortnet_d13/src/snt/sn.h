// sn.h -- core data structures for comparator-network search (no external deps).
// Conventions: channels 0..n-1; comparator (i,j), i<j, puts min on i.
// Binary vectors are uint64_t with bit c = value on channel c.
// Sorted vector with k ones = ((1<<k)-1) << (n-k).
#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using Out = uint64_t;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "CHECK failed: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
      abort();                                                                 \
    }                                                                          \
  } while (0)

inline Out bit(int c) { return Out(1) << c; }

inline std::vector<Out> add_comparator(const std::vector<Out> &outs, int i, int j) {
  std::vector<Out> r;
  r.reserve(outs.size());
  Out m = bit(i) | bit(j);
  for (Out x : outs) {
    if (((x >> i) & 1) > ((x >> j) & 1)) x ^= m;
    r.push_back(x);
  }
  std::sort(r.begin(), r.end());
  r.erase(std::unique(r.begin(), r.end()), r.end());
  return r;
}

inline bool has_inverse(const std::vector<Out> &outs, int i, int j) {
  for (Out x : outs)
    if (((x >> i) & 1) > ((x >> j) & 1)) return true;
  return false;
}

inline Out reflect_invert(int n, Out x) {
  Out r = 0;
  for (int i = 0; i < n; i++) r |= ((x >> i) & 1) << (n - 1 - i);
  return r ^ ((n == 64) ? ~Out(0) : (bit(n) - 1));
}

inline bool is_symmetric_set(int n, const std::vector<Out> &s) {
  for (Out x : s)
    if (!std::binary_search(s.begin(), s.end(), reflect_invert(n, x))) return false;
  return true;
}

struct Net {
  int n = 0;
  std::vector<std::vector<int>> layers;  // matching: layers[l][i] = j or -1
  std::vector<Out> outputs;              // sorted, unique; empty = not computed
  Net() {}
  Net(int n_, int d) : n(n_), layers(d, std::vector<int>(n_, -1)) {}
  int depth() const { return (int)layers.size(); }
  int size() const {
    int s = 0;
    for (auto &L : layers)
      for (int i = 0; i < n; i++)
        if (L[i] > i) s++;
    return s;
  }
  void add_layer() { layers.emplace_back(n, -1); }
  void add_comp(int i, int j) {
    CHECK(i < j);
    CHECK(layers.back()[i] == -1 && layers.back()[j] == -1);
    layers.back()[i] = j;
    layers.back()[j] = i;
    if (!outputs.empty()) outputs = add_comparator(outputs, i, j);
  }
  bool is_symmetric() const {
    for (auto &L : layers)
      for (int i = 0; i < n; i++) {
        int j = L[i];
        int rj = (j == -1) ? -1 : n - 1 - j;
        if (L[n - 1 - i] != rj) return false;
      }
    return true;
  }
  bool has_inv(int i, int j) const { return has_inverse(outputs, i, j); }
  bool is_sorting() const {
    if ((int)outputs.size() != n + 1) return false;
    for (int k = 0; k <= n; k++)
      if (outputs[k] != ((k == 0) ? 0 : ((bit(k) - 1) << (n - k)))) return false;
    return true;
  }
  std::string layer_string(int l) const {
    std::string s = "[";
    bool first = true;
    for (int i = 0; i < n; i++)
      if (layers[l][i] > i) {
        if (!first) s += ",";
        first = false;
        s += "(" + std::to_string(i) + "," + std::to_string(layers[l][i]) + ")";
      }
    return s + "]";
  }
  std::string to_string() const {  // one line, layers separated by commas
    std::string s;
    for (int l = 0; l < depth(); l++) {
      if (l) s += ",";
      s += layer_string(l);
    }
    return s;
  }
  std::string to_multiline() const {
    std::string s;
    for (int l = 0; l < depth(); l++) s += layer_string(l) + "\n";
    return s;
  }
};

// Sparse output-set computation: product construction for layer 0, then apply.
inline std::vector<Out> compute_outputs(const Net &net) {
  int n = net.n;
  std::vector<Out> outs = {0};
  std::vector<bool> touched(n, false);
  if (!net.layers.empty()) {
    for (int i = 0; i < n; i++) {
      int j = net.layers[0][i];
      if (j > i) {
        touched[i] = touched[j] = true;
        std::vector<Out> nx;
        nx.reserve(outs.size() * 3);
        for (Out x : outs) {
          nx.push_back(x);
          nx.push_back(x | bit(j));
          nx.push_back(x | bit(i) | bit(j));
        }
        outs.swap(nx);
      }
    }
  }
  for (int c = 0; c < n; c++)
    if (!touched[c]) {
      std::vector<Out> nx;
      nx.reserve(outs.size() * 2);
      for (Out x : outs) {
        nx.push_back(x);
        nx.push_back(x | bit(c));
      }
      outs.swap(nx);
    }
  std::sort(outs.begin(), outs.end());
  for (int l = 1; l < net.depth(); l++)
    for (int i = 0; i < n; i++) {
      int j = net.layers[l][i];
      if (j > i) outs = add_comparator(outs, i, j);
    }
  return outs;
}

// Parse "[(0,1),(2,3)],[(0,2)]" (or with whitespace / newlines between layers).
inline Net parse_net(int n, const std::string &text) {
  Net net(n, 0);
  size_t p = 0;
  bool in = false;
  while (p < text.size()) {
    char c = text[p];
    if (c == '[') {
      CHECK(!in);
      in = true;
      net.add_layer();
      p++;
    } else if (c == ']') {
      CHECK(in);
      in = false;
      p++;
    } else if (c == '(') {
      CHECK(in);
      int i = -1, j = -1;
      int consumed = 0;
      CHECK(sscanf(text.c_str() + p, "(%d,%d)%n", &i, &j, &consumed) == 2);
      CHECK(0 <= i && i < j && j < n);
      net.add_comp(i, j);
      p += consumed;
    } else if (c == '#') {
      break;
    } else {
      p++;
    }
  }
  CHECK(!in);
  // drop empty layers
  std::vector<std::vector<int>> L;
  for (auto &l : net.layers)
    if (std::any_of(l.begin(), l.end(), [](int v) { return v != -1; })) L.push_back(l);
  net.layers = L;
  return net;
}

// Library file: '#' comments; each non-comment line is one network. First
// comment line may be "# n=14". Outputs are recomputed on load if requested.
inline std::vector<Net> load_nets(const std::string &path, int n, bool fill_outputs) {
  std::ifstream f(path);
  CHECK(f.is_open());
  std::vector<Net> nets;
  std::string line;
  while (std::getline(f, line)) {
    size_t h = line.find('#');
    std::string body = (h == std::string::npos) ? line : line.substr(0, h);
    if (body.find('[') == std::string::npos) {
      if (line.rfind("# n=", 0) == 0 && n == 0) n = atoi(line.c_str() + 4);
      continue;
    }
    CHECK(n > 0);
    nets.push_back(parse_net(n, body));
  }
  if (fill_outputs)
    for (auto &net : nets) net.outputs = compute_outputs(net);
  return nets;
}

// Multi-line single network (Dobbelaere style: one layer per line).
inline Net load_multiline(const std::string &path, int n) {
  std::ifstream f(path);
  CHECK(f.is_open());
  std::stringstream ss;
  std::string line;
  while (std::getline(f, line)) {
    size_t h = line.find('#');
    if (h != std::string::npos) line = line.substr(0, h);
    ss << line << "\n";
  }
  std::string all = ss.str();
  if (n == 0) {
    // infer n
    int mx = -1;
    for (size_t p = 0; p < all.size(); p++)
      if (all[p] == '(') {
        int i, j;
        if (sscanf(all.c_str() + p, "(%d,%d)", &i, &j) == 2) mx = std::max(mx, std::max(i, j));
      }
    n = mx + 1;
  }
  return parse_net(n, all);
}

inline void save_nets(const std::vector<Net> &nets, const std::string &path, const std::string &header = "") {
  std::ofstream f(path);
  CHECK(f.is_open());
  if (!nets.empty()) f << "# n=" << nets[0].n << "\n";
  if (!header.empty()) f << "# " << header << "\n";
  for (auto &net : nets) {
    f << net.to_string();
    if (!net.outputs.empty()) f << "  # out=" << net.outputs.size();
    f << "\n";
  }
}

// perm[i] = new position of channel i.
inline std::vector<Out> permute_outputs(const std::vector<Out> &s, const std::vector<int> &perm) {
  int n = perm.size();
  std::vector<Out> r;
  r.reserve(s.size());
  for (Out x : s) {
    Out y = 0;
    for (int i = 0; i < n; i++) y |= ((x >> i) & 1) << perm[i];
    r.push_back(y);
  }
  std::sort(r.begin(), r.end());
  return r;
}

// Relabel channels: channel i becomes perm[i]. If a comparator would become
// reversed (min on the higher channel), it is written standard and the two
// labels are swapped for all later comparators (Knuth's untangling, TAOCP
// 5.3.4 ex. 16). For a suffix applied after a prefix this preserves sorting of
// output(prefix) because that set contains every sorted vector, forcing the
// final relabeling to be the identity.
inline Net permute_net(const Net &net, std::vector<int> perm, int *num_untangled = nullptr) {
  Net r(net.n, net.depth());
  int cnt = 0;
  for (int l = 0; l < net.depth(); l++)
    for (int i = 0; i < net.n; i++) {
      int j = net.layers[l][i];
      if (j > i) {
        int a = perm[i], b = perm[j];
        if (a > b) {
          std::swap(a, b);
          std::swap(perm[i], perm[j]);
          cnt++;
        }
        r.layers[l][a] = b;
        r.layers[l][b] = a;
      }
    }
  if (num_untangled) *num_untangled = cnt;
  return r;
}

inline std::vector<int> inverse_perm(const std::vector<int> &p) {
  std::vector<int> q(p.size());
  for (size_t i = 0; i < p.size(); i++) q[p[i]] = i;
  return q;
}

inline void window_stats(int n, const std::vector<Out> &outs, long long *sum, int *mx) {
  long long s = 0;
  int m = 0;
  for (Out x : outs) {
    int lead0 = 0;
    while (lead0 < n && !((x >> lead0) & 1)) lead0++;
    int trail1 = 0;
    while (trail1 < n && ((x >> (n - 1 - trail1)) & 1)) trail1++;
    int w = n - lead0 - trail1;
    if (w < 0) w = 0;
    s += w;
    m = std::max(m, w);
  }
  if (sum) *sum = s;
  if (mx) *mx = m;
}
