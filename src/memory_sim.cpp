#include "memory_sim.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <queue>
#include <algorithm>
#include <unordered_map>

MemorySimulator::MemorySimulator() {}

PagingResult MemorySimulator::simulate_fifo(const std::vector<int>& reference_string, int num_frames) {
    PagingResult result;
    result.algorithm_name = "FIFO (First-In First-Out)";
    result.num_frames = num_frames;
    result.total_references = static_cast<int>(reference_string.size());
    result.page_faults = 0;
    result.page_hits = 0;

    std::vector<int> frames(num_frames, -1);
    std::queue<int> fifo_order;

    std::stringstream ss;
    ss << "=== FIFO PAGE REPLACEMENT TRACE ===\n";
    ss << "Frames Allocated: " << num_frames << "\n";
    ss << "Step | Page Ref | Frames State              | Status\n";
    ss << "-----|----------|---------------------------|---------\n";

    for (size_t step = 0; step < reference_string.size(); ++step) {
        int page = reference_string[step];
        bool hit = false;

        for (int f : frames) {
            if (f == page) {
                hit = true;
                break;
            }
        }

        if (hit) {
            result.page_hits++;
            result.was_fault.push_back(false);
        } else {
            result.page_faults++;
            result.was_fault.push_back(true);

            // Check if empty slot available
            auto empty_it = std::find(frames.begin(), frames.end(), -1);
            if (empty_it != frames.end()) {
                *empty_it = page;
                fifo_order.push(page);
            } else {
                // Evict FIFO victim
                int victim = fifo_order.front();
                fifo_order.pop();

                for (int& f : frames) {
                    if (f == victim) {
                        f = page;
                        break;
                    }
                }
                fifo_order.push(page);
            }
        }

        result.frame_history.push_back(frames);

        ss << std::setw(4) << step + 1 << " | "
           << std::setw(8) << page << " | [ ";
        for (int f : frames) {
            if (f == -1) ss << "- ";
            else ss << f << " ";
        }
        ss << "] | " << (hit ? "HIT " : "FAULT (MISS)") << "\n";
    }

    result.fault_rate_percent = (static_cast<double>(result.page_faults) / result.total_references) * 100.0;
    ss << "--------------------------------------------------------\n";
    ss << "Total References: " << result.total_references << "\n";
    ss << "Page Hits:        " << result.page_hits << "\n";
    ss << "Page Faults:      " << result.page_faults << "\n";
    ss << "Fault Rate:       " << std::fixed << std::setprecision(2) << result.fault_rate_percent << "%\n";
    result.log_output = ss.str();

    return result;
}

PagingResult MemorySimulator::simulate_lru(const std::vector<int>& reference_string, int num_frames) {
    PagingResult result;
    result.algorithm_name = "LRU (Least Recently Used)";
    result.num_frames = num_frames;
    result.total_references = static_cast<int>(reference_string.size());
    result.page_faults = 0;
    result.page_hits = 0;

    std::vector<int> frames(num_frames, -1);
    std::unordered_map<int, int> last_used_time; // page -> last step

    std::stringstream ss;
    ss << "=== LRU PAGE REPLACEMENT TRACE ===\n";
    ss << "Frames Allocated: " << num_frames << "\n";
    ss << "Step | Page Ref | Frames State              | Status\n";
    ss << "-----|----------|---------------------------|---------\n";

    for (size_t step = 0; step < reference_string.size(); ++step) {
        int page = reference_string[step];
        bool hit = false;

        for (int f : frames) {
            if (f == page) {
                hit = true;
                break;
            }
        }

        if (hit) {
            result.page_hits++;
            result.was_fault.push_back(false);
            last_used_time[page] = static_cast<int>(step);
        } else {
            result.page_faults++;
            result.was_fault.push_back(true);

            auto empty_it = std::find(frames.begin(), frames.end(), -1);
            if (empty_it != frames.end()) {
                *empty_it = page;
            } else {
                // Find least recently used page
                int lru_page = -1;
                int min_time = 1e9;
                for (int f : frames) {
                    if (last_used_time[f] < min_time) {
                        min_time = last_used_time[f];
                        lru_page = f;
                    }
                }

                for (int& f : frames) {
                    if (f == lru_page) {
                        f = page;
                        break;
                    }
                }
            }
            last_used_time[page] = static_cast<int>(step);
        }

        result.frame_history.push_back(frames);

        ss << std::setw(4) << step + 1 << " | "
           << std::setw(8) << page << " | [ ";
        for (int f : frames) {
            if (f == -1) ss << "- ";
            else ss << f << " ";
        }
        ss << "] | " << (hit ? "HIT " : "FAULT (MISS)") << "\n";
    }

    result.fault_rate_percent = (static_cast<double>(result.page_faults) / result.total_references) * 100.0;
    ss << "--------------------------------------------------------\n";
    ss << "Total References: " << result.total_references << "\n";
    ss << "Page Hits:        " << result.page_hits << "\n";
    ss << "Page Faults:      " << result.page_faults << "\n";
    ss << "Fault Rate:       " << std::fixed << std::setprecision(2) << result.fault_rate_percent << "%\n";
    result.log_output = ss.str();

    return result;
}

std::string MemorySimulator::compare_algorithms(const std::vector<int>& reference_string, int num_frames) {
    PagingResult fifo_res = simulate_fifo(reference_string, num_frames);
    PagingResult lru_res  = simulate_lru(reference_string, num_frames);

    std::stringstream ss;
    ss << "========================================================\n";
    ss << "       PAGE REPLACEMENT ALGORITHM COMPARISON REPORT     \n";
    ss << "========================================================\n";
    ss << "Total Memory Page References: " << reference_string.size() << "\n";
    ss << "Physical Page Frames:         " << num_frames << "\n";
    ss << "--------------------------------------------------------\n";
    ss << std::left 
       << std::setw(15) << "Algorithm" 
       << std::setw(12) << "Page Faults" 
       << std::setw(12) << "Page Hits" 
       << std::setw(15) << "Fault Rate (%)" 
       << "\n";
    ss << "--------------------------------------------------------\n";
    ss << std::left 
       << std::setw(15) << "FIFO" 
       << std::setw(12) << fifo_res.page_faults 
       << std::setw(12) << fifo_res.page_hits 
       << std::setw(15) << std::fixed << std::setprecision(2) << fifo_res.fault_rate_percent 
       << "\n";
    ss << std::left 
       << std::setw(15) << "LRU" 
       << std::setw(12) << lru_res.page_faults 
       << std::setw(12) << lru_res.page_hits 
       << std::setw(15) << std::fixed << std::setprecision(2) << lru_res.fault_rate_percent 
       << "\n";
    ss << "--------------------------------------------------------\n";
    if (lru_res.page_faults < fifo_res.page_faults) {
        ss << "[Analysis] LRU outperformed FIFO by " 
           << (fifo_res.page_faults - lru_res.page_faults) 
           << " fewer page faults due to temporal locality of reference.\n";
    } else if (lru_res.page_faults == fifo_res.page_faults) {
        ss << "[Analysis] Both algorithms exhibited identical fault counts for this reference pattern.\n";
    } else {
        ss << "[Analysis] FIFO had fewer page faults than LRU for this specific reference sequence.\n";
    }
    ss << "========================================================\n";
    return ss.str();
}

std::vector<int> MemorySimulator::generate_submission_references(int submission_size_kb) {
    // Generate page references modeling code execution:
    // Pages 0-2: code/instructions, Page 3-5: stack/local vars, Page 6+: heap/data
    std::vector<int> refs = {1, 2, 3, 4, 2, 1, 5, 6, 2, 1, 2, 3, 7, 6, 3, 2, 1, 2, 3, 6};
    if (submission_size_kb > 100) {
        refs.insert(refs.end(), {7, 8, 9, 2, 3, 1, 5, 8, 9, 2});
    }
    return refs;
}
