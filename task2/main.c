#define _GNU_SOURCE

#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <sched.h>
#include <time.h>
#include <pthread.h>

#define ROUTINE (int)10e5

/* SPIN LOCK */

struct tas_thread_task_t {
    volatile int *acc;

    volatile _Atomic int *is_running;

    unsigned int count;
}; 

void *tas_worker(void *args) {
    struct tas_thread_task_t *tt = args;

    _Atomic int expected;
    atomic_init(&expected, 1);

    while (atomic_exchange(tt->is_running, expected) == 1) {
        sched_yield();
    }

    for (int i = 0; i < tt->count; ++i) {
        for (int j = 0; j < 500; ++j) {}

        ++*(tt->acc);
    }

    atomic_store(tt->is_running, 0);

    return NULL;
}

void tas_test(int N) {
    volatile int acc = 0;
    volatile _Atomic int is_running;
    atomic_init(&is_running, 1);

    struct tas_thread_task_t *tts = malloc(sizeof(struct tas_thread_task_t) * N);

    for (int i = 0; i < N; ++i) {
        tts[i].acc = &acc;
        tts[i].is_running = &is_running;

        tts[i].count = ROUTINE / N;
    }

    tts[N - 1].count = ROUTINE / N + ROUTINE % N;


    pthread_t *threads = malloc(sizeof(pthread_t) * N);

    for (int i = 0; i < N; ++i) {
        pthread_create(threads + i, NULL, tas_worker, tts + i);
    }


    clock_t start, stop;
    start = clock();

    // start
    atomic_store(&is_running, 0);

    // await
    while (acc != ROUTINE) {
        // nop
    }

    stop = clock();

    for (int i = 0; i < N; ++i) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(tts);

    if (acc != ROUTINE) {
        dprintf(2, "error\n");
    }

    printf("%ld\n", (unsigned long)stop - start);
}


/* TICKET LOCK */

struct ticket_thread_task_t {
    volatile int *acc;

    volatile _Atomic int *next;
    volatile _Atomic int *current;

    unsigned int count;
}; 

void *ticket_worker(void *args) {
    struct ticket_thread_task_t *tt = args;

    // enqueue
    int ticket_number = atomic_fetch_add(tt->next, 1);

    // await
    while (atomic_load(tt->current) != ticket_number) {
        sched_yield();
    }

    for (int i = 0; i < tt->count; ++i) {
        for (int j = 0; j < 500; ++j) {}

        ++*(tt->acc);
    }

    atomic_fetch_add(tt->current, 1);

    return NULL;
}

void ticket_test(int N) {
    volatile int acc = 0;
    volatile _Atomic int next;
    volatile _Atomic int current;
    atomic_init(&next, 0);
    atomic_init(&current, -1);

    struct ticket_thread_task_t *tts = malloc(sizeof(struct ticket_thread_task_t) * N);

    for (int i = 0; i < N; ++i) {
        tts[i].acc = &acc;
        tts[i].next = &next;
        tts[i].current = &current;

        tts[i].count = ROUTINE / N;
    }

    tts[N - 1].count = ROUTINE / N + ROUTINE % N;


    pthread_t *threads = malloc(sizeof(pthread_t) * N);

    for (int i = 0; i < N; ++i) {
        pthread_create(threads + i, NULL, ticket_worker, tts + i);
    }

    clock_t start, stop;
    start = clock();

    // start
    atomic_fetch_add(&current, 1);

    // await
    while (acc != ROUTINE) {
        // nop
    }

    stop = clock();

    for (int i = 0; i < N; ++i) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(tts);

    if (acc != ROUTINE) {
        dprintf(2, "error\n");
    }

    printf("%ld\n", (unsigned long)stop - start);
}


int main(int argc, char **argv) {
    
    if (argc < 2) {
        dprintf(2, "argc amount mismatch\n");
        return 1;
    }

    int N = atoi(argv[1]);

    if (argc == 2) {
        tas_test(N);
    } else if (argc == 3) {
        ticket_test(N);
    } else {
        dprintf(2, "argc amount mismatch\n");
        return 1;
    }

    return 0;
}
