#ifndef DISK_SIM_H
#define DISK_SIM_H

#include <vector>
#include <string>

struct DiskResult {
    std::string algorithm_name;
    int initial_head;
    std::vector<int> seek_sequence;
    int total_head_movement;
    double average_seek_length;
    std::string step_log;
};

class DiskSimulator {
public:
    DiskSimulator();

    // Secondary Storage Disk Scheduling Algorithms
    static DiskResult run_fcfs(const std::vector<int>& requests, int initial_head);
    static DiskResult run_sstf(const std::vector<int>& requests, int initial_head);
    static DiskResult run_scan(const std::vector<int>& requests, int initial_head, int disk_size, bool direction_up);
    static DiskResult run_cscan(const std::vector<int>& requests, int initial_head, int disk_size);

    // Comprehensive comparison across all 4 algorithms
    static std::string compare_all(const std::vector<int>& requests, int initial_head, int disk_size = 200);
};

#endif // DISK_SIM_H
