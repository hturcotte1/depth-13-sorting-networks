// gen.cc -- prefix generation (generate-and-prune), stacking, greedy extension.
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

#include "iso.h"
#include "sn.h"

namespace {

// Wang's AddComparator DFS: enumerate comparator subsets added to the last layer.
void dfs(const Net &net, bool sym, const std::vector<std::vector<char>> &hinv, int i0, int remaining,
         const std::function<void(const Net &)> &emit) {
  int n = net.n;
  emit(net);
  if (remaining == 0) return;
  const auto &L = net.layers.back();
  for (int i = i0; i < n; i++) {
    if (L[i] != -1) continue;
    if (sym && L[n - 1 - i] != -1) continue;
    for (int j = i + 1; j < n; j++) {
      if (L[j] != -1) continue;
      if (sym && n - 1 - j < i0) continue;
      if (sym && L[n - 1 - j] != -1) continue;
      if (!hinv[i][j]) continue;
      if (sym && !hinv[n - 1 - j][n - 1 - i]) continue;
      Net nn = net;
      nn.add_comp(i, j);
      if (sym && i + j != n - 1) nn.add_comp(n - 1 - j, n - 1 - i);
      std::vector<std::vector<char>> h = hinv;
      auto upd = [&](int a, int b) {  // recompute pair (a,b), a<b, and mirror
        h[a][b] = has_inverse(nn.outputs, a, b);
        if (sym) h[n - 1 - b][n - 1 - a] = h[a][b];
      };
      for (int k = 0; k < n; k++) {
        if (k < i) upd(k, i);
        if (k > i) upd(i, k);
        if (k < j) upd(k, j);
        if (k > j) upd(j, k);
      }
      dfs(nn, sym, h, i + 1, remaining - 1, emit);
    }
  }
}

std::vector<std::vector<char>> compute_hinv(const Net &net) {
  int n = net.n;
  std::vector<std::vector<char>> h(n, std::vector<char>(n, 0));
  for (int i = 0; i < n; i++)
    for (int j = i + 1; j < n; j++) h[i][j] = has_inverse(net.outputs, i, j);
  return h;
}

}  // namespace

// Extend every network (whose last layer is being filled) by comparators.
// one_at_a_time: add exactly one comparator (+mirror) per network per call.
// Networks are processed in chunks with a fast pre-prune to bound memory.
std::vector<Net> extend_nets(const std::vector<Net> &nets, bool sym, bool one_at_a_time, int keep_best,
                             std::mt19937 &gen, int threads, int chunk) {
  int n = nets[0].n;
  const size_t FLUSH_ELEMS = 20000000;  // per-worker buffer budget (output-set elements, ~160 MB) before a fast local prune
  std::vector<Net> acc;
  std::mutex acc_m;
  std::atomic<size_t> total_generated(0);
  std::atomic<size_t> next(0);
  std::vector<std::mt19937> gens;
  for (int t = 0; t < threads; t++) gens.emplace_back(gen());
  auto local_prune = [&](std::vector<Net> &buf, std::mt19937 &g) {
    buf = remove_redundant(std::move(buf), sym, true, g, 1);
    if (keep_best > 0 && (int)buf.size() > 4 * keep_best) {
      size_t thr = buf[4 * keep_best - 1].outputs.size();
      while (buf.back().outputs.size() > thr) buf.pop_back();
    }
  };
  std::vector<std::thread> ts;
  for (int t = 0; t < threads; t++)
    ts.emplace_back([&, t]() {
      std::vector<Net> buf;
      size_t buf_elems = 0;
      auto emit = [&](const Net &x) {
        buf.push_back(x);
        buf_elems += x.outputs.size();
        total_generated++;
        if (buf_elems >= FLUSH_ELEMS) {
          local_prune(buf, gens[t]);
          buf_elems = 0;
          for (auto &b : buf) buf_elems += b.outputs.size();
        }
      };
      while (true) {
        size_t idx = next.fetch_add(1);
        if (idx >= nets.size()) break;
        const Net &net = nets[idx];
        CHECK(!net.outputs.empty());
        dfs(net, sym, compute_hinv(net), 0, one_at_a_time ? 1 : 1 << 30, emit);
        if ((idx + 1) % 500 == 0) fprintf(stderr, "extend: %zu/%zu prefixes expanded, %zu candidates so far\n", idx + 1, nets.size(), total_generated.load());
      }
      if (buf_elems > FLUSH_ELEMS / 2) local_prune(buf, gens[t]);
      std::lock_guard<std::mutex> lk(acc_m);
      for (auto &x : buf) acc.push_back(std::move(x));
    });
  for (auto &t : ts) t.join();
  (void)chunk;
  fprintf(stderr, "extend: total generated %zu, %zu after local pruning; final clean_up (keep=%d)\n", total_generated.load(), acc.size(), keep_best);
  if (keep_best <= 0) return remove_redundant(std::move(acc), sym, false, gen, threads);
  return clean_up(std::move(acc), sym, keep_best, gen, threads);
}

std::vector<Net> first_layers(int n, bool sym) {
  std::vector<Net> r;
  if (sym) {
    CHECK(n % 2 == 0);
    for (int k = 0; k <= n / 4; k++) {
      Net net(n, 1);
      for (int i = 0; i < k; i++) {
        net.layers[0][2 * i] = 2 * i + 1;
        net.layers[0][2 * i + 1] = 2 * i;
        net.layers[0][n - 1 - 2 * i] = n - 2 - 2 * i;
        net.layers[0][n - 2 - 2 * i] = n - 1 - 2 * i;
      }
      for (int i = 2 * k; i < n / 2; i++) {
        net.layers[0][i] = n - 1 - i;
        net.layers[0][n - 1 - i] = i;
      }
      net.outputs = compute_outputs(net);
      r.push_back(net);
    }
  } else {
    Net net(n, 1);
    for (int i = 0; i + 1 < n; i += 2) {
      net.layers[0][i] = i + 1;
      net.layers[0][i + 1] = i;
    }
    net.outputs = compute_outputs(net);
    r.push_back(net);
  }
  return r;
}

// Stacking. nested: A symmetric on outer channels, B symmetric on inner channels.
// mirrored: A on channels 0..na-1 and its reflection on na..2na-1.
Net stack_nets(const Net &a, const Net &b, const std::string &mode) {
  int na = a.n, nb = b.n;
  if (mode == "nested") {
    CHECK(na % 2 == 0 && nb % 2 == 0);
    CHECK(a.is_symmetric() && b.is_symmetric());
    int n = na + nb;
    std::vector<int> pa(na), pb(nb);
    for (int i = 0; i < na; i++) pa[i] = (i < na / 2) ? i : i + nb;
    for (int i = 0; i < nb; i++) pb[i] = i + na / 2;
    Net r(n, std::max(a.depth(), b.depth()));
    for (int l = 0; l < a.depth(); l++)
      for (int i = 0; i < na; i++)
        if (a.layers[l][i] > i) {
          int x = pa[i], y = pa[a.layers[l][i]];
          r.layers[l][x] = y;
          r.layers[l][y] = x;
        }
    for (int l = 0; l < b.depth(); l++)
      for (int i = 0; i < nb; i++)
        if (b.layers[l][i] > i) {
          int x = pb[i], y = pb[b.layers[l][i]];
          r.layers[l][x] = y;
          r.layers[l][y] = x;
        }
    std::vector<Out> oa, ob;
    for (Out x : a.outputs) {
      Out y = 0;
      for (int i = 0; i < na; i++) y |= ((x >> i) & 1) << pa[i];
      oa.push_back(y);
    }
    for (Out x : b.outputs) {
      Out y = 0;
      for (int i = 0; i < nb; i++) y |= ((x >> i) & 1) << pb[i];
      ob.push_back(y);
    }
    r.outputs.reserve(oa.size() * ob.size());
    for (Out x : oa)
      for (Out y : ob) r.outputs.push_back(x | y);
    std::sort(r.outputs.begin(), r.outputs.end());
    return r;
  } else if (mode == "mirrored") {
    // b is ignored; the second block is the reflection of a.
    int n = 2 * na;
    Net r(n, a.depth());
    for (int l = 0; l < a.depth(); l++)
      for (int i = 0; i < na; i++)
        if (a.layers[l][i] > i) {
          int j = a.layers[l][i];
          r.layers[l][i] = j;
          r.layers[l][j] = i;
          int x = n - 1 - j, y = n - 1 - i;
          r.layers[l][x] = y;
          r.layers[l][y] = x;
        }
    // Reflected block: comparators (na-1-j, na-1-i). Its output set equals
    // rho(~output(a)) (reflect-and-invert); cross-checked by direct recomputation.
    Net ra(na, a.depth());
    for (int l = 0; l < a.depth(); l++)
      for (int i = 0; i < na; i++)
        if (a.layers[l][i] > i) {
          int j = a.layers[l][i];
          int x = na - 1 - j, y = na - 1 - i;
          ra.layers[l][x] = y;
          ra.layers[l][y] = x;
        }
    std::vector<Out> ob;
    for (Out x : a.outputs) ob.push_back(reflect_invert(na, x));
    std::sort(ob.begin(), ob.end());
    CHECK(ob == compute_outputs(ra));
    for (Out &x : ob) x <<= na;
    r.outputs.reserve(a.outputs.size() * ob.size());
    for (Out x : a.outputs)
      for (Out y : ob) r.outputs.push_back(x | y);
    std::sort(r.outputs.begin(), r.outputs.end());
    return r;
  }
  CHECK(false);
  return Net();
}
