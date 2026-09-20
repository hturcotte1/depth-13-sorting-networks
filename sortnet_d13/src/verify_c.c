/*
 * verify_c.c -- exhaustive bit-sliced verifier for comparator networks.
 *
 * Reads a network in Dobbelaere's bracket format (one layer per line, or
 * several "[...]" groups on one line; lines starting with '#' are ignored),
 * runs ALL 2^n binary inputs through it (64 inputs per machine word), and
 * reports whether every input is sorted (zero-one principle).
 *
 * Convention: comparator (i,j) with i<j puts min on channel i, max on j.
 * Sorted binary output = zeros on low channels, ones on high channels.
 *
 * Build:  gcc -O3 -march=native -fopenmp -o verify_c verify_c.c
 * Usage:  ./verify_c [-n N] [-q] network.txt
 * Exit:   0 = sorts all inputs, 1 = does not sort, 2 = parse/usage error.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#define MAXN 64
#define MAXC 4096
#define MAXL 256

static int ci[MAXC], cj[MAXC], clayer[MAXC];
static int ncomp = 0, nlayers = 0;
static int layer_size[MAXL];

static int parse(FILE *f, int *maxch) {
    int c, in_layer = 0, line = 1, cur_used[MAXN];
    *maxch = -1;
    memset(cur_used, 0, sizeof cur_used);
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') { line++; continue; }
        if (c == '#') { while ((c = fgetc(f)) != EOF && c != '\n'); line++; continue; }
        if (c == '[') {
            if (in_layer) { fprintf(stderr, "line %d: nested '['\n", line); return -1; }
            in_layer = 1; memset(cur_used, 0, sizeof cur_used);
            if (nlayers >= MAXL) { fprintf(stderr, "too many layers\n"); return -1; }
            layer_size[nlayers] = 0;
            continue;
        }
        if (c == ']') {
            if (!in_layer) { fprintf(stderr, "line %d: stray ']'\n", line); return -1; }
            in_layer = 0;
            if (layer_size[nlayers] > 0) nlayers++;   /* empty layers are not counted */
            else fprintf(stderr, "warning: line %d: empty layer ignored\n", line);
            continue;
        }
        if (c == '(') {
            int a, b;
            if (!in_layer) { fprintf(stderr, "line %d: comparator outside layer\n", line); return -1; }
            if (fscanf(f, "%d,%d)", &a, &b) != 2) { fprintf(stderr, "line %d: bad comparator\n", line); return -1; }
            if (a < 0 || b < 0 || a >= MAXN || b >= MAXN) { fprintf(stderr, "line %d: channel out of range\n", line); return -1; }
            if (a >= b) { fprintf(stderr, "line %d: comparator (%d,%d) must have i<j\n", line, a, b); return -1; }
            if (cur_used[a] || cur_used[b]) { fprintf(stderr, "line %d: channel reused within a layer at (%d,%d)\n", line, a, b); return -1; }
            cur_used[a] = cur_used[b] = 1;
            if (ncomp >= MAXC) { fprintf(stderr, "too many comparators\n"); return -1; }
            ci[ncomp] = a; cj[ncomp] = b; clayer[ncomp] = nlayers; ncomp++;
            layer_size[nlayers]++;
            if (a > *maxch) *maxch = a;
            if (b > *maxch) *maxch = b;
            continue;
        }
        if (isspace(c) || c == ',') continue;
        fprintf(stderr, "line %d: unexpected character '%c'\n", line, c);
        return -1;
    }
    if (in_layer) { fprintf(stderr, "unterminated layer\n"); return -1; }
    return 0;
}

int main(int argc, char **argv) {
    int n = 0, quiet = 0, maxch;
    const char *path = NULL;
    for (int a = 1; a < argc; a++) {
        if (!strcmp(argv[a], "-n") && a + 1 < argc) n = atoi(argv[++a]);
        else if (!strcmp(argv[a], "-q")) quiet = 1;
        else path = argv[a];
    }
    if (!path) { fprintf(stderr, "usage: %s [-n N] [-q] network.txt\n", argv[0]); return 2; }
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return 2; }
    if (parse(f, &maxch) < 0) return 2;
    fclose(f);
    if (ncomp == 0) { fprintf(stderr, "no comparators\n"); return 2; }
    if (n == 0) n = maxch + 1;
    if (n <= maxch) { fprintf(stderr, "-n %d too small for channel %d\n", n, maxch); return 2; }
    if (n > 40) { fprintf(stderr, "n=%d too large for exhaustive check\n", n); return 2; }

    /* lane patterns for channels 0..5 (64 lanes = low 6 bits of input index) */
    uint64_t pat[6];
    for (int b = 0; b < 6; b++) {
        uint64_t p = 0;
        for (int lane = 0; lane < 64; lane++) if ((lane >> b) & 1) p |= (uint64_t)1 << lane;
        pat[b] = p;
    }
    int low = n < 6 ? n : 6;
    uint64_t lanemask = n >= 6 ? ~(uint64_t)0 : (((uint64_t)1 << (1 << n)) - 1);
    uint64_t nblocks = (n > 6) ? ((uint64_t)1 << (n - 6)) : 1;

    long long fail_count = 0;
    uint64_t first_fail = ~(uint64_t)0;
    int64_t nb = (int64_t)nblocks;

#pragma omp parallel for schedule(dynamic, 1024) reduction(+:fail_count) reduction(min:first_fail)
    for (int64_t blk = 0; blk < nb; blk++) {
        uint64_t w[MAXN];
        for (int c = 0; c < low; c++) w[c] = pat[c];
        for (int c = 6; c < n; c++) w[c] = ((blk >> (c - 6)) & 1) ? ~(uint64_t)0 : 0;
        for (int k = 0; k < ncomp; k++) {
            uint64_t a = w[ci[k]], b = w[cj[k]];
            w[ci[k]] = a & b;   /* min */
            w[cj[k]] = a | b;   /* max */
        }
        uint64_t bad = 0;
        for (int c = 0; c + 1 < n; c++) bad |= w[c] & ~w[c + 1];
        bad &= lanemask;
        if (bad) {
            fail_count += __builtin_popcountll(bad);
            int lane = __builtin_ctzll(bad);
            uint64_t inp = ((uint64_t)blk << 6) | (uint64_t)lane;
            if (inp < first_fail) first_fail = inp;
        }
    }

    if (!quiet) {
        printf("file=%s n=%d size=%d depth=%d inputs=%llu\n", path, n, ncomp, nlayers,
               (unsigned long long)((n >= 6) ? nblocks * 64 : ((uint64_t)1 << n)));
    }
    if (fail_count == 0) {
        printf("RESULT: SORTS all 2^%d binary inputs (n=%d, size=%d, depth=%d)\n", n, n, ncomp, nlayers);
        return 0;
    } else {
        printf("RESULT: FAILS on %lld inputs; first failing input (bit c = channel c) = 0x%llx\n",
               fail_count, (unsigned long long)first_fail);
        return 1;
    }
}
