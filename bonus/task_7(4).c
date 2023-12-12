// ## (7) False sharing
// Points: 4.
// You need to check how strongly threads can affect each other even
// if they don't access the same data. You need to create an array of
// uint64_t numbers. And then start N threads. Each thread should
// increment its own number in the array in a loop.

// Make sure the compiler won't turn the loop into a single +=
// operation. For that you can try to add a check on a dummy volatile
// variable in the loop's condition.

// Assuming the threads are in an array, thread with index A should
// use number arr[A]. See how much time it takes for all the threads
// to reach certain value of their numbers.

// Then make the threads use numbers on a distance from each other.
// So thread with index A uses number arr[A * 8]. Distance between
// numbers should be at least 64 bytes. Repeat the bench. You need to
// try at least the following combinations:
// * 100 mln increments with 1 thread;
// * 100 mln increments with 2 threads with close numbers;
// * 100 mln increments with 2 threads with distant numbers;
// * 100 mln increments with 3 threads with close numbers;
// * 100 mln increments with 3 threads with distant numbers.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

#define ARRAY_SIZE 1000000
#define NUM_INCREMENTS 100000000
#define NUM_RUNS 10

typedef struct {
    int thread_id;
    int close_numbers;
} ThreadInfo;

volatile int dummy = 0;

uint64_t arr[ARRAY_SIZE];

void *increment_thread(void *arg) {
    ThreadInfo *thread_info = (ThreadInfo *)arg;
    int distance;
    
    if (thread_info->close_numbers == 1) {
        distance = 0;
    } else {
        distance = thread_info->thread_id * 8;
    }

    for (int i = 0; i < NUM_INCREMENTS; ++i) {
        arr[thread_info->thread_id + distance]++;
        dummy = arr[thread_info->thread_id + distance];
    }

    pthread_exit(NULL);
}

void array_increment_benchmark(int num_threads, int close_numbers, long long *durations) {
    pthread_t threads[num_threads];
    ThreadInfo thread_info[num_threads];
    struct timespec start, end;

    for (int j = 0; j < NUM_RUNS; j++){
        for (int i = 0; i < ARRAY_SIZE; i++) {
            arr[i] = 0;
        }

        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int i = 0; i < num_threads; i++) {
            thread_info[i].thread_id = i;
            thread_info[i].close_numbers = close_numbers;
            pthread_create(&threads[i], NULL, increment_thread, &thread_info[i]);
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

int main(void) {
    long long durations[NUM_RUNS];

    array_increment_benchmark(1, 1, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Number of threads: 1, Close numbers: Yes\n");
    printf("Min duration: %lld ns\n", durations[0]);
    printf("Max duration: %lld ns\n", durations[NUM_RUNS - 1]);
    printf("Median duration: %lld ns\n", durations[NUM_RUNS / 2]);
    printf("\n");

    array_increment_benchmark(2, 1, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Number of threads: 2, Close numbers: Yes\n");
    printf("Min duration: %lld ns\n", durations[0]);
    printf("Max duration: %lld ns\n", durations[NUM_RUNS - 1]);
    printf("Median duration: %lld ns\n", durations[NUM_RUNS / 2]);
    printf("\n");

    array_increment_benchmark(2, 0, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Number of threads: 2, Close numbers: No\n");
    printf("Min duration: %lld ns\n", durations[0]);
    printf("Max duration: %lld ns\n", durations[NUM_RUNS - 1]);
    printf("Median duration: %lld ns\n", durations[NUM_RUNS / 2]);
    printf("\n");

    array_increment_benchmark(3, 1, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Number of threads: 3, Close numbers: Yes\n");
    printf("Min duration: %lld ns\n", durations[0]);
    printf("Max duration: %lld ns\n", durations[NUM_RUNS - 1]);
    printf("Median duration: %lld ns\n", durations[NUM_RUNS / 2]);
    printf("\n");

    array_increment_benchmark(3, 0, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Number of threads: 3, Close numbers: No\n");
    printf("Min duration: %lld ns\n", durations[0]);
    printf("Max duration: %lld ns\n", durations[NUM_RUNS - 1]);
    printf("Median duration: %lld ns\n", durations[NUM_RUNS / 2]);
    printf("\n");

    return 0;
}
