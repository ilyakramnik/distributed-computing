#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_rand.h"
#include "timer.h"

// Библиотечная rwlock
pthread_rwlock_t rwlock;

void* Thread_work(void* rank) {
    long my_rank = (long) rank;
    int i, val;
    double which_op;
    unsigned seed = my_rank + 1;
    int my_member_count = 0, my_insert_count = 0, my_delete_count = 0;
    int ops_per_thread = total_ops / thread_count;

    for (i = 0; i < ops_per_thread; i++) {
        which_op = my_drand(&seed);
        val = my_rand(&seed) % MAX_KEY;

        if (which_op < search_percent) {
            pthread_rwlock_rdlock(&rwlock);
            Member(val);
            pthread_rwlock_unlock(&rwlock);
            my_member_count++;
        } else if (which_op < search_percent + insert_percent) {
            pthread_rwlock_wrlock(&rwlock);
            Insert(val);
            pthread_rwlock_unlock(&rwlock);
            my_insert_count++;
        } else {
            pthread_rwlock_wrlock(&rwlock);
            Delete(val);
            pthread_rwlock_unlock(&rwlock);
            my_delete_count++;
        }
    }

    pthread_mutex_lock(&count_mutex);
    member_count += my_member_count;
    insert_count += my_insert_count;
    delete_count += my_delete_count;
    pthread_mutex_unlock(&count_mutex);

    return NULL;
}

int main(int argc, char* argv[]) {
    long thread;
    pthread_t* thread_handles;
    double start, finish, elapsed;

    thread_count = strtol(argv[1], NULL, 10);
    total_ops = 100000; // Число операций

    thread_handles = malloc(thread_count * sizeof(pthread_t));
    pthread_mutex_init(&count_mutex, NULL);
    pthread_rwlock_init(&rwlock, NULL);

    GET_TIME(start);

    for (thread = 0; thread < thread_count; thread++)
        pthread_create(&thread_handles[thread], NULL, Thread_work, (void*) thread);

    for (thread = 0; thread < thread_count; thread++)
        pthread_join(thread_handles[thread], NULL);

    GET_TIME(finish);
    elapsed = finish - start;
    printf("Библиотечная реализация rwlock: Время выполнения = %f секунд\n", elapsed);

    free(thread_handles);
    pthread_rwlock_destroy(&rwlock);
    pthread_mutex_destroy(&count_mutex);

    return 0;
}
