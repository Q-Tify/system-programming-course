// ## (3) Pthread mutex lock
// Points: 4.
// pthread_mutex_lock/unlock in N threads. You need to start N
// threads (customizable number), each is constantly doing mutex
// lock/unlock. For each lock you need to increment a global atomic
// counter. See how much time it takes to do certain amount of
// locks/unlocks depending on thread count, PER ONE LOCK/UNLOCK pair.
// Try at least
// * 10 mln locks with 1 thread;
// * 10 mln locks with 2 threads;
// * 10 mln locks with 3 threads.


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>

#define NUM_RUNS 10
#define NUM_LOCKS 10000000

typedef struct {
    int thread_id;
    int num_threads;
} ThreadInfo;

atomic_int counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg) {
    ThreadInfo *thread_info = (ThreadInfo *)arg;

    for (int i = 0; i < NUM_LOCKS / thread_info->num_threads; i++) {
        pthread_mutex_lock(&mutex);
        counter++;
        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(NULL);
}


void pthread_mutex_benchmark(int num_threads, long long *durations) {
    pthread_t threads[num_threads];
    ThreadInfo thread_info[num_threads];
    struct timespec start, end;
    
    for (int j = 0; j < NUM_RUNS; j++) {
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int i = 0; i < num_threads; i++) {
            thread_info[i].thread_id = i;
            thread_info[i].num_threads = num_threads;
            pthread_create(&threads[i], NULL, worker, &thread_info[i]);
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

    pthread_mutex_benchmark(1, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Number of threads: 1\n");
    printf("Min duration PER ONE LOCK/UNLOCK pair: %lld ns\n", durations[0] / NUM_LOCKS);
    printf("Max duration PER ONE LOCK/UNLOCK pair: %lld ns\n", durations[NUM_RUNS - 1] / NUM_LOCKS);
    printf("Median duration PER ONE LOCK/UNLOCK pair: %lld ns\n", durations[NUM_RUNS / 2] / NUM_LOCKS);
    printf("\n");

    pthread_mutex_benchmark(2, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Number of threads: 2\n");
    printf("Min duration PER ONE LOCK/UNLOCK pair: %lld ns\n", durations[0] / NUM_LOCKS);
    printf("Max duration PER ONE LOCK/UNLOCK pair: %lld ns\n", durations[NUM_RUNS - 1] / NUM_LOCKS);
    printf("Median duration PER ONE LOCK/UNLOCK pair: %lld ns\n", durations[NUM_RUNS / 2] / NUM_LOCKS);
    printf("\n");

    pthread_mutex_benchmark(3, durations);
    qsort(durations, NUM_RUNS, sizeof(long long), compare);
    printf("Number of threads: 3\n");
    printf("Min duration PER ONE LOCK/UNLOCK pair: %lld ns\n", durations[0] / NUM_LOCKS);
    printf("Max duration PER ONE LOCK/UNLOCK pair: %lld ns\n", durations[NUM_RUNS - 1] / NUM_LOCKS);
    printf("Median duration PER ONE LOCK/UNLOCK pair: %lld ns\n", durations[NUM_RUNS / 2] / NUM_LOCKS);
    printf("\n");

    return 0;
}