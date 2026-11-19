/* Task 2 alternative-concept measurement: reaching the LAST block of a file.
   FAT (linked allocation, table in RAM): follow k pointers   -> O(k)
   Indexed (inode with double-indirect): two table look-ups    -> O(1)
   Compile: gcc -std=c11 -Wall -Wextra -O2 -o fat_bench fat_bench.c                      */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
static double now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
int main(void)
{
    printf("%-12s %-12s %-18s %-18s %-10s\n", "blocks", "file size", "FAT chase (ms)", "indexed (ns)", "ratio");
    for (int p = 16; p <= 24; p += 2) {
        uint32_t n = 1u << p;
        uint32_t *fat = malloc((size_t)n * 4), *perm = malloc((size_t)n * 4);
        for (uint32_t i = 0; i < n; i++) perm[i] = i;
        srand(1);
        for (uint32_t i = n - 1; i > 0; i--) { uint32_t j = (uint32_t)rand() % i; uint32_t t = perm[i]; perm[i] = perm[j]; perm[j] = t; } /* Sattolo: one cycle */
        for (uint32_t i = 0; i < n; i++) fat[perm[i]] = perm[(i + 1) % n];     /* scattered chain, like an aged disk */
        uint32_t cur = perm[0]; volatile uint32_t sink = 0;
        double t0 = now_ms();
        for (uint32_t k = 0; k < n - 1; k++) cur = fat[cur];                    /* k = n-1 FAT look-ups */
        double fat_ms = now_ms() - t0; sink = cur;
        /* two-level index: 1024 index blocks of 1024 pointers (the inode's double-indirect layout) */
        uint32_t per = 1024, nidx = (n + per - 1) / per;
        uint32_t **idx = malloc(nidx * sizeof *idx);
        for (uint32_t i = 0; i < nidx; i++) { idx[i] = malloc(per * 4); for (uint32_t j = 0; j < per; j++) idx[i][j] = i * per + j; }
        double t1 = now_ms(); uint32_t acc = 0;
        for (int rep = 0; rep < 1000000; rep++) { uint32_t k = (uint32_t)(((uint64_t)rep * 2654435761u) % n); acc += idx[k / per][k % per]; }   /* random block numbers */
        double idx_ns = (now_ms() - t1) * 1e6 / 1e6; sink += acc;      /* ns per look-up */
        printf("%-12u %-12s %-18.3f %-18.1f %-10.0f\n", n, p == 16 ? "256 MB" : p == 18 ? "1 GB" : p == 20 ? "4 GB" : p == 22 ? "16 GB" : "64 GB", fat_ms, idx_ns, fat_ms * 1e6 / idx_ns);
        for (uint32_t i = 0; i < nidx; i++) free(idx[i]);
        free(idx); free(fat); free(perm); (void)sink;
    }
    return 0;
}
