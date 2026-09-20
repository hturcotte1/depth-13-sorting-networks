#include "iso.h"

#include <atomic>
#include <cmath>
#include <mutex>
#include <numeric>
#include <thread>
#include <functional>

namespace {

Out full_mask(int n) { return n == 64 ? ~Out(0) : (bit(n) - 1); }

// rows: sorted popcounts (zeros, ones); cols: sorted per-column counts (zeros, ones)
struct Agg {
  std::array<std::vector<uint8_t>, 2> row;
  std::array<std::vector<uint64_t>, 2> col;
};

Agg aggregate(int n, const std::vector<Out> &s) {
  Agg a;
  a.row[1].reserve(s.size());
  a.row[0].reserve(s.size());
  for (Out x : s) {
    int pc = std::popcount(x);
    a.row[1].push_back(pc);
    a.row[0].push_back(n - pc);
  }
  std::sort(a.row[0].begin(), a.row[0].end());
  std::sort(a.row[1].begin(), a.row[1].end());
  a.col[0].assign(n, 0);
  a.col[1].assign(n, 0);
  for (int i = 0; i < n; i++) {
    uint64_t c = 0;
    for (Out x : s) c += (x >> i) & 1;
    a.col[1][i] = c;
    a.col[0][i] = s.size() - c;
  }
  std::sort(a.col[0].begin(), a.col[0].end());
  std::sort(a.col[1].begin(), a.col[1].end());
  return a;
}

Agg invert_agg(const Agg &a) {
  Agg b;
  b.row = {a.row[1], a.row[0]};
  b.col = {a.col[1], a.col[0]};
  return b;
}

// necessary conditions for sigma(a) subset b
bool precheck_col(int n, const Agg &a, const Agg &b) {
  for (int t = 0; t < 2; t++)
    for (int i = 0; i < n; i++)
      if (a.col[t][i] > b.col[t][i]) return false;
  return true;
}
bool precheck_row(const Agg &a, const Agg &b) {
  // sorted popcount multiset of a must embed in that of b: a[i] >= b[i] elementwise
  // (Wang's condition: RowSumA is a subsequence of RowSumB, checked as a[i]>=b[i] on sorted seqs)
  for (int t = 0; t < 2; t++) {
    if (a.row[t].size() > b.row[t].size()) return false;
    for (size_t i = 0; i < a.row[t].size(); i++)
      if (a.row[t][i] < b.row[t][i]) return false;
  }
  return true;
}

bool bt_rec(int n, const std::vector<Out> &a, const std::vector<std::vector<Out>> &b_pasts, bool symmetric, int pos,
            std::vector<int> &perm, std::vector<bool> &used) {
  // check partial permutation perm[0..pos-1] (and mirrored part)
  std::vector<Out> ap;
  ap.reserve(a.size());
  for (Out x : a) {
    Out y = 0;
    for (int j = 0; j < pos; j++) y |= ((x >> perm[j]) & 1) << j;
    if (symmetric)
      for (int j = n - pos; j < n; j++) y |= ((x >> perm[j]) & 1) << j;
    ap.push_back(y);
  }
  std::sort(ap.begin(), ap.end());
  ap.erase(std::unique(ap.begin(), ap.end()), ap.end());
  if (!std::includes(b_pasts[pos].begin(), b_pasts[pos].end(), ap.begin(), ap.end())) return false;
  if (pos == n) return true;
  if (symmetric && pos == n / 2) return true;
  for (int i = 0; i < n; i++) {
    if (used[i]) continue;
    if (symmetric && used[n - 1 - i]) continue;
    perm[pos] = i;
    used[i] = true;
    if (symmetric) {
      perm[n - 1 - pos] = n - 1 - i;
      used[n - 1 - i] = true;
    }
    if (bt_rec(n, a, b_pasts, symmetric, pos + 1, perm, used)) return true;
    used[i] = false;
    if (symmetric) used[n - 1 - i] = false;
  }
  return false;
}

bool backtracking(int n, const std::vector<Out> &a, const std::vector<Out> &b, bool symmetric) {
  if (symmetric) CHECK(n % 2 == 0);
  std::vector<int> perm(n, 0);
  std::vector<bool> used(n, false);
  std::vector<std::vector<Out>> b_pasts;
  int last = symmetric ? n / 2 : n;
  for (int pos = 0; pos <= last; pos++) {
    Out pm = (pos == 0) ? 0 : (bit(pos) - 1);
    if (symmetric && pos > 0) pm |= pm << (n - pos);
    std::vector<Out> bp;
    bp.reserve(b.size());
    for (Out x : b) bp.push_back(x & pm);
    std::sort(bp.begin(), bp.end());
    bp.erase(std::unique(bp.begin(), bp.end()), bp.end());
    b_pasts.push_back(std::move(bp));
  }
  return bt_rec(n, a, b_pasts, symmetric, 0, perm, used);
}

bool is_iso_subset_agg(int n, const std::vector<Out> &a, const Agg &aa, const std::vector<Out> &b, const Agg &ab, bool symmetric) {
  if (a.size() > b.size()) return false;
  if (!precheck_col(n, aa, ab)) return false;
  if (!precheck_row(aa, ab)) return false;
  return backtracking(n, a, b, symmetric);
}

void parallel_for(int count, int threads, const std::function<void(int)> &fn) {
  std::atomic<int> next(0);
  std::vector<std::thread> ts;
  for (int t = 0; t < threads; t++)
    ts.emplace_back([&]() {
      while (true) {
        int i = next.fetch_add(1);
        if (i >= count) break;
        fn(i);
      }
    });
  for (auto &t : ts) t.join();
}

}  // namespace

bool is_iso_subset(int n, const std::vector<Out> &a, const std::vector<Out> &b, bool symmetric, std::mt19937 &gen) {
  (void)gen;
  return is_iso_subset_agg(n, a, aggregate(n, a), b, aggregate(n, b), symmetric);
}

std::pair<std::vector<Out>, std::vector<int>> sort_by_weight(int n, const std::vector<Out> &s, std::mt19937 *gen, bool symmetric) {
  std::vector<uint64_t> cnt(n, 0);
  for (Out x : s)
    for (int i = 0; i < n; i++) cnt[i] += (x >> i) & 1;
  std::vector<int> inv(n);
  std::iota(inv.begin(), inv.end(), 0);
  if (gen) {
    if (symmetric) {
      std::uniform_int_distribution<int> dist(0, n - 1);
      for (int i = 0; i < n; i++) {
        int j = dist(*gen);
        std::swap(inv[i], inv[j]);
        if (i + j != n - 1) std::swap(inv[n - 1 - i], inv[n - 1 - j]);
      }
    } else {
      std::shuffle(inv.begin(), inv.end(), *gen);
    }
  }
  std::stable_sort(inv.begin(), inv.end(), [&](int i, int j) { return cnt[i] < cnt[j]; });
  // inv[p] = source channel placed at position p
  std::vector<Out> r;
  r.reserve(s.size());
  for (Out x : s) {
    Out y = 0;
    for (int p = 0; p < n; p++) y |= ((x >> inv[p]) & 1) << p;
    r.push_back(y);
  }
  std::sort(r.begin(), r.end());
  return {r, inverse_perm(inv)};
}

std::vector<bool> find_redundant(int n, std::vector<std::vector<Out>> outs, bool fast, bool symmetric, std::mt19937 &gen, int threads) {
  int N = outs.size();
  for (int i = 1; i < N; i++) CHECK(outs[i - 1].size() <= outs[i].size());
  std::vector<Agg> agg(N), agg_inv(N);
  parallel_for(N, threads, [&](int i) {
    agg[i] = aggregate(n, outs[i]);
    agg_inv[i] = invert_agg(agg[i]);
  });
  std::vector<std::atomic<bool>> red(N);
  for (int i = 0; i < N; i++) red[i] = false;
  int passes = fast ? 2 : 6;
  std::vector<std::mt19937> gens;
  for (int t = 0; t < threads; t++) gens.emplace_back(gen());
  for (int pass = 0; pass < passes; pass++) {
    bool last = (pass + 1 == passes);
    bool use_inv = !fast && (pass + 2 >= passes);
    std::vector<std::vector<Out>> outs_inv;
    if (use_inv) outs_inv = outs;
    // re-randomise column order (symmetric-preserving), and complement copies
    {
      std::mutex m;
      std::atomic<int> next(0);
      std::vector<std::thread> ts;
      for (int t = 0; t < threads; t++)
        ts.emplace_back([&, t]() {
          while (true) {
            int i = next.fetch_add(1);
            if (i >= N) break;
            if (red[i]) continue;
            outs[i] = sort_by_weight(n, outs[i], &gens[t], symmetric).first;
            if (use_inv) {
              for (Out &x : outs_inv[i]) x ^= full_mask(n);
              outs_inv[i] = sort_by_weight(n, outs_inv[i], &gens[t], symmetric).first;
            }
          }
        });
      for (auto &t : ts) t.join();
    }
    parallel_for(N, threads, [&](int i) {
      if (red[i]) return;
      size_t si = outs[i].size();
      for (int j = 0; j < N; j++) {
        if (red[j] || j == i) continue;
        size_t sj = outs[j].size();
        if (si < sj) break;
        if (si == sj && i < j) break;
        if (fast || !last) {
          if (precheck_col(n, agg[j], agg[i]) &&
              std::includes(outs[i].begin(), outs[i].end(), outs[j].begin(), outs[j].end())) {
            red[i] = true;
            return;
          }
          if (use_inv && precheck_col(n, agg[j], agg_inv[i]) &&
              std::includes(outs_inv[i].begin(), outs_inv[i].end(), outs[j].begin(), outs[j].end())) {
            red[i] = true;
            return;
          }
        } else {
          if (is_iso_subset_agg(n, outs[j], agg[j], outs[i], agg[i], symmetric)) {
            red[i] = true;
            return;
          }
          if (is_iso_subset_agg(n, outs[j], agg[j], outs_inv[i], agg_inv[i], symmetric)) {
            red[i] = true;
            return;
          }
        }
      }
    });
    int cnt = 0;
    for (int i = 0; i < N; i++) cnt += !red[i];
    fprintf(stderr, "  prune pass %d/%d: %d remain\n", pass + 1, passes, cnt);
  }
  std::vector<bool> r(N);
  for (int i = 0; i < N; i++) r[i] = red[i];
  return r;
}

std::vector<Net> remove_redundant(std::vector<Net> nets, bool symmetric, bool fast, std::mt19937 &gen, int threads) {
  if (nets.size() <= 1) return nets;
  std::stable_sort(nets.begin(), nets.end(), [](const Net &a, const Net &b) { return a.outputs.size() < b.outputs.size(); });
  int n = nets[0].n;
  std::vector<std::vector<Out>> outs;
  outs.reserve(nets.size());
  for (auto &x : nets) outs.push_back(x.outputs);
  std::vector<bool> red = find_redundant(n, std::move(outs), fast, symmetric, gen, threads);
  std::vector<Net> r;
  for (size_t i = 0; i < nets.size(); i++)
    if (!red[i]) r.push_back(std::move(nets[i]));
  return r;
}

std::vector<Net> clean_up(std::vector<Net> nets, bool symmetric, int keep_best, std::mt19937 &gen, int threads) {
  if (nets.empty()) return nets;
  CHECK(keep_best > 0);
  if ((size_t)keep_best >= nets.size()) return remove_redundant(std::move(nets), symmetric, false, gen, threads);
  nets = remove_redundant(std::move(nets), symmetric, true, gen, threads);
  int filtered = (int)std::ceil(keep_best * 2.0);
  while (true) {
    bool is_filtered = false;
    std::vector<Net> f;
    if ((int)nets.size() > filtered) {
      f.assign(nets.begin(), nets.begin() + filtered);
      is_filtered = true;
    } else {
      f = nets;
    }
    f = remove_redundant(std::move(f), symmetric, false, gen, threads);
    if (!is_filtered || ((int)f.size() > keep_best && f.back().outputs.size() > f[keep_best - 1].outputs.size())) {
      size_t thr = f[std::min<int>(keep_best, f.size()) - 1].outputs.size();
      while (f.back().outputs.size() > thr) f.pop_back();
      return f;
    }
    filtered = (int)std::ceil(1.5 * filtered * std::max<int>(keep_best, f.size()) / (double)f.size());
    fprintf(stderr, "  clean_up: increasing prefilter to %d\n", filtered);
  }
}

std::pair<std::vector<Out>, std::vector<int>> optimize_window(int n, const std::vector<Out> &outs, std::mt19937 &gen, bool symmetric) {
  auto [cur, perm] = sort_by_weight(n, outs, &gen, symmetric);
  long long best = 0;
  window_stats(n, cur, &best, nullptr);
  while (true) {
    bool improved = false;
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), gen);
    for (int i : order) {
      std::vector<int> order2(n);
      std::iota(order2.begin(), order2.end(), 0);
      std::shuffle(order2.begin(), order2.end(), gen);
      for (int j : order2) {
        if (i >= j) continue;
        std::vector<int> sw(n);
        std::iota(sw.begin(), sw.end(), 0);
        std::swap(sw[i], sw[j]);
        if (symmetric && i + j != n - 1) std::swap(sw[n - 1 - i], sw[n - 1 - j]);
        std::vector<Out> cand = permute_outputs(cur, sw);
        long long s = 0;
        window_stats(n, cand, &s, nullptr);
        if (s < best) {
          best = s;
          cur = std::move(cand);
          std::vector<int> inv = inverse_perm(perm);
          std::swap(inv[i], inv[j]);
          if (symmetric && i + j != n - 1) std::swap(inv[n - 1 - i], inv[n - 1 - j]);
          perm = inverse_perm(inv);
          improved = true;
          break;
        }
      }
      if (improved) break;
    }
    if (!improved) break;
  }
  return {cur, perm};
}
