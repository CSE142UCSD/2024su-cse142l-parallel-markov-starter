#include <cstdlib>
#include "archlab.h"
#include <unistd.h>
#include <algorithm>
#include <cstdint>
#include "function_map.hpp"
#include <dlfcn.h>
#include <vector>
#include <sstream>
#include <string>
#include "perfstats.h"
#include <omp.h>

#define ELEMENT_TYPE uint64_t

uint array_size;

typedef void(*markov_impl)(double *init_change, double **transition_matrix, uint64_t days, uint64_t scale, double *result);

double *init_change_vector(uint64_t scale, uint64_t *seed);
double **init_transition_matrix(uint64_t scale, uint64_t *seed);
double *init_result(uint64_t scale);
bool compare_data(double *M, double *N, uint64_t size);
void dump_vector(double *vector, uint64_t scale);
void dump_matrix(double **matrix, uint64_t scale);
int main(int argc, char *argv[])
{
    int i, reps=1, iterations=1,mhz, verify =0;
    char *stat_file = NULL;
    char default_filename[] = "stats.csv";
    char preamble[1024];
    char epilogue[1024];
    char header[1024];
    std::stringstream clocks;
    std::vector<std::string> functions;
    std::vector<std::string> default_functions;
    std::vector<int> mhz_s;
    std::vector<int> default_mhz;
    std::vector<unsigned long int> days;
    std::vector<unsigned long int> default_days;
    std::vector<unsigned long int> default_scales;
    std::vector<unsigned long int> scales;

    default_days.push_back(365);
    default_scales.push_back(1024);
    double *init_change; 
    double **transition_matrix;
    double *result, *verification;
    uint64_t scale, day, seed;
    float minv = -1.0;
    float maxv = 1.0;
    std::vector<uint64_t> seeds;
    std::vector<uint64_t> default_seeds;
    default_seeds.push_back(0xDEADBEEF);
    
    for(i = 1; i < argc; i++)
    {
            // This is an option.
        if(argv[i][0]=='-')
        {
            switch(argv[i][1])
            {
                case 'o':
                    if(i+1 < argc && argv[i+1][0]!='-')
                        stat_file = argv[i+1];
                    break;
                case 'r':
                    if(i+1 < argc && argv[i+1][0]!='-')
                        reps = atoi(argv[i+1]);
                    break;
                case 's':
                    for(;i+1<argc;i++)
                    {
                        if(argv[i+1][0]!='-')
                        {
                            scale = atoi(argv[i+1]);
                            scales.push_back(scale);
                        }
                        else
                            break;
                    }
                    break;
                case 'd':
                    for(;i+1<argc;i++)
                    {
                        if(argv[i+1][0]!='-')
                        {
                            day = atoi(argv[i+1]);
                            days.push_back(day);
                        }
                        else
                            break;
                    }
                    break;
                case 'M':
                    for(;i+1<argc;i++)
                    {
                        if(argv[i+1][0]!='-')
                        {
                            mhz = atoi(argv[i+1]);
                            mhz_s.push_back(mhz);
                        }
                        else
                            break;
                    }
                    break;
                case 'f':
                    for(;i+1<argc;i++)
                    {
                        if(argv[i+1][0]!='-')
                        {
                            functions.push_back(std::string(argv[i+1]));
                        }
                    else
                        break;
                    }
                    break;
                case 'i':
                    if(i+1 < argc && argv[i+1][0]!='-')
                        iterations = atoi(argv[i+1]);
                    break;
                case 'h':
                    std::cout << "-s set the size of the source matrix.\n-k set the kernel matrix size.\n-f what functions to run.\n-d sets the random seed.\n-o sets where statistics should go.\n-i sets the number of iterations.\n-v compares the result with the reference solution.\n";
                    break;
                case 'v':
                    verify = 1;
                    break;
                }
            }
        }
    if(stat_file==NULL)
        stat_file = default_filename;

    if (std::find(functions.begin(), functions.end(), "ALL") != functions.end()) {
        functions.clear();
        for(auto & f : function_map::get()) {
            functions.push_back(f.first);
        }
    }
    
    for(auto & function : functions) {
        auto t= function_map::get().find(function);
        if (t == function_map::get().end()) {
            std::cerr << "Unknown function: " << function <<"\n";
            exit(1);
        }
        std::cerr << "Gonna run " << function << "\n";
    }
    if(scales.size()==0)
        scales = default_scales;
    if(seeds.size()==0)
        seeds = default_seeds;
    if(functions.size()==0)
        functions = default_functions;

    if(verify == 1)
                sprintf(header,"days,change_scale,function,IC,Cycles,CPI,CT,ET,L1_dcache_miss_rate,L1_dcache_misses,L1_dcache_accesses,correctness");
    else
        sprintf(header,"days,change_scale,function,IC,Cycles,CPI,CT,ET,L1_dcache_miss_rate,L1_dcache_misses,L1_dcache_accesses");
    perfstats_print_header(stat_file, header);
    omp_set_num_threads(4);
    for(auto & seed: seeds ) {
        for(auto & scale: scales) {
            for(auto & day: days) {
                init_change = init_change_vector(scale, &seed);
                transition_matrix = init_transition_matrix(scale, &seed);
                for(auto & function : functions) {
                    result = init_result(scale);
                    std::cerr << "Running: " << function << "\n";
                    function_spec_t f_spec = function_map::get()[function];
                    auto fut = reinterpret_cast<markov_impl>(f_spec.second);
                    sprintf(preamble, "%lu,%lu,%s,",day, scale,function.c_str());
                    perfstats_init();
                    perfstats_enable();
                    fut(init_change, transition_matrix, day, scale, result);
                    perfstats_disable();
//                    dump_vector(result, scale);
                    if(verify)
                    {
                        if(function.find("markov_solution_c") != std::string::npos)
                        {
                            function_spec_t t = function_map::get()[std::string("markov_reference_c")];
                            auto verify_fut = reinterpret_cast<markov_impl>(t.second);                        
                            verification =  init_result(scale);
                            verify_fut(init_change, transition_matrix, day, scale, verification);
                            if(compare_data(result,verification,scale))
                            {
                                std::cerr << "Passed!!\n";
                                sprintf(epilogue,",1\n");
                            }
                            else
                            {
                                std::cerr << "Reference solution does not agree with your optimization!\n";
                                sprintf(epilogue,",-1\n");
                            }
                            free(verification);
                        }
                        else
                            sprintf(epilogue,",0\n");
                    }
                    else
                        sprintf(epilogue,"\n");
                    perfstats_print(preamble, stat_file, epilogue);
                    perfstats_deinit();
                    std::cerr << "Done execution: " << function << "\n";
                    free(result);
                }
                free(init_change);
                free(transition_matrix);
            }
        }
    }
    return 0;
}

// In case you want to debug
void dump_matrix(double **matrix, uint64_t scale)
{
    for(uint64_t i=0;i<scale;i++)
    {
        for(uint64_t j=0;j<scale;j++)
            std::cout << matrix[i][j] << ",";
        std::cout << "\n";
    }    
}
void dump_vector(double *vector, uint64_t scale)
{
    for(uint64_t i=0;i<scale;i++)
    {
        std::cout << vector[i] << ",";
    }    
    std::cout << "\n";
}

//START_INIT
double *init_change_vector(uint64_t scale, uint64_t *seed)
{
    double *data = (double *)calloc(scale,sizeof(double));
    data[(fast_rand(seed) % scale)]=1.0;
//    dump_vector(data,scale);
    return data;
}


double **init_transition_matrix(uint64_t scale, uint64_t *seed)
{
    double **data, *elements;
    uint64_t i,j;
    double remaining_probabilities=1;
    data = (double **)malloc(scale*sizeof(double*));
    elements = (double *)malloc(scale*scale*sizeof(double));
    for(i=0;i<scale;i++)
        data[i] = &elements[i*scale];

    for(i=0;i<scale;i++)
    {
        remaining_probabilities=1;
        for(j=0;j<scale-1;j++)
        {
            data[j][i] = remaining_probabilities*(double)(fast_rand(seed) % 1000000)/1000000.0;
            remaining_probabilities -= data[j][i];
        }
        data[j][i] = remaining_probabilities;
    }
//    dump_matrix(data,scale);
    return data;
}


double *init_result(uint64_t scale)
{
    double *data = (double *)calloc(scale,sizeof(double));
    return data;
}
//END_INIT

bool compare_data(double *M, double *N, uint64_t size)
{
    uint64_t i,j;
    bool result = true;
    //double diff = 0.0; 
    for(i=0;i<size;i++)
    {
        if(std::abs(M[i]-N[i])>0.0001)
        {
            std::cout << "Verification failed at " << i <<"th element\n" << M[i]<< " v.s "<< N[i] << "\n";
            result = false;
        }
//        else
//            std::cout << "Verification correct at " << i <<"th element\n" << M[i]<< " v.s "<< N[i] << "\n";
    }
    return result;
}