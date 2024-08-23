#ifndef markov_SOLUTION_INCLUDED
#define markov_SOLUTION_INCLUDED
#include <cstdlib>
#include "archlab.h"
#include <unistd.h>
#include<cstdint>
#include"function_map.hpp"
#include <omp.h>

//template<typename T>
void __attribute__((noinline)) markov_solution(double *init_change, double **transition_matrix, uint64_t days, uint64_t scale, double *result) {

    double *intermediate_result;
    intermediate_result = (double *)malloc(scale*sizeof(double));
    std::memcpy(intermediate_result, init_change, scale*sizeof(double));

    for(uint64_t i = 0; i < days; i++) {
        std::memset(result, 0, scale*sizeof(double));
        for(uint64_t j = 0; j < scale; j++) {
            for(uint64_t k = 0; k < scale; k++) {
                result[k] += intermediate_result[j]*transition_matrix[k][j];
            }
        }
        std::memcpy(intermediate_result, result, scale*sizeof(double));
    }
    return;
}

#endif
