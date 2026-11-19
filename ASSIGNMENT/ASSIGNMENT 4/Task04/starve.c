/* Task 4 Complete-the-Code extension: a stream of requests near cylinder 60 arriving
   every 1 ms.  Service time = 1.0 ms + 0.01 ms per cylinder, so the disk can never
   catch up with the stream.  What happens to the request at cylinder 183?
   Compile: gcc -std=c11 -Wall -Wextra -o starve starve.c                        */
#include <stdio.h>
#include <stdlib.h>
#define MAXCYL 199
#define LIMIT_MS 2000.0
#define MAXP 5000
static int init_q[8] = {98, 183, 37, 122, 14, 124, 65, 67};
static int stream_cyl[5] = {58, 60, 62, 59, 61};

static void run(const char *name, int sstf)
{
    int cyl[MAXP], id[MAXP], np = 0, head = 53, dir = +1, k = 0;
    double t = 0, served_at[8]; for (int i = 0; i < 8; i++) served_at[i] = -1;
    for (int i = 0; i < 8; i++) { cyl[np] = init_q[i]; id[np] = i; np++; }
    double next_arr = 1.0;
    while (t < LIMIT_MS) {
        while (next_arr <= t && np < MAXP) { cyl[np] = stream_cyl[k++ % 5]; id[np] = -1; np++; next_arr += 1.0; }
        if (np == 0) { t = next_arr; continue; }
        int pick = -1;
        if (sstf) {
            for (int i = 0; i < np; i++) if (pick < 0 || abs(cyl[i] - head) < abs(cyl[pick] - head)) pick = i;
        } else {                                              /* SCAN, going up first */
            for (int pass = 0; pass < 2 && pick < 0; pass++) {
                for (int i = 0; i < np; i++) {
                    int d = (cyl[i] - head) * dir;
                    if (d >= 0 && (pick < 0 || d < (cyl[pick] - head) * dir)) pick = i;
                }
                if (pick < 0) { head = (dir > 0) ? MAXCYL : 0; dir = -dir; }   /* reverse at the disk end */
            }
        }
        double svc = 1.0 + 0.01 * abs(cyl[pick] - head);
        t += svc; head = cyl[pick];
        if (id[pick] >= 0) served_at[id[pick]] = t;
        cyl[pick] = cyl[np - 1]; id[pick] = id[np - 1]; np--;
    }
    printf("\n%s  (simulated %.0f ms, %d requests still queued at the end)\n", name, LIMIT_MS, np);
    for (int i = 0; i < 8; i++) {
        if (served_at[i] < 0) printf("  request at cylinder %3d : NOT served - starved\n", init_q[i]);
        else printf("  request at cylinder %3d : served at t = %.1f ms\n", init_q[i], served_at[i]);
    }
}

int main(void)
{
    run("SSTF with a stream near cylinder 60", 1);
    run("SCAN with the same stream", 0);
    return 0;
}
