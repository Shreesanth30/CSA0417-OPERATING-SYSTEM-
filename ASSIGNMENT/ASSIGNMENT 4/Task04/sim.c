/* Task 4.2 - disk-scheduling simulation: 10,000 random requests, three loads.
   Model: cylinders 0..199, seek = 0.05 ms per cylinder, 0.5 ms fixed
   (rotation + transfer) per request, requests arrive as a Poisson stream.
   Waiting time = time from arrival until the head starts moving for that request
   (the sweep to the disk end in SCAN/C-SCAN counts as waiting).
   Compile: gcc -std=c11 -Wall -Wextra -O2 -o sim sim.c -lm                      */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define MAXCYL 199
#define NREQ   10000
#define SEEK_MS  0.05
#define FIXED_MS 0.5
#ifndef JUMP_FACTOR
#define JUMP_FACTOR 1.0   /* cost of the C-SCAN return jump relative to normal seeking */
#endif
enum { FCFS, SSTF, SCAN, CSCAN, LOOK, CLOOK, NALGO };
static const char *names[NALGO] = {"FCFS", "SSTF", "SCAN", "C-SCAN", "LOOK", "C-LOOK"};
typedef struct { double arr; int cyl; } Req;
static Req r[NREQ];
static double waits[NREQ];
static int wcyl[NREQ];
static double zone_avg[5];   /* mean wait for cylinders 0-39, 40-79, 80-119, 120-159, 160-199 */

static double urand(void) { return (rand() + 1.0) / (RAND_MAX + 2.0); }

/* index (in pend[]) of the nearest request with cyl >= head (dir>0) or <= head (dir<0); -1 if none */
static int nearest_dir(const int *pend, int np, int head, int dir)
{
    int best = -1;
    for (int i = 0; i < np; i++) {
        int c = r[pend[i]].cyl, d = (c - head) * dir;
        if (d < 0) continue;
        if (best < 0 || d < (r[pend[best]].cyl - head) * dir) best = i;
    }
    return best;
}

static void simulate(int algo, double *avg_move, double *avg_wait, double *max_wait, double *sd_wait)
{
    static int pend[NREQ];
    int np = 0, next = 0, served = 0, head = 100, dir = +1;
    double t = 0, moved = 0;
    while (served < NREQ) {
        while (next < NREQ && r[next].arr <= t) pend[np++] = next++;
        if (np == 0) { t = r[next].arr; continue; }          /* disk idle until next arrival */
        int pick = -1, extra = 0, head_before = head;
        switch (algo) {
        case FCFS: pick = 0; break;                          /* pend[] is in arrival order */
        case SSTF:
            for (int i = 0; i < np; i++)
                if (pick < 0 || abs(r[pend[i]].cyl - head) < abs(r[pend[pick]].cyl - head)) pick = i;
            break;
        case SCAN: case LOOK:
            pick = nearest_dir(pend, np, head, dir);
            if (pick < 0) {                                  /* nothing ahead: turn round */
                if (algo == SCAN) { extra = (dir > 0) ? MAXCYL - head : head; head = (dir > 0) ? MAXCYL : 0; }
                dir = -dir;
                pick = nearest_dir(pend, np, head, dir);
            }
            break;
        case CSCAN:
            pick = nearest_dir(pend, np, head, +1);
            if (pick < 0) { extra = (MAXCYL - head) + MAXCYL; head = 0; pick = nearest_dir(pend, np, head, +1); }
            break;
        case CLOOK:
            pick = nearest_dir(pend, np, head, +1);
            if (pick < 0) {                                  /* jump back to the lowest pending request */
                for (int i = 0; i < np; i++) if (pick < 0 || r[pend[i]].cyl < r[pend[pick]].cyl) pick = i;
            }
            break;
        }
        int id = pend[pick];
        if (algo == CSCAN && extra > 0)                      /* sweep to 199, then the return jump */
            t += ((MAXCYL - (head_before)) + MAXCYL * JUMP_FACTOR) * SEEK_MS;
        else
            t += extra * SEEK_MS;                            /* sweep to the end (SCAN) */
        double w = t - r[id].arr;                            /* the head now starts moving to the request */
        waits[served] = w; wcyl[served] = r[id].cyl;
        int mv = abs(r[id].cyl - head);
        moved += mv + extra;
        t += mv * SEEK_MS + FIXED_MS;
        head = r[id].cyl;
        memmove(&pend[pick], &pend[pick + 1], (size_t)(np - pick - 1) * sizeof pend[0]);
        np--; served++;
    }
    double sum = 0, mx = 0, sq = 0;
    for (int i = 0; i < NREQ; i++) { sum += waits[i]; sq += waits[i] * waits[i]; if (waits[i] > mx) mx = waits[i]; }
    double zs[5] = {0}; int zc[5] = {0};
    for (int i = 0; i < NREQ; i++) { int z = wcyl[i] / 40; if (z > 4) z = 4; zs[z] += waits[i]; zc[z]++; }
    for (int z = 0; z < 5; z++) zone_avg[z] = zc[z] ? zs[z] / zc[z] : 0;
    *avg_move = moved / NREQ; *avg_wait = sum / NREQ; *max_wait = mx;
    *sd_wait = sqrt(sq / NREQ - (sum / NREQ) * (sum / NREQ));
}

int main(void)
{
    const char *load_name[3] = {"LIGHT  (mean inter-arrival 10.0 ms)", "MEDIUM (mean inter-arrival  4.5 ms)", "HEAVY  (mean inter-arrival  3.2 ms)"};
    const double gap[3] = {10.0, 4.5, 3.2};
    static double zones[NALGO][5];
    for (int L = 0; L < 3; L++) {
        srand(2024 + L);                                     /* same request set for every algorithm */
        double t = 0;
        for (int i = 0; i < NREQ; i++) { t += -gap[L] * log(urand()); r[i].arr = t; r[i].cyl = rand() % (MAXCYL + 1); }
        printf("\n%s, %d requests\n", load_name[L], NREQ);
        printf("%-7s %14s %14s %14s %14s\n", "Algo", "avg move/req", "avg wait (ms)", "max wait (ms)", "std dev (ms)");
        for (int a = 0; a < NALGO; a++) {
            double m, w, mx, sd;
            simulate(a, &m, &w, &mx, &sd);
            printf("%-7s %14.2f %14.2f %14.2f %14.2f\n", names[a], m, w, mx, sd);
            if (a == SCAN || a == CSCAN) {
                memcpy(zones[a], zone_avg, sizeof zone_avg);
            }
        }
        printf("Mean wait (ms) by cylinder zone    0-39  40-79 80-119 120-159 160-199   spread(max-min)\n");
        for (int a = SCAN; a <= CSCAN; a++) {
            double mn = zones[a][0], mx2 = zones[a][0];
            for (int z = 1; z < 5; z++) { if (zones[a][z] < mn) mn = zones[a][z]; if (zones[a][z] > mx2) mx2 = zones[a][z]; }
            printf("  %-7s                      %6.2f %6.2f %6.2f %7.2f %8.2f   %8.2f\n", names[a],
                   zones[a][0], zones[a][1], zones[a][2], zones[a][3], zones[a][4], mx2 - mn);
        }
    }
    return 0;
}
