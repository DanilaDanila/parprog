#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>

#include "src/common.h"

/* basic print functions */

// void mpprint(struct mul_params_t *mp) {
//     fprintf(
//         stderr,
//         "struct mul_params_t {\n"
//         "  uint32_t *A;\n"
//         "  uint32_t *B_T;\n"
//         "  uint32_t *M;\n"
//         "\n"
//         "  uint16_t A_nrows = %d;\n"
//         "  union {\n"
//         "    uint16_t A_ncols = %d;\n"
//         "    uint16_t B_nrows = %d;\n"
//         "  };\n"
//         "  uint16_t B_ncols = %d;\n"
//         "\n"
//         "  uint16_t A_first_row = %d;\n"
//         "  uint16_t A_last_row = %d;\n"
//         "\n"
//         "  union {\n"
//         "    uint16_t A_first_col = %d;\n"
//         "    uint16_t B_first_row = %d;\n"
//         "  };\n"
//         "\n"
//         "  union {\n"
//         "    uint16_t A_last_col = %d;\n"
//         "    uint16_t B_last_row = %d;\n"
//         "  };\n"
//         "\n"
//         "  uint16_t B_first_col = %d;\n"
//         "  uint16_t B_last_col = %d;\n"
//         "};\n\n",
//         mp->A_nrows, mp->A_ncols, mp->B_nrows, mp->B_ncols,
//         mp->A_first_row, mp->A_last_row, mp->A_first_col, mp->B_first_row,
//         mp->A_last_col, mp->B_last_row, mp->B_first_col, mp->B_last_col
//     );
// }

uint32_t *createRandomMatrix(uint16_t nrows, uint16_t ncols) {
    uint32_t *M = malloc(sizeof(uint32_t) * nrows * ncols);

    for (uint32_t i = 0; i < nrows * ncols; ++i) {
        M[i] = rand() % 1000;
    }

    return M;
}

enum stored_t {
    STORED_NORMAL,
    STORED_TRANSPOSED,
};

void pprint(uint32_t *M, uint16_t nrows, uint16_t ncols, enum stored_t stored) {
    printf("[\n");
    for (uint16_t i = 0; i < nrows; ++i) {
        printf("  [");
        for (uint16_t j = 0; j < ncols; ++j) {
            uint32_t elem;
            if (stored == STORED_NORMAL) {
                elem = M_at(M, nrows, ncols, i, j);
            } else if (stored == STORED_TRANSPOSED) {
                elem = M_T_at(M, nrows, ncols, i, j);
            }

            printf("%5d, ", elem);
        }
        printf("],\n");
    }
    printf("]\n");
}

// ---------- //

int main(int argc, char **argv) {
    srand(time(NULL));

    if (argc < 4) {
        fprintf(stderr, "Parameters count mismatch!\n");
        return 1;
    }

    const int N = atoi(argv[1]), M = atoi(argv[2]), K = atoi(argv[3]);
    uint16_t nthreads = 0;
    uint16_t block_nrows = 0;
    uint16_t block_ncols = 0;

    if (argc >= 5) {
        nthreads = atoi(argv[4]);
    }

    if (argc == 7) {
        block_nrows = atoi(argv[5]);
        block_ncols = atoi(argv[6]);
    }

    struct runner_config_t args = {
        .A = createRandomMatrix(N, M),
        .B_T = createRandomMatrix(M, K),
        .M = malloc(sizeof(uint32_t) * N * K),

        .A_nrows = N,
        .A_ncols = M,
        .B_ncols = K,

        .nthreads = nthreads,

        .block_nrows = block_nrows,
        .block_ncols = block_ncols,
    };

    #ifdef VERBOSE
        pprint(args.A, args.A_nrows, args.A_ncols, STORED_NORMAL);

        #ifdef TRANSPOSED
            pprint(args.B_T, args.B_nrows, args.B_ncols, STORED_TRANSPOSED);
        #else // TRANSPOSED
            pprint(args.B_T, args.B_nrows, args.B_ncols, STORED_NORMAL);
        #endif // TRANSPOSED
    #endif // VERBOSE

    clock_t start, stop;

    struct thread_params_t *tp = mul_prepare(&args);

    mul_spawn(tp);

    /* measure time */
    {
        start = clock();

        mul_run(tp);

        stop = clock();
    }

    mul_join(tp);

    mul_free(tp);

    #ifdef VERBOSE
        pprint(args.M, args.A_nrows, args.B_ncols, STORED_NORMAL);
    #endif // VERBOSE

    free(args.A);
    free(args.B_T);
    free(args.M);

    printf("%ld\n", (unsigned long)stop - start);

    return 0;
}
