#include "common.h"

/* native multiplication */

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
};

runner_args_t mul_prepare(struct runner_config_t *mp) {
    return (runner_args_t)mp;
}

void mul_spawn(runner_args_t) {
    // pass
}

void mul_run(runner_args_t ra) {
    struct mul_params_t *args = (struct mul_params_t*)ra;

    for (uint16_t i = 0; i < args->A_nrows; ++i) {
        for (uint16_t j = 0; j < args->B_ncols; ++j) {
            M_at(args->M, args->A_nrows, args->B_ncols, i, j) = 0;

            for (uint16_t k = 0; k < args->A_ncols; ++k) {
                M_at(args->M, args->A_nrows, args->B_ncols, i, j) +=
                    M_at(args->A, args->A_nrows, args->A_ncols, i, k) *
                    M_T_at(args->B_T, args->B_nrows, args->B_ncols, k, j);
            }
        }
    }
}

void mul_join(runner_args_t) {
    // pass
}

void mul_free(runner_args_t) {
    // pass
}
