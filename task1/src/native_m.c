#include "common.h"

/* Native multithreading multiplication */

struct mul_params_t {
    uint32_t *A;
    uint32_t *B_T;
    uint32_t *M;

    uint16_t A_nrows;
    union {
        uint16_t A_ncols;
        uint16_t B_nrows;
    };
    uint16_t B_ncols;

    uint16_t A_first_row;
    uint16_t A_last_row;

    volatile int *is_ready;
    atomic_int *is_finish;
};

struct thread_params_t {
    pthread_t *threads;
    struct mul_params_t *thread_args;

    uint16_t nthreads;

    volatile int *is_ready;
    atomic_int *is_finish;
};

runner_args_t mul_prepare(struct runner_config_t *args) {
    struct thread_params_t *p = malloc(sizeof(struct thread_params_t));

    p->nthreads = args->nthreads;
    
    p->threads = malloc(sizeof(pthread_t) * args->nthreads);
    p->thread_args = malloc(sizeof(struct mul_params_t) * args->nthreads);

    // -----

    p->is_ready = malloc(sizeof(int));
    *p->is_ready = 0;

    p->is_finish = malloc(sizeof(atomic_int));
    *p->is_finish = 0;

    // -----

    int nrows_per_thread = args->A_nrows / args->nthreads;

    uint16_t first_row = 0;
    for (int i = 0; i < args->nthreads; ++i) {
        memcpy(p->thread_args + i, args, sizeof(struct runner_config_t));

        p->thread_args[i].A_first_row = first_row;
        p->thread_args[i].A_last_row = first_row + nrows_per_thread;

        p->thread_args[i].is_ready = p->is_ready;
        p->thread_args[i].is_finish = p->is_finish;

        first_row += nrows_per_thread;
    }

    if (args->A_nrows % args->nthreads != 0) {
        (p->thread_args + args->nthreads - 1)->A_last_row = args->A_nrows;
    }

    return (runner_args_t)p;
}

void mul_free(runner_args_t ra) {
    struct thread_params_t *p = ra;

    free(p->is_finish);
    free((int*)p->is_ready);
    free(p->threads);
    free(p->thread_args);
    free(p);
}

void *worker(void *args_ptr) {
    struct mul_params_t *args = args_ptr;

    // Не 'жгу процессор', а spinlock!)
    while (args->is_ready == 0) {
        // nop
    }

    for (uint16_t i = args->A_first_row; i < args->A_last_row; ++i) {
        for (uint16_t j = 0; j < args->B_ncols; ++j) {
            M_at(args->M, args->A_nrows, args->B_ncols, i, j) = 0;

            for (uint16_t k = 0; k < args->A_ncols; ++k) {
                M_at(args->M, args->A_nrows, args->B_ncols, i, j) +=
                    M_at(args->A, args->A_nrows, args->A_ncols, i, k) *
                    M_T_at(args->B_T, args->B_nrows, args->B_ncols, k, j);
            }
        }
    }

    atomic_fetch_add(args->is_finish, 1);

    return NULL;
}

void mul_spawn(runner_args_t ra) {
    struct thread_params_t *thread_params = ra;

    for (int i = 0; i < thread_params->nthreads; ++i) {
        pthread_create(thread_params->threads + i, NULL, worker, thread_params->thread_args + i);
    }
}

void mul_run(runner_args_t ra) {
    struct thread_params_t *thread_params = ra;

    // start caclulation
    *thread_params->is_ready = 1;

    // wait for end
    // Не 'жгу процессор', а второй spinlock!)
    while (*thread_params->is_finish != thread_params->nthreads) {
        // nop
    }
}

void mul_join(runner_args_t ra) {
    struct thread_params_t *thread_params = ra;

    for (int i = 0; i < thread_params->nthreads; ++i) {
        pthread_join(thread_params->threads[i], NULL);
    }
}
