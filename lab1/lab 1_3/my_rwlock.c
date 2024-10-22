#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_rand.h"
#include "timer.h"

const int MAX_KEY = 100000000;

typedef struct {
    pthread_mutex_t mutex;         
    pthread_cond_t readers;        
    pthread_cond_t writers;        
    int reader_count;              
    int writer_count;              
    int waiting_readers;           
    int waiting_writers;           
    int writer_present;            
} rwlock_t;

void rwlock_init(rwlock_t* rw) {
    pthread_mutex_init(&(rw->mutex), NULL);
    pthread_cond_init(&(rw->readers), NULL);
    pthread_cond_init(&(rw->writers), NULL);
    rw->reader_count = 0;
    rw->writer_count = 0;
    rw->waiting_readers = 0;
    rw->waiting_writers = 0;
    rw->writer_present = 0;
}

void rwlock_destroy(rwlock_t* rw) {
    pthread_mutex_destroy(&(rw->mutex));
    pthread_cond_destroy(&(rw->readers));
    pthread_cond_destroy(&(rw->writers));
}

void rwlock_rdlock(rwlock_t* rw) {
    pthread_mutex_lock(&(rw->mutex));
    rw->waiting_readers++;
    
    while (rw->writer_present || rw->waiting_writers > 0) {
        pthread_cond_wait(&(rw->readers), &(rw->mutex));
    }
    
    rw->waiting_readers--;
    rw->reader_count++;
    pthread_mutex_unlock(&(rw->mutex));
}

void rwlock_wrlock(rwlock_t* rw) {
    pthread_mutex_lock(&(rw->mutex));
    rw->waiting_writers++;
    
    while (rw->writer_present || rw->reader_count > 0) {
        pthread_cond_wait(&(rw->writers), &(rw->mutex));
    }

    rw->waiting_writers--;
    rw->writer_present = 1;
    pthread_mutex_unlock(&(rw->mutex));
}

void rwlock_unlock(rwlock_t* rw) {
    pthread_mutex_lock(&(rw->mutex));

    if (rw->writer_present) {
        rw->writer_present = 0;
    } else {
        rw->reader_count--;
    }

    if (rw->waiting_writers > 0) {
        pthread_cond_signal(&(rw->writers));
    } else if (rw->waiting_readers > 0) {
        pthread_cond_broadcast(&(rw->readers));
    }

    pthread_mutex_unlock(&(rw->mutex));
}

rwlock_t rwlock;

struct list_node_s {
    int data;
    struct list_node_s* next;
};

struct list_node_s* head = NULL;
pthread_mutex_t count_mutex;
int total_ops, member_count = 0, insert_count = 0, delete_count = 0;
double search_percent, insert_percent, delete_percent;
int thread_count; // Объявляем переменную thread_count

// Вставка узла в список
int Insert(int value) {
    struct list_node_s* pred = NULL;
    struct list_node_s* curr = head;
    struct list_node_s* temp;

    while (curr != NULL && curr->data < value) {
        pred = curr;
        curr = curr->next;
    }

    if (curr == NULL || curr->data > value) {
        temp = malloc(sizeof(struct list_node_s));
        temp->data = value;
        temp->next = curr;
        if (pred == NULL)
            head = temp;
        else
            pred->next = temp;
        return 1;
    } else {
        return 0;
    }
}

// Проверка наличия узла в списке
int Member(int value) {
    struct list_node_s* temp = head;
    while (temp != NULL && temp->data < value)
        temp = temp->next;

    return (temp != NULL && temp->data == value);
}

// Удаление узла из списка
int Delete(int value) {
    struct list_node_s* pred = NULL;
    struct list_node_s* curr = head;

    while (curr != NULL && curr->data < value) {
        pred = curr;
        curr = curr->next;
    }

    if (curr != NULL && curr->data == value) {
        if (pred == NULL)
            head = curr->next;
        else
            pred->next = curr->next;
        free(curr);
        return 1;
    } else {
        return 0;
    }
}

void* Thread_work(void* rank) {
    long my_rank = (long)rank;
    int i, val;
    double which_op;
    unsigned seed = my_rank + 1;
    int my_member_count = 0, my_insert_count = 0, my_delete_count = 0;
    int ops_per_thread = total_ops / thread_count;

    for (i = 0; i < ops_per_thread; i++) {
        which_op = my_drand(&seed);
        val = my_rand(&seed) % MAX_KEY;

        if (which_op < search_percent) {
            rwlock_rdlock(&rwlock);
            Member(val);
            rwlock_unlock(&rwlock);
            my_member_count++;
        } else if (which_op < search_percent + insert_percent) {
            rwlock_wrlock(&rwlock);
            Insert(val);
            rwlock_unlock(&rwlock);
            my_insert_count++;
        } else {
            rwlock_wrlock(&rwlock);
            Delete(val);
            rwlock_unlock(&rwlock);
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

void Free_list() {
    struct list_node_s* current = head;
    struct list_node_s* next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    head = NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number of threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    long thread;
    pthread_t* thread_handles;
    double start, finish, elapsed;

    thread_count = strtol(argv[1], NULL, 10);
    total_ops = 100000; 

    thread_handles = malloc(thread_count * sizeof(pthread_t));
    pthread_mutex_init(&count_mutex, NULL);
    rwlock_init(&rwlock);

    // Замер времени для собственной реализации rwlock
    GET_TIME(start);

    for (thread = 0; thread < thread_count; thread++)
        pthread_create(&thread_handles[thread], NULL, Thread_work, (void*)thread);

    for (thread = 0; thread < thread_count; thread++)
        pthread_join(thread_handles[thread], NULL);

    GET_TIME(finish);
    elapsed = finish - start;
    printf("Собственная реализация rwlock: Время выполнения = %f секунд\n", elapsed);

    free(thread_handles);
    rwlock_destroy(&rwlock);
    pthread_mutex_destroy(&count_mutex);
    Free_list();
    return 0;
}
