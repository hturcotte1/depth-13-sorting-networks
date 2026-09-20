// iso.h -- subsumption (isomorphic-subset) tests and redundancy pruning,
// ported from Wang 2025 (isomorphism.cc) to 64-bit outputs.
#pragma once
#include "sn.h"

// exists permutation sigma (in the centralizer of reflection if symmetric)
// with sigma(a) subset of b ?
bool is_iso_subset(int n, const std::vector<Out> &a, const std::vector<Out> &b, bool symmetric, std::mt19937 &gen);

// Permute channels so column one-counts are non-decreasing (after an optional
// random (symmetric) shuffle). Returns permuted set and perm (perm[i] = new pos).
std::pair<std::vector<Out>, std::vector<int>> sort_by_weight(int n, const std::vector<Out> &s, std::mt19937 *gen, bool symmetric);

// outs must be sorted by size. Returns is_redundant[i]: some j with
// (|out_j|,j) < (|out_i|,i) has sigma(out_j) subset of out_i or of ~out_i.
// outs is not modified (a working copy is permuted/compacted internally).
std::vector<bool> find_redundant(int n, std::vector<std::vector<Out>> &outs, bool fast, bool symmetric, std::mt19937 &gen, int threads);

// RemoveRedundantNetworks + CleanUp(keep_best) as in Wang's code.
std::vector<Net> remove_redundant(std::vector<Net> nets, bool symmetric, bool fast, std::mt19937 &gen, int threads);
std::vector<Net> clean_up(std::vector<Net> nets, bool symmetric, int keep_best, std::mt19937 &gen, int threads);

// Greedy channel permutation minimising the sum of window sizes (symmetric-aware).
std::pair<std::vector<Out>, std::vector<int>> optimize_window(int n, const std::vector<Out> &outs, std::mt19937 &gen, bool symmetric);
