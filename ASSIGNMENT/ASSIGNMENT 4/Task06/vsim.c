/* Task 6 Stage 5 - simulate the lecture-video start-up workload on two alternative designs.
   Workload : 2,000 videos (avg 1 GB), Zipf(1.0) popularity, random start minute.
              Each "play start" reads the first 4 MB chunk at the requested minute.
   Design A : 4 hard disks only, videos spread over the disks, FCFS (no scheduling)
   Design A2: same 4 hard disks, C-LOOK disk scheduler
   Design B : 1 SSD holding the 400 most popular videos + the same 4 hard disks (C-LOOK)
   HDD  : 10,000 cylinders, seek = 1.0 + 0.0015*distance ms, rotation 4.17 ms, 4 MB at 150 MB/s
   SSD  : 0.1 ms + 4 MB at 500 MB/s  = 8.1 ms
   Compile: gcc -std=c11 -Wall -Wextra -O2 -o vsim vsim.c -lm                                   */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define NVID 2000
#define NHDD 4
#define CYL  10000
#define NREQ 30000
#define HOT  400
#define SSD_MS 8.1
#define HDD_FIXED_MS (4.17 + 4.0 / 150.0 * 1000.0)
typedef struct { double arr; int cyl; } Rq;
static double cum[NVID];
static int slot[NVID];
static double urand(void) { return (rand() + 1.0) / (RAND_MAX + 2.0); }

static int cmp_d(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }

/* one single-server device; returns per-request latency (wait + service) in lat[]; busy time via *busy */
static void run_device(const Rq *q, int n, int is_ssd, int clook, double *lat, double *busy, double *end_t)
{
    int *pend = malloc((size_t)(n + 1) * sizeof(int));
    int np = 0, next = 0, done = 0, head = 0;
    double t = 0, b = 0;
    while (done < n) {
        while (next < n && q[next].arr <= t) pend[np++] = next++;
        if (np == 0) { t = q[next].arr; continue; }
        int pick = 0;
        if (!is_ssd && clook) {
            pick = -1;
            for (int i = 0; i < np; i++) if (q[pend[i]].cyl >= head && (pick < 0 || q[pend[i]].cyl < q[pend[pick]].cyl)) pick = i;
            if (pick < 0) for (int i = 0; i < np; i++) if (pick < 0 || q[pend[i]].cyl < q[pend[pick]].cyl) pick = i;
        }
        int id = pend[pick];
        double svc;
        if (is_ssd) svc = SSD_MS;
        else { int d = abs(q[id].cyl - head); svc = (d ? 1.0 + 0.0015 * d : 0.0) + HDD_FIXED_MS; head = q[id].cyl; }
        t += svc; b += svc;
        lat[done++] = t - q[id].arr;
        memmove(&pend[pick], &pend[pick + 1], (size_t)(np - pick - 1) * sizeof(int));
        np--;
    }
    *busy = b; *end_t = t;
    free(pend);
}

int main(void)
{
    double z = 0;
    for (int i = 0; i < NVID; i++) { z += 1.0 / (i + 1); cum[i] = z; }
    for (int i = 0; i < NVID; i++) cum[i] /= z;
    srand(7);
    for (int i = 0; i < NVID; i++) slot[i] = i;               /* random placement of videos on the disks */
    for (int i = NVID - 1; i > 0; i--) { int j = rand() % (i + 1), t = slot[i]; slot[i] = slot[j]; slot[j] = t; }

    printf("Zipf share of requests that hit the %d hottest videos: %.1f %%\n\n", HOT, cum[HOT - 1] * 100);
    printf("%-34s %9s %9s %9s %9s %9s\n", "Design / arrival rate", "mean ms", "p95 ms", "p99 ms", "max ms", "busiest%");
    const double rates[3] = {40, 80, 100};
    for (int r = 0; r < 3; r++) {
        for (int design = 0; design < 3; design++) {
            srand(100 + r);                                    /* identical request stream for each design */
            static Rq hq[NHDD][NREQ], sq[NREQ];
            int nh[NHDD] = {0}, ns = 0;
            double t = 0;
            for (int i = 0; i < NREQ; i++) {
                t += -1000.0 / rates[r] * log(urand());
                double u = urand(); int v = 0;
                while (cum[v] < u) v++;
                double frac = urand();
                Rq q; q.arr = t;
                int sl = slot[v], d = sl % NHDD;
                q.cyl = (int)((sl / NHDD) * (CYL / (NVID / NHDD)) + frac * (CYL / (NVID / NHDD) - 1));
                if (design == 2 && v < HOT) sq[ns++] = q;       /* hot video served by the SSD */
                else hq[d][nh[d]++] = q;
            }
            static double lat[NHDD + 1][NREQ]; int cnt[NHDD + 1]; double busy[NHDD + 1], endt[NHDD + 1];
            for (int d = 0; d < NHDD; d++) { cnt[d] = nh[d]; if (nh[d]) run_device(hq[d], nh[d], 0, design != 0, lat[d], &busy[d], &endt[d]); else { busy[d] = 0; endt[d] = 1; } }
            cnt[NHDD] = ns; if (ns) run_device(sq, ns, 1, 0, lat[NHDD], &busy[NHDD], &endt[NHDD]); else { busy[NHDD] = 0; endt[NHDD] = 1; }
            static double all[NREQ]; int na = 0; double sum = 0, mx = 0, util = 0;
            for (int d = 0; d <= NHDD; d++) {
                for (int i = 0; i < cnt[d]; i++) { all[na++] = lat[d][i]; sum += lat[d][i]; if (lat[d][i] > mx) mx = lat[d][i]; }
                double u2 = busy[d] / endt[d]; if (u2 > util) util = u2;
            }
            qsort(all, (size_t)na, sizeof all[0], cmp_d);
            char label[64];
            const char *dn[3] = {"A  4 HDD, FCFS", "A2 4 HDD, C-LOOK", "B  SSD tier + 4 HDD, C-LOOK"};
            snprintf(label, sizeof label, "%s @ %.0f/s", dn[design], rates[r]);
            printf("%-34s %9.1f %9.1f %9.1f %9.1f %8.1f%%\n", label, sum / na, all[(int)(na * 0.95)], all[(int)(na * 0.99)], mx, util * 100);
        }
        printf("\n");
    }
    return 0;
}
