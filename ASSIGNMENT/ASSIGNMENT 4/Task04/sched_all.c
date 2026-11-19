/* Task 4.1 - service order and head movement of all six algorithms.
   Queue: 98 183 37 122 14 124 65 67   head = 53   cylinders 0..199            */
#include <stdio.h>
#include <stdlib.h>
#define N 8
#define MAXCYL 199
static int req[N] = {98, 183, 37, 122, 14, 124, 65, 67};
static int cmp(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }

/* prints the path and returns the total head movement (cylinders) */
static int path(const char *name, const int *ord, int n, int head)
{
    int total = 0, pos = head;
    printf("%-7s %d", name, head);
    for (int i = 0; i < n; i++) { printf(" -> %d", ord[i]); total += abs(ord[i] - pos); pos = ord[i]; }
    printf("\n");
    return total;
}

int main(void)
{
    int head = 53, ord[N + 4], n, i, s[N], t;
    for (i = 0; i < N; i++) s[i] = req[i];
    qsort(s, N, sizeof s[0], cmp);                       /* sorted copy: 14 37 65 67 98 122 124 183 */

    t = path("FCFS", req, N, head);
    printf("        total = %d cylinders\n\n", t);

    int done[N] = {0}, pos = head;                       /* SSTF */
    for (int k = 0; k < N; k++) {
        int b = -1;
        for (i = 0; i < N; i++)
            if (!done[i] && (b < 0 || abs(req[i] - pos) < abs(req[b] - pos))) b = i;
        done[b] = 1; ord[k] = req[b]; pos = req[b];
    }
    t = path("SSTF", ord, N, head);
    printf("        total = %d cylinders\n\n", t);

    n = 0;                                               /* SCAN: down to 0, then up */
    for (i = N - 1; i >= 0; i--) if (s[i] < head) ord[n++] = s[i];
    ord[n++] = 0;
    for (i = 0; i < N; i++) if (s[i] > head) ord[n++] = s[i];
    t = path("SCAN", ord, n, head);
    printf("        total = %d cylinders\n\n", t);

    n = 0;                                               /* C-SCAN: up to 199, jump to 0, up */
    for (i = 0; i < N; i++) if (s[i] > head) ord[n++] = s[i];
    ord[n++] = MAXCYL;
    t = path("C-SCAN", ord, n, head);
    int lowpart[N + 1], m = 0;
    for (i = 0; i < N; i++) if (s[i] < head) lowpart[m++] = s[i];
    int t2 = path("  after jump", lowpart, m, 0);
    printf("        total = %d + %d = %d cylinders (+%d for the 199->0 jump = %d)\n\n",
           t, t2, t + t2, MAXCYL, t + t2 + MAXCYL);

    n = 0;                                               /* LOOK: down to 14, then up to 183 */
    for (i = N - 1; i >= 0; i--) if (s[i] < head) ord[n++] = s[i];
    for (i = 0; i < N; i++) if (s[i] > head) ord[n++] = s[i];
    t = path("LOOK", ord, n, head);
    printf("        total = %d cylinders\n\n", t);

    n = 0;                                               /* C-LOOK: up to 183, jump to 14, up */
    for (i = 0; i < N; i++) if (s[i] > head) ord[n++] = s[i];
    int up_end = ord[n - 1];
    t = path("C-LOOK", ord, n, head);
    int lo[N], k2 = 0;
    for (i = 0; i < N; i++) if (s[i] < head) lo[k2++] = s[i];
    int jump = up_end - lo[0];                           /* 183 -> 14 */
    t2 = path("  after jump", lo + 1, k2 - 1, lo[0]);
    printf("        total = %d + %d = %d cylinders (+%d for the 183->%d jump = %d)\n",
           t, t2, t + t2, jump, lo[0], t + t2 + jump);
    return 0;
}
