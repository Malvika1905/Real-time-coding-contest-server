#ifndef MEMORY_SIM_H
#define MEMORY_SIM_H

#include <vector>
#include <string>
#include <cstdint>

struct PagingResult {
    std::string algorithm_name;
    int num_frames;
    int total_references;
    int page_faults;
    int page_hits;
    double fault_rate_percent;
    std::vector<std::vector<int>> frame_history; // History of frames at each step
    std::vector<bool> was_fault;                 // Fault flag at each step
    std::string log_output;
};

class MemorySimulator {
public:
    MemorySimulator();

    // Run FIFO Page Replacement Algorithm
    static PagingResult simulate_fifo(const std::vector<int>& reference_string, int num_frames);

    // Run LRU (Least Recently Used) Page Replacement Algorithm
    static PagingResult simulate_lru(const std::vector<int>& reference_string, int num_frames);

    // Comparison summary of FIFO vs LRU
    static std::string compare_algorithms(const std::vector<int>& reference_string, int num_frames);

    // Generate realistic page reference string from submission memory profile
    static std::vector<int> generate_submission_references(int submission_size_kb);
};

#endif // MEMORY_SIM_H
