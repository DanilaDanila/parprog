#include "common.h"

/* block multiplaction */

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

    union {
        uint16_t A_first_col;
        uint16_t B_first_row;
    };

    union {
        uint16_t A_last_col;
        uint16_t B_last_row;
    };

    uint16_t B_first_col;
    uint16_t B_last_col;
};

enum {
    STATUS_WAITING = 0,
    STATUS_PROCESSING = 1,
    STATUS_EXIT = 2,
};

struct worker_args_t {
    struct mul_params_t *thread_args;

    atomic_int status;
};

struct thread_params_t {
    pthread_t *threads;
    struct worker_args_t *wa;
    struct mul_params_t *all_blocks;

    uint16_t nthreads;
    uint16_t nblocks;

    volatile int *is_ready;
    atomic_int *is_finish;
};

runner_args_t mul_prepare(struct runner_config_t *mp) {
    // const uint16_t nthreads = *((uint16_t*)&param);
    const uint16_t block_nrows = mp->block_nrows;
    const uint16_t block_ncols = mp->block_ncols;

    const uint16_t nblocks_in_row = (mp->A_nrows + block_nrows - 1) / block_nrows;
    const uint16_t nblocks_in_col = (mp->B_ncols + block_ncols - 1) / block_ncols;
    const uint32_t nblocks = nblocks_in_row * nblocks_in_col;
    
    struct thread_params_t *tp = malloc(sizeof(struct thread_params_t));

    tp->threads = malloc(sizeof(pthread_t) * mp->nthreads);
    tp->wa = malloc(sizeof(struct worker_args_t) * mp->nthreads);
    tp->all_blocks = malloc(sizeof(struct mul_params_t) * nblocks);

    tp->nthreads = mp->nthreads;
    tp->nblocks = nblocks;

    for (uint32_t i = 0; i < nblocks; ++i) {
        uint16_t iblock_row = i % nblocks_in_row;
        uint16_t iblock_col = i / nblocks_in_row;

        memcpy(tp->all_blocks + i, mp, sizeof(struct runner_config_t));

        tp->all_blocks[i].A_first_col = 0;
        tp->all_blocks[i].A_last_col = mp->A_ncols;

        tp->all_blocks[i].A_first_row = iblock_row * block_nrows;
        tp->all_blocks[i].A_last_row = MIN(tp->all_blocks[i].A_first_row + block_nrows, mp->A_nrows);

        tp->all_blocks[i].B_first_col = iblock_col * block_ncols;
        tp->all_blocks[i].B_last_col = MIN(tp->all_blocks[i].B_first_col + block_ncols, mp->B_ncols);
    }

    return (runner_args_t)tp;
}

void* worker(void *v_args) {
    struct worker_args_t *wa = v_args;

    for (;;) {
        while (wa->status == STATUS_WAITING) {
            // nop
        }

        if (wa->status == STATUS_EXIT) {
            break;
        }

        struct mul_params_t *args = wa->thread_args;

        for (uint16_t i = args->A_first_row; i < args->A_last_row; ++i) {
            for (uint16_t j = args->B_first_col; j < args->B_last_col; ++j) {
                M_at(args->M, args->A_nrows, args->B_ncols, i, j) = 0;

                for (uint16_t k = args->A_first_col; k < args->A_last_col; ++k) {
                    M_at(args->M, args->A_nrows, args->B_ncols, i, j) +=
                        M_at(args->A, args->A_nrows, args->A_ncols, i, k) *
                        M_T_at(args->B_T, args->B_nrows, args->B_ncols, k, j);
                }
            }
        }

        wa->status = STATUS_WAITING;
    }

    return NULL;
}

void mul_spawn(runner_args_t ra) {
    struct thread_params_t *thread_params = ra;
    
    for (int i = 0; i < thread_params->nthreads; ++i) {
        thread_params->wa->status = STATUS_WAITING;

        pthread_create(thread_params->threads + i, NULL, worker, thread_params->wa + i);
    }
}

void mul_run(runner_args_t ra) {
    struct thread_params_t *tp = ra;

    uint16_t i_worker = 0;
    for (uint16_t i = 0; i < tp->nblocks; ++i) {
        while (tp->wa[i_worker].status != STATUS_WAITING) {
            i_worker = (i_worker + 1) % tp->nthreads;
        }

        tp->wa[i_worker].thread_args = tp->all_blocks + i;
        tp->wa[i_worker].status = STATUS_PROCESSING;
    }

    for (uint16_t i = 0; i < tp->nthreads; ++i) {
        while (tp->wa[i].status == STATUS_PROCESSING) {
            // nop
        }

        tp->wa[i].status = STATUS_EXIT;
    }
}

void mul_join(runner_args_t ra) {
    struct thread_params_t *thread_params = ra;

    for (int i = 0; i < thread_params->nthreads; ++i) {
        pthread_join(thread_params->threads[i], NULL);
    }
}

void mul_free(runner_args_t ra) {
    struct thread_params_t *tp = ra;

    free(tp->all_blocks);
    free(tp->wa);
    free(tp->threads);
    free(tp);
}
