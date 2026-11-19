/* Task 1 alternative measurement: finding one file among 12,000 students x 50 files.
   Flat directory (linear search)  vs  3-level tree (group / student / file, linear at each level).
   Compile: gcc -std=c11 -Wall -Wextra -O2 -o dir_bench dir_bench.c                      */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define STUDENTS 12000
#define FILES 50
#define GROUPS 20
#define PER_GROUP (STUDENTS / GROUPS)
typedef char Name[40];
static Name *flat;
static double now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e3 + t.tv_nsec / 1e6; }
int main(void)
{
    flat = malloc((size_t)STUDENTS * FILES * sizeof(Name));
    for (int s = 0; s < STUDENTS; s++) for (int f = 0; f < FILES; f++) snprintf(flat[s * FILES + f], 40, "S%05d_report%02d.docx", s, f);
    static Name group[GROUPS], student[PER_GROUP], file[FILES];
    for (int g = 0; g < GROUPS; g++) snprintf(group[g], 40, "G%02d", g);
    for (int s = 0; s < PER_GROUP; s++) snprintf(student[s], 40, "S%03d", s);
    for (int f = 0; f < FILES; f++) snprintf(file[f], 40, "report%02d.docx", f);
    srand(3); int trials = 200; long cmp_flat = 0, cmp_tree = 0; volatile int sink = 0;
    double tf = 0, tt = 0;
    for (int k = 0; k < trials; k++) {
        int s = rand() % STUDENTS, f = rand() % FILES, g = s / PER_GROUP, sl = s % PER_GROUP;
        char target[40], tg[40], ts[40], tfn[40];
        snprintf(target, 40, "S%05d_report%02d.docx", s, f); snprintf(tg, 40, "G%02d", g); snprintf(ts, 40, "S%03d", sl); snprintf(tfn, 40, "report%02d.docx", f);
        double t0 = now_ms(); int i;
        for (i = 0; i < STUDENTS * FILES; i++) { cmp_flat++; if (strcmp(flat[i], target) == 0) break; }
        tf += now_ms() - t0; sink += i;
        t0 = now_ms();
        for (i = 0; i < GROUPS; i++) { cmp_tree++; if (!strcmp(group[i], tg)) break; }
        for (i = 0; i < PER_GROUP; i++) { cmp_tree++; if (!strcmp(student[i], ts)) break; }
        for (i = 0; i < FILES; i++) { cmp_tree++; if (!strcmp(file[i], tfn)) break; }
        tt += now_ms() - t0;
    }
    printf("entries in the flat directory : %d\n", STUDENTS * FILES);
    printf("flat directory : %8.0f name comparisons / lookup, %8.3f ms / lookup\n", (double)cmp_flat / trials, tf / trials);
    printf("3-level tree   : %8.0f name comparisons / lookup, %8.4f ms / lookup\n", (double)cmp_tree / trials, tt / trials);
    printf("speed-up (comparisons): %.0f x\n", (double)cmp_flat / (double)cmp_tree);
    free(flat); (void)sink; return 0;
}
