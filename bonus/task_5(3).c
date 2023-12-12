// ## (5) Atomic increment
// Points: 3.
// Atomic increment in N threads. You need to start N threads
// (customizable number), each is constantly doing atomic lock-free
// increment on a certain variable. See how much time it takes to
// reach certain value on that variable depending on thread count and
// memory order. Try at least
// * 100 mln increments with 1 thread and relaxed order;
// * 100 mln increments with 3 threads and relaxed order;
// * 100 mln increments with 3 threads and sequentially consistent
//   order.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#define NUM_RUNS 10
#define NUM_INCREMENTS 100000000

typedef struct {
    int thread_id;
    int num_threads;
} ThreadInfo;

atomic_int counter = 0;

void *atomic_increment(void *arg) {
    ThreadInfo *thread_info = (ThreadInfo *)arg;

    for (int i = 0; i < NUM_INCREMENTS / thread_info->num_threads; i++) {
        atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed);
    }

    pthread_exit(NULL);
}


void atomic_increment_benchmark(int num_threads, memory_order order, long long *durations) {
    pthread_t threads[num_threads];
    ThreadInfo thread_info[num_threads];

    atomic_store_explicit(&counter, 0, order);
    struct timespec start, end;

    for (int j = 0; j < NUM_RUNS; j++) {
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int i = 0; i < num_threads; i++) {
            thread_info[i].thread_id = i;
            thread_info[i].num_threads = num_threads;
            pthread_create(&threads[i], NULL, atomic_increment, &thread_info[i]);
        }

        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
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
    
    atomic_increment_benchmark(1, memory_order_relaxed, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Number of threads: 1, \nMemory Order: memory_order_relaxed\n");
    printf("Min duration: %lld ns\n", durations[0]);
    printf("Max duration: %lld ns\n", durations[NUM_RUNS - 1]);
    printf("Median duration: %lld ns\n", durations[NUM_RUNS / 2]);
    printf("\n");

    atomic_increment_benchmark(3, memory_order_relaxed, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Number of threads: 3, \nMemory Order: memory_order_relaxed\n");
    printf("Min duration: %lld ns\n", durations[0]);
    printf("Max duration: %lld ns\n", durations[NUM_RUNS - 1]);
    printf("Median duration: %lld ns\n", durations[NUM_RUNS / 2]);
    printf("\n");

    atomic_increment_benchmark(3, memory_order_seq_cst, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Number of threads: 3, \nMemory Order: memory_order_seq_cst\n");
    printf("Min duration: %lld ns\n", durations[0]);
    printf("Max duration: %lld ns\n", durations[NUM_RUNS - 1]);
    printf("Median duration: %lld ns\n", durations[NUM_RUNS / 2]);
    printf("\n");

    return 0;
}

