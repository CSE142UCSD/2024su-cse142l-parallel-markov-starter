#include"markov_reference.hpp"
#include"markov_solution.hpp"
#include <vector>

#define ELEMENT_TYPE uint64_t


extern "C"
void markov_reference_c(double *init_change, double **transition_matrix, uint64_t days, uint64_t scale, double *result)
{
    markov_reference(init_change, transition_matrix, days, scale, result);
}
FUNCTION(markov, markov_reference_c);


extern "C"
void markov_solution_c(double *init_change, double **transition_matrix, uint64_t days, uint64_t scale, double *result)
{
    markov_solution(init_change, transition_matrix, days, scale, result);
}
FUNCTION(markov, markov_solution_c);
