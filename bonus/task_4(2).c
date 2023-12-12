// ## (4) Pthread create + join
// Points: 2.
// You need to create and join a dummy thread with an empty function
// some number of times. To see how much time one create+join costs
// approximately, PER ONE PAIR. Do it in a loop 100k times, measure
// the time per one create+join.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define NUM_RUNS 10
#define NUM_LOOPS 100000

void *dummy_thread(void *arg) {
    pthread_exit(NULL);
}

void create_join_benchmark(long long *durations) {
    pthread_t thread;
    struct timespec start, end;

    for (int j = 0; j < NUM_RUNS; j++) {
        
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int i = 0; i < NUM_LOOPS; i++) {
            pthread_create(&thread, NULL, dummy_thread, NULL);
            pthread_join(thread, NULL);
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        durations[j] = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
    }
}

int compare(const void *a, const void *b) {
    return (*(long long *)a - *(long long *)b);
}

int main() {
    long long durations[NUM_RUNS];

    create_join_benchmark(durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Min duration PER ONE create+join pair: %lld ns\n", durations[0] / NUM_LOOPS);
    printf("Max duration PER ONE create+join pair: %lld ns\n", durations[NUM_RUNS - 1] / NUM_LOOPS);
    printf("Median duration PER ONE create+join pair: %lld ns\n", durations[NUM_RUNS / 2] / NUM_LOOPS);
    printf("\n");
    
    return 0;
}
