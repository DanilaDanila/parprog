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

    volatile int *is_ready;
    atomic_int *is_finish;
};

struct thread_params_t {
    pthread_t *threads;
    struct mul_params_t *thread_args;

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

    tp->threads = NULL;
    tp->thread_args = malloc(sizeof(struct mul_params_t) * nblocks);

    tp->nthreads = 0;
    tp->nblocks = nblocks;

    for (uint32_t i = 0; i < nblocks; ++i) {
        uint16_t iblock_row = i % nblocks_in_row;
        uint16_t iblock_col = i / nblocks_in_row;

        memcpy(tp->thread_args + i, mp, sizeof(struct runner_config_t));

        tp->thread_args[i].A_first_col = 0;
        tp->thread_args[i].A_last_col = mp->A_ncols;

        tp->thread_args[i].A_first_row = iblock_row * block_nrows;
        tp->thread_args[i].A_last_row = MIN(tp->thread_args[i].A_first_row + block_nrows, mp->A_nrows);

        tp->thread_args[i].B_first_col = iblock_col * block_ncols;
        tp->thread_args[i].B_last_col = MIN(tp->thread_args[i].B_first_col + block_ncols, mp->B_ncols);
    }

    return (runner_args_t)tp;
}

void mul_spawn(runner_args_t) {
    // pass
}

void worker(struct mul_params_t *args) {
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
}

void mul_run(runner_args_t ra) {
    struct thread_params_t *tp = ra;

    for (uint16_t i = 0; i < tp->nblocks; ++i) {
        worker(tp->thread_args + i);
    }
}

void mul_join(runner_args_t) {
    // pass
}

void mul_free(runner_args_t ra) {
    struct thread_params_t *tp = ra;

    free(tp->thread_args);
    free(tp);
}
