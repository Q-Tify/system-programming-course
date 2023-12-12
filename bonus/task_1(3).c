// ## (1) Clock bench
// Points: 3.
// Bench clock_gettime() with CLOCK_REALTIME, CLOCK_MONOTONIC,
// CLOCK_MONOTONIC_RAW. Run the function with those params 50 mln
// times and see how much time it took PER ONE CALL.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_RUNS 10
#define NUM_CALLS 50000000

void clock_benchmark(clockid_t clock_type, long long *durations) {
    struct timespec start, end, temp;

    for (int i = 0; i < NUM_RUNS; i++) {

        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int j = 0; j < NUM_CALLS; j++) {
            clock_gettime(clock_type, &temp);
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        durations[i] = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
    }
}

int compare(const void *a, const void *b) {
    return (*(long long *)a - *(long long *)b);
}

int main() {
    long long durations[NUM_RUNS];

    clock_benchmark(CLOCK_REALTIME, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Clock type: CLOCK_REALTIME\n");
    printf("Min duration PER ONE CALL: %lld ns\n", durations[0] / NUM_CALLS);
    printf("Max duration PER ONE CALL: %lld ns\n", durations[NUM_RUNS - 1] / NUM_CALLS);
    printf("Median duration PER ONE CALL: %lld ns\n", durations[NUM_RUNS / 2] / NUM_CALLS);
    printf("\n");

    clock_benchmark(CLOCK_MONOTONIC, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Clock type: CLOCK_MONOTONIC\n");
    printf("Min duration PER ONE CALL: %lld ns\n", durations[0] / NUM_CALLS);
    printf("Max duration PER ONE CALL: %lld ns\n", durations[NUM_RUNS - 1] / NUM_CALLS);
    printf("Median duration PER ONE CALL: %lld ns\n", durations[NUM_RUNS / 2] / NUM_CALLS);
    printf("\n");

    clock_benchmark(CLOCK_MONOTONIC_RAW, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Clock type: CLOCK_MONOTONIC_RAW\n");
    printf("Min duration PER ONE CALL: %lld ns\n", durations[0] / NUM_CALLS);
    printf("Max duration PER ONE CALL: %lld ns\n", durations[NUM_RUNS - 1] / NUM_CALLS);
    printf("Median duration PER ONE CALL: %lld ns\n", durations[NUM_RUNS / 2] / NUM_CALLS);

    return 0;
}
