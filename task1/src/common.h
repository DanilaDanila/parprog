#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>


#ifdef TRANSPOSED
    #define M_at(M, nrows, ncols, row, col) ((M)[(row) * (ncols) + (col)])
    #define M_T_at(M_T, nrows, ncols, row, col) ((M_T)[(col) * (nrows) + (row)])
#else
    #define M_at(M, nrows, ncols, row, col) ((M)[(row) * (ncols) + (col)])
    #define M_T_at(M_T, nrows, ncols, row, col) ((M_T)[(row) * (ncols) + (col)])  // same
#endif // TRANSPOSED


#define MIN(a, b) (((a) <= (b)) ? (a) : (b))


typedef void* runner_args_t;


/* template functions */

struct runner_config_t {
    uint32_t *A;
    uint32_t *B_T;
    uint32_t *M;

    uint16_t A_nrows;
    union {
        uint16_t A_ncols;
        uint16_t B_nrows;
    };
    uint16_t B_ncols;

    uint16_t nthreads;
    uint16_t block_nrows;
    uint16_t block_ncols;
};

runner_args_t mul_prepare(struct runner_config_t*);

void mul_spawn(runner_args_t);

void mul_run(runner_args_t);

void mul_join(runner_args_t);

void mul_free(runner_args_t);
