#pragma once
#include <string>

struct Config {
    int num_cpu;
    std::string scheduler;
    int quantum_cycles;
    int batch_process_freq;
    int min_ins;
    int max_ins;
    int delays_per_exec;
    size_t max_overall_mem;
    size_t mem_per_frame;
    size_t min_mem_per_proc;
    size_t max_mem_per_proc;
};

extern Config cfg;
