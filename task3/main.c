#define _GNU_SOURCE

#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>


void may_be_yield() {
    if (rand() % 3 == 0) {
        sched_yield();
    }
}


struct node_t {
    _Atomic unsigned version;
    _Atomic struct node_t *next;

    void *value;
};


struct LFStack {
    _Atomic struct node_t *head;

    _Atomic unsigned version_enc;
};


void LFStack_Init(struct LFStack *lfs) {
    atomic_init(&lfs->head, (_Atomic struct node_t*)NULL);
    atomic_init(&lfs->version_enc, 0);
}


void LFStack_Push(struct LFStack *lfs, void *value) {
    struct node_t *new_node = malloc(sizeof(struct node_t));

    for (;;) {
        atomic_store(&new_node->version, atomic_fetch_add(&lfs->version_enc, 1));
        atomic_store(&new_node->next, (_Atomic struct node_t*)atomic_load(&lfs->head));
        new_node->value = value;

        if (atomic_compare_exchange_weak(&lfs->head, &new_node->next, (_Atomic struct node_t *)new_node)) {
            return;
        }

        sched_yield();
    }
}


void *LFStack_Pop(struct LFStack *lfs) {
    _Atomic struct node_t *head;

    for (;;) {
        head = atomic_load(&lfs->head);

        if (head == NULL) {
            return NULL;
        }

        _Atomic struct node_t *next_node = atomic_load(&((struct node_t *)head)->next);

        if (atomic_compare_exchange_weak(&lfs->head, &head, next_node)) {
            break;
        }
    }

    void *value = ((struct node_t *)head)->value;
    free(head);

    return value;
}


/* JUST WORKS SINGLE THREAD */

void single_thread_test() {
    int N = 1000;
    int test_ok = 1;

    struct LFStack lfs;
    LFStack_Init(&lfs);

    for (uint64_t i = 1; i < N; ++i) {
        LFStack_Push(&lfs, (void*)i);
    }

    void *value;
    for (uint64_t i = N - 1; i > 0; --i) {
        value = LFStack_Pop(&lfs);

        if ((uint64_t)value != i) {
            test_ok = 0;
            dprintf(2, "expected: %d\ngot: %d\n\n", i, (uint64_t)value);
        }
    }

    if (test_ok) {
        dprintf(2, "Single thread test: passed\n");
    } else {
        dprintf(2, "Single thread test: failed!\n");
    }
}


/* JUST WORKS MULTYTHREAD TEST */

struct mt_worker_args_t {
    struct LFStack *lfs;

    _Atomic int *barrier;

    uint64_t from;
    uint64_t to;
};

void *mt_worker(void *args) {
    struct mt_worker_args_t *wargs = args;

    while (atomic_load(wargs->barrier) == 0) {
        // nop
    }

    for (uint64_t i = wargs->from; i < wargs->to; ++i) {
        LFStack_Push(wargs->lfs, (void*)i);
        may_be_yield();
    }
}

void mt_test() {
    uint64_t N = 500;

    struct LFStack lfs;
    LFStack_Init(&lfs);

    _Atomic int barrier;
    atomic_init(&barrier, 0);

    struct mt_worker_args_t *mtwa = malloc(sizeof(struct mt_worker_args_t) * N);
    for (uint64_t i = 0; i < N; ++i) {
        mtwa[i].lfs = &lfs;
        mtwa[i].from = N * i + 1;
        mtwa[i].to = N * (i + 1) + 1;
        mtwa[i].barrier = &barrier;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * N);
    for (int i = 0; i < N; ++i) {
        pthread_create(threads + i, NULL, mt_worker, mtwa + i);
    }

    atomic_store(&barrier, 1);

    for (int i = 0; i < N; ++i) {
        pthread_join(threads[i], NULL);
    }

    uint64_t sum = 0;
    for (void *value = LFStack_Pop(&lfs); value != NULL; value = LFStack_Pop(&lfs)) {
        sum += (uint64_t)value;
    }

    uint64_t expected = (1 + N * N) * (N * N) / 2;

    if (sum == expected) {
        dprintf(2, "Multythread test: passed\n");
    } else {
        dprintf(2, "Multythread test: failed!\n");
        dprintf(2, "Expected: %ld\nGot: %ld\n\n", expected, sum);
    }
}


/* ABA TEST */

struct aba_worker_args_t {
    struct LFStack *lfs;

    _Atomic int *barrier;

    uint64_t count;
};

void *aba_worker(void *args) {
    struct aba_worker_args_t *wargs = args;

    while (atomic_load(wargs->barrier) == 0) {
        // nop
    }

    for (int i = 0; i < wargs->count; ++i) {
        LFStack_Push(wargs->lfs, (void*)1);

        may_be_yield();

        LFStack_Pop(wargs->lfs);

        may_be_yield();

        LFStack_Push(wargs->lfs, (void*)1);
    }
}

void aba_test() {
    uint64_t N = 1000;

    struct LFStack lfs;
    LFStack_Init(&lfs);

    _Atomic int barrier;
    atomic_init(&barrier, 0);

    struct aba_worker_args_t *abawa = malloc(sizeof(struct aba_worker_args_t) * N);
    for (uint64_t i = 0; i < N; ++i) {
        abawa[i].lfs = &lfs;
        abawa[i].barrier = &barrier;
        abawa[i].count = N;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * N);
    for (int i = 0; i < N; ++i) {
        pthread_create(threads + i, NULL, aba_worker, abawa + i);
    }

    atomic_store(&barrier, 1);

    for (int i = 0; i < N; ++i) {
        pthread_join(threads[i], NULL);
    }

    uint64_t count = 0;
    for (void *value = LFStack_Pop(&lfs); value != NULL; value = LFStack_Pop(&lfs)) {
        ++count;
    }


    if (count == N * N) {
        dprintf(2, "ABA test: passed\n");
    } else {
        dprintf(2, "ABA test: failed!\n");
        dprintf(2, "Expected: %d\nGot: %d\n\n", N * N, count);
    }
}


int main() {
    single_thread_test();

    mt_test();

    aba_test();

    return 0;
}
