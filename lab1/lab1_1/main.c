#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "timer.h"

int global_hits = 0;
int total_attempts;
pthread_mutex_t mutex_lock;

void* calculate_pi(void* arg) {
    unsigned int seed = time(NULL) + pthread_self();
    int local_hits = 0;
    int attempts_per_thread = *(int*)arg;

    for (int i = 0; i < attempts_per_thread; ++i) {
        double x = ((double)rand_r(&seed) / RAND_MAX) * 2.0 - 1.0;
        double y = ((double)rand_r(&seed) / RAND_MAX) * 2.0 - 1.0;

        if (x * x + y * y <= 1.0) {
            ++local_hits;
        }
    }

    pthread_mutex_lock(&mutex_lock);
    global_hits += local_hits;
    pthread_mutex_unlock(&mutex_lock);

    return NULL;
}

void create_threads(pthread_t* threads, int num_threads, int attempts_per_thread) {
    for (int i = 0; i < num_threads; ++i) {
        if (pthread_create(&threads[i], NULL, calculate_pi, &attempts_per_thread)) {
            fprintf(stderr, "Error: unable to create thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
}

void join_threads(pthread_t* threads, int num_threads) {
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }
}

void initialize_mutex() {
    if (pthread_mutex_init(&mutex_lock, NULL) != 0) {
        fprintf(stderr, "Error: unable to initialize mutex\n");
        exit(EXIT_FAILURE);
    }
}

void destroy_mutex() {
    pthread_mutex_destroy(&mutex_lock);
}

double estimate_pi(int total_attempts) {
    double pi_estimation = 4.0 * (double)global_hits / (double)total_attempts;
    printf("Estimated value of Pi: %f\n", pi_estimation);
    return pi_estimation;
}

void log_results_to_file(int num_threads, int total_attempts, double execution_time, double pi_estimation) {
    FILE* file = fopen("results.txt", "a");
    if (file == NULL) {
        fprintf(stderr, "Error: unable to open file for logging\n");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%d, %d, %f, %f\n", num_threads, total_attempts, execution_time, pi_estimation);
    fclose(file);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <num_threads> <num_attempts>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = atoi(argv[1]);
    total_attempts = atoi(argv[2]);

    if (num_threads <= 0 || total_attempts <= 0) {
        fprintf(stderr, "Error: both number of threads and number of attempts must be positive integers.\n");
        return EXIT_FAILURE;
    }

    pthread_t threads[num_threads];
    int attempts_per_thread = total_attempts / num_threads;

    double start, finish;

    GET_TIME(start);

    initialize_mutex();
    create_threads(threads, num_threads, attempts_per_thread);
    join_threads(threads, num_threads);
    double pi_estimation = estimate_pi(total_attempts);
    destroy_mutex();

    GET_TIME(finish);

    double execution_time = finish - start;

    printf("Execution time: %f seconds\n", execution_time);

    log_results_to_file(num_threads, total_attempts, execution_time, pi_estimation);

    return EXIT_SUCCESS;
}
