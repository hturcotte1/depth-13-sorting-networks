// snt.cc -- command-line driver.
//   snt gen     --n N [--sym] --depth D [--in lib --in_depth d] --out lib [--keep k2,k3,...] [--threads T] [--chunk C]
//   snt stack   --a libA --na NA --b libB --nb NB --mode nested|mirrored --out lib
//   snt extend  --in lib --out lib [--sym] --keep K [--layers L] [--threads T] [--limit M]
//   snt cnf     --in lib --depth D [--sym] --outdir dir [--limit L] [--no-window] [--subnet S]
//   snt decode  --cnf file.cnf --sol file.sol [--out net.txt] [--raw]
//   snt info    --net file [--multiline] [-n N]
//   snt sizes   --in lib   (print output-set sizes)
#include <chrono>
#include <cstring>
#include <filesystem>
#include <map>
#include <thread>

#include "iso.h"
#include "sn.h"

std::vector<Net> extend_nets(const std::vector<Net> &nets, bool sym, bool one_at_a_time, int keep_best, std::mt19937 &gen, int threads, int chunk);
std::vector<Net> first_layers(int n, bool sym);
Net stack_nets(const Net &a, const Net &b, const std::string &mode);
#include "cnf.h"

static std::map<std::string, std::string> args;
static std::string arg(const std::string &k, const std::string &def = "") { return args.count(k) ? args[k] : def; }
static int argi(const std::string &k, int def) { return args.count(k) ? atoi(args[k].c_str()) : def; }
static bool flag(const std::string &k) { return args.count(k); }

static double now() {
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

static std::vector<int> parse_keep(const std::string &s, int count) {
  std::vector<int> r;
  if (s.empty()) return std::vector<int>(count, 0);
  std::stringstream ss(s);
  std::string tok;
  while (std::getline(ss, tok, ',')) r.push_back(tok.empty() ? 0 : atoi(tok.c_str()));
  while ((int)r.size() < count) r.push_back(0);
  CHECK((int)r.size() == count);
  return r;
}

int cmd_gen() {
  int n = argi("n", 0);
  bool sym = flag("sym");
  int depth = argi("depth", 0);
  int threads = argi("threads", std::thread::hardware_concurrency());
  int chunk = argi("chunk", 2000);
  std::mt19937 gen(argi("seed", 1));
  CHECK(n > 0 && depth > 0);
  std::vector<Net> nets;
  int in_depth = 1;
  if (arg("in").empty()) {
    nets = first_layers(n, sym);
  } else {
    nets = load_nets(arg("in"), n, true);
    in_depth = nets[0].depth();
  }
  std::vector<int> keep = parse_keep(arg("keep"), depth - in_depth);
  fprintf(stderr, "gen: n=%d sym=%d start depth %d with %zu networks\n", n, sym, in_depth, nets.size());
  for (int d = in_depth; d < depth; d++) {
    double t0 = now();
    for (auto &x : nets) x.add_layer();
    nets = extend_nets(nets, sym, false, keep[d - in_depth], gen, threads, chunk);
    fprintf(stderr, "gen: depth %d -> %zu non-redundant prefixes (%.1f s); best |out| = %zu\n", d + 1, nets.size(),
            now() - t0, nets.empty() ? 0 : nets[0].outputs.size());
    if (!arg("out").empty()) save_nets(nets, arg("out") + ".d" + std::to_string(d + 1), "depth=" + std::to_string(d + 1));
  }
  save_nets(nets, arg("out"), "depth=" + std::to_string(depth) + " sym=" + std::to_string(sym));
  return 0;
}

int cmd_stack() {
  std::string mode = arg("mode", "nested");
  std::vector<Net> A = load_nets(arg("a"), argi("na", 0), true);
  std::vector<Net> B;
  if (mode == "nested") B = load_nets(arg("b"), argi("nb", 0), true);
  std::vector<Net> out;
  if (mode == "nested") {
    for (auto &a : A)
      for (auto &b : B) {
        Net s = stack_nets(a, b, mode);
        CHECK(s.is_symmetric());
        out.push_back(s);
      }
  } else {
    for (auto &a : A) {
      Net s = stack_nets(a, a, mode);
      CHECK(s.is_symmetric());
      CHECK(is_symmetric_set(s.n, s.outputs));
      out.push_back(s);
    }
  }
  std::stable_sort(out.begin(), out.end(), [](const Net &a, const Net &b) { return a.outputs.size() < b.outputs.size(); });
  for (auto &s : out) fprintf(stderr, "stacked |out|=%zu  %s\n", s.outputs.size(), s.to_string().c_str());
  save_nets(out, arg("out"), "stacked mode=" + mode);
  return 0;
}

int cmd_extend() {
  bool sym = flag("sym");
  int keep = argi("keep", 64);
  int layers = argi("layers", 1);
  int threads = argi("threads", std::thread::hardware_concurrency());
  std::mt19937 gen(argi("seed", 1));
  std::vector<Net> nets = load_nets(arg("in"), argi("n", 0), true);
  if (flag("limit") && (int)nets.size() > argi("limit", 0)) nets.resize(argi("limit", 0));
  int n = nets[0].n;
  for (int L = 0; L < layers; L++) {
    for (auto &x : nets) {
      if (sym) CHECK(x.is_symmetric());
      x.add_layer();
    }
    for (int round = 0; round < n / 2; round++) {
      double t0 = now();
      nets = extend_nets(nets, sym, true, keep, gen, threads, 1 << 30);
      fprintf(stderr, "extend: layer +%d round %d: %zu networks, best |out|=%zu worst=%zu (%.1f s)\n", L + 1, round,
              nets.size(), nets[0].outputs.size(), nets.back().outputs.size(), now() - t0);
    }
    if (!arg("out").empty()) save_nets(nets, arg("out") + ".L" + std::to_string(L + 1), "greedy layer");
  }
  save_nets(nets, arg("out"), "greedy keep=" + std::to_string(keep));
  return 0;
}

int cmd_cnf() {
  bool sym = flag("sym");
  int depth = argi("depth", 0);
  std::string outdir = arg("outdir");
  std::mt19937 gen(argi("seed", 1));
  std::vector<Net> nets = load_nets(arg("in"), argi("n", 0), false);
  if (flag("limit") && (int)nets.size() > argi("limit", 0)) nets.resize(argi("limit", 0));
  for (auto &x : nets) x.outputs = compute_outputs(x);
  std::filesystem::create_directories(outdir);
  int start = argi("start", 0);
  for (int idx = start; idx < (int)nets.size(); idx++) {
    const Net &net = nets[idx];
    int n = net.n;
    CHECK(depth > net.depth());
    char num[16];
    snprintf(num, sizeof num, "%04d", idx);
    std::string name = outdir + "/" + num + ".cnf";
    if (std::filesystem::exists(name)) continue;
    double t0 = now();
    std::vector<int> perm(n);
    std::vector<Out> outs = net.outputs;
    if (flag("no-window")) {
      for (int i = 0; i < n; i++) perm[i] = i;
    } else {
      auto r = optimize_window(n, net.outputs, gen, sym);
      outs = r.first;
      perm = r.second;
      CHECK(outs == permute_outputs(net.outputs, perm));
    }
    long long ws = 0;
    int wm = 0;
    window_stats(n, outs, &ws, &wm);
    Cnf f = build_cnf(n, depth - net.depth(), outs, sym, argi("subnet", -1));
    std::vector<std::string> header = {"n " + std::to_string(n), "sym " + std::to_string(sym), "depth " + std::to_string(depth),
                                       "prefix_depth " + std::to_string(net.depth()), "prefix " + net.to_string()};
    std::string ps = "perm";
    for (int p : perm) ps += " " + std::to_string(p);
    header.push_back(ps);
    header.push_back("outputs " + std::to_string(net.outputs.size()) + " window_sum " + std::to_string(ws) + " window_max " + std::to_string(wm));
    write_cnf(f, name, header);
    fprintf(stderr, "cnf %04d: |out|=%zu window_sum=%lld max=%d vars=%d clauses=%zu (%.1f s)\n", idx, net.outputs.size(), ws, wm,
            f.nvars, f.clauses.size(), now() - t0);
  }
  return 0;
}

int cmd_decode() {
  CnfMeta m = read_cnf_meta(arg("cnf"));
  bool sat = false;
  std::vector<int> lits = read_solution(arg("sol"), &sat);
  if (!sat) {
    printf("UNSAT\n");
    return 1;
  }
  int n = m.n;
  Net prefix = parse_net(n, m.prefix_str);
  prefix.outputs = compute_outputs(prefix);
  int d = m.depth - prefix.depth();
  Net suffix(n, d);
  for (int v : lits) {
    auto it = m.var2comp.find(v);
    if (it == m.var2comp.end()) continue;
    auto [k, i, j] = it->second;
    CHECK(suffix.layers[k][i] == -1 && suffix.layers[k][j] == -1);
    suffix.layers[k][i] = j;
    suffix.layers[k][j] = i;
    if (m.sym && i + j != n - 1) {
      int a = n - 1 - j, b = n - 1 - i;
      CHECK(suffix.layers[k][a] == -1 && suffix.layers[k][b] == -1);
      suffix.layers[k][a] = b;
      suffix.layers[k][b] = a;
    }
  }
  // un-permute: suffix lives in permuted space; channel a there = original inv[a]
  std::vector<int> inv = inverse_perm(m.perm);
  int untangled = 0;
  Net suffix_orig = permute_net(suffix, inv, &untangled);
  Net full = prefix;
  for (int l = 0; l < d; l++) {
    full.add_layer();
    for (int i = 0; i < n; i++)
      if (suffix_orig.layers[l][i] > i) full.add_comp(i, suffix_orig.layers[l][i]);
  }
  bool ok = full.is_sorting();
  printf("# raw: n=%d depth=%d size=%d sorting=%d symmetric=%d |out|=%zu untangled=%d\n", n, full.depth(), full.size(), ok, full.is_symmetric(), full.outputs.size(), untangled);
  if (!ok) {
    printf("%s", full.to_multiline().c_str());
    printf("DECODE ERROR: not a sorting network\n");
    return 2;
  }
  Net simp = flag("raw") ? full : simplify_and_relayer(full);
  CHECK(simp.is_sorting());
  printf("# simplified: n=%d depth=%d size=%d symmetric=%d\n", n, simp.depth(), simp.size(), simp.is_symmetric());
  printf("%s", simp.to_multiline().c_str());
  if (!arg("out").empty()) {
    std::ofstream f(arg("out"));
    f << "# n=" << n << " size=" << simp.size() << " depth=" << simp.depth() << "\n" << simp.to_multiline();
  }
  return 0;
}

int cmd_info() {
  int n = argi("n", 0);
  std::vector<Net> nets;
  if (flag("multiline")) nets.push_back(load_multiline(arg("net"), n));
  else nets = load_nets(arg("net"), n, false);
  for (auto &net : nets) {
    Net t(net.n, 0);
    std::string sizes;
    for (int l = 0; l < net.depth(); l++) {
      t.layers.push_back(net.layers[l]);
      t.outputs = compute_outputs(t);
      sizes += std::to_string(t.outputs.size()) + " ";
    }
    net.outputs = t.outputs;
    printf("n=%d size=%d depth=%d symmetric=%d sorting=%d |out| per layer: %s\n", net.n, net.size(), net.depth(), net.is_symmetric(),
           net.is_sorting(), sizes.c_str());
    if (flag("print")) printf("%s", net.to_multiline().c_str());
  }
  return 0;
}

int cmd_sizes() {
  std::vector<Net> nets = load_nets(arg("in"), argi("n", 0), false);
  if (flag("limit") && (int)nets.size() > argi("limit", 0)) nets.resize(argi("limit", 0));
  for (auto &x : nets) x.outputs = compute_outputs(x);
  for (size_t i = 0; i < nets.size(); i++) printf("%zu %zu %s\n", i, nets[i].outputs.size(), nets[i].to_string().c_str());
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: snt <gen|stack|extend|cnf|decode|info|sizes> [--key value] [--flag]\n");
    return 2;
  }
  std::string cmd = argv[1];
  for (int i = 2; i < argc; i++) {
    std::string a = argv[i];
    if (a.rfind("--", 0) == 0) {
      std::string k = a.substr(2);
      if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) args[k] = argv[++i];
      else args[k] = "1";
    } else if (a == "-n" && i + 1 < argc) {
      args["n"] = argv[++i];
    }
  }
  if (cmd == "gen") return cmd_gen();
  if (cmd == "stack") return cmd_stack();
  if (cmd == "extend") return cmd_extend();
  if (cmd == "cnf") return cmd_cnf();
  if (cmd == "decode") return cmd_decode();
  if (cmd == "info") return cmd_info();
  if (cmd == "sizes") return cmd_sizes();
  fprintf(stderr, "unknown command %s\n", cmd.c_str());
  return 2;
}
