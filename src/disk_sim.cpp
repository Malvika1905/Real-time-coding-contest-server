#include "disk_sim.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

DiskSimulator::DiskSimulator() {}

DiskResult DiskSimulator::run_fcfs(const std::vector<int>& requests, int initial_head) {
    DiskResult res;
    res.algorithm_name = "FCFS (First-Come, First-Served)";
    res.initial_head = initial_head;
    res.total_head_movement = 0;

    int current = initial_head;
    res.seek_sequence.push_back(current);

    std::stringstream ss;
    ss << "=== FCFS DISK SCHEDULING TRACE ===\n";
    ss << "Head starts at cylinder " << initial_head << "\n";

    for (int req : requests) {
        int diff = std::abs(req - current);
        res.total_head_movement += diff;
        ss << "Seek: " << current << " -> " << req << " (dist: " << diff << " cylinders)\n";
        current = req;
        res.seek_sequence.push_back(current);
    }

    res.average_seek_length = requests.empty() ? 0.0 : 
        static_cast<double>(res.total_head_movement) / requests.size();
    ss << "Total Head Movement: " << res.total_head_movement << " cylinders\n";
    res.step_log = ss.str();
    return res;
}

DiskResult DiskSimulator::run_sstf(const std::vector<int>& requests, int initial_head) {
    DiskResult res;
    res.algorithm_name = "SSTF (Shortest Seek Time First)";
    res.initial_head = initial_head;
    res.total_head_movement = 0;

    std::vector<int> pending = requests;
    int current = initial_head;
    res.seek_sequence.push_back(current);

    std::stringstream ss;
    ss << "=== SSTF DISK SCHEDULING TRACE ===\n";
    ss << "Head starts at cylinder " << initial_head << "\n";

    while (!pending.empty()) {
        int best_idx = 0;
        int min_diff = std::abs(pending[0] - current);

        for (size_t i = 1; i < pending.size(); ++i) {
            int diff = std::abs(pending[i] - current);
            if (diff < min_diff) {
                min_diff = diff;
                best_idx = static_cast<int>(i);
            }
        }

        int next_cylinder = pending[best_idx];
        res.total_head_movement += min_diff;
        ss << "Seek: " << current << " -> " << next_cylinder << " (shortest seek: " << min_diff << " cylinders)\n";
        current = next_cylinder;
        res.seek_sequence.push_back(current);
        pending.erase(pending.begin() + best_idx);
    }

    res.average_seek_length = requests.empty() ? 0.0 : 
        static_cast<double>(res.total_head_movement) / requests.size();
    ss << "Total Head Movement: " << res.total_head_movement << " cylinders\n";
    res.step_log = ss.str();
    return res;
}

DiskResult DiskSimulator::run_scan(const std::vector<int>& requests, int initial_head, int disk_size, bool direction_up) {
    DiskResult res;
    res.algorithm_name = "SCAN (Elevator Algorithm)";
    res.initial_head = initial_head;
    res.total_head_movement = 0;

    std::vector<int> left, right;
    for (int r : requests) {
        if (r < initial_head) left.push_back(r);
        else right.push_back(r);
    }

    std::sort(left.begin(), left.end());
    std::sort(right.begin(), right.end());

    std::stringstream ss;
    ss << "=== SCAN DISK SCHEDULING TRACE ===\n";
    ss << "Head starts at cylinder " << initial_head << ", direction: " << (direction_up ? "UP (towards high)" : "DOWN") << "\n";

    int current = initial_head;
    res.seek_sequence.push_back(current);

    if (direction_up) {
        // Move towards higher cylinders
        for (int r : right) {
            res.total_head_movement += std::abs(r - current);
            current = r;
            res.seek_sequence.push_back(current);
        }
        // Sweep to the end of the disk
        if (current != disk_size - 1 && !left.empty()) {
            res.total_head_movement += std::abs((disk_size - 1) - current);
            current = disk_size - 1;
            res.seek_sequence.push_back(current);
        }
        // Reverse direction and service lower cylinders in descending order
        std::reverse(left.begin(), left.end());
        for (int r : left) {
            res.total_head_movement += std::abs(r - current);
            current = r;
            res.seek_sequence.push_back(current);
        }
    } else {
        // Move towards cylinder 0
        std::reverse(left.begin(), left.end());
        for (int r : left) {
            res.total_head_movement += std::abs(r - current);
            current = r;
            res.seek_sequence.push_back(current);
        }
        if (current != 0 && !right.empty()) {
            res.total_head_movement += std::abs(0 - current);
            current = 0;
            res.seek_sequence.push_back(current);
        }
        for (int r : right) {
            res.total_head_movement += std::abs(r - current);
            current = r;
            res.seek_sequence.push_back(current);
        }
    }

    res.average_seek_length = requests.empty() ? 0.0 : 
        static_cast<double>(res.total_head_movement) / requests.size();
    ss << "Total Head Movement: " << res.total_head_movement << " cylinders\n";
    res.step_log = ss.str();
    return res;
}

DiskResult DiskSimulator::run_cscan(const std::vector<int>& requests, int initial_head, int disk_size) {
    DiskResult res;
    res.algorithm_name = "C-SCAN (Circular SCAN)";
    res.initial_head = initial_head;
    res.total_head_movement = 0;

    std::vector<int> left, right;
    for (int r : requests) {
        if (r < initial_head) left.push_back(r);
        else right.push_back(r);
    }

    std::sort(left.begin(), left.end());
    std::sort(right.begin(), right.end());

    std::stringstream ss;
    ss << "=== C-SCAN DISK SCHEDULING TRACE ===\n";
    ss << "Head starts at cylinder " << initial_head << "\n";

    int current = initial_head;
    res.seek_sequence.push_back(current);

    // Service requests to the right (higher cylinders)
    for (int r : right) {
        res.total_head_movement += std::abs(r - current);
        current = r;
        res.seek_sequence.push_back(current);
    }

    // Go to end of disk
    if (current != disk_size - 1) {
        res.total_head_movement += std::abs((disk_size - 1) - current);
        current = disk_size - 1;
        res.seek_sequence.push_back(current);
    }

    // Jump to cylinder 0 (wrap around)
    res.total_head_movement += std::abs(current - 0);
    current = 0;
    res.seek_sequence.push_back(current);

    // Service requests from 0 onwards
    for (int r : left) {
        res.total_head_movement += std::abs(r - current);
        current = r;
        res.seek_sequence.push_back(current);
    }

    res.average_seek_length = requests.empty() ? 0.0 : 
        static_cast<double>(res.total_head_movement) / requests.size();
    ss << "Total Head Movement: " << res.total_head_movement << " cylinders\n";
    res.step_log = ss.str();
    return res;
}

std::string DiskSimulator::compare_all(const std::vector<int>& requests, int initial_head, int disk_size) {
    DiskResult fcfs = run_fcfs(requests, initial_head);
    DiskResult sstf = run_sstf(requests, initial_head);
    DiskResult scan = run_scan(requests, initial_head, disk_size, true);
    DiskResult cscan = run_cscan(requests, initial_head, disk_size);

    std::stringstream ss;
    ss << "========================================================================\n";
    ss << "            DISK SCHEDULING ALGORITHMS COMPARISON REPORT                \n";
    ss << "========================================================================\n";
    ss << "Initial Head Position: " << initial_head << " | Disk Cylinders: 0 to " << disk_size - 1 << "\n";
    ss << "Request Queue: [ ";
    for (int r : requests) ss << r << " ";
    ss << "]\n";
    ss << "------------------------------------------------------------------------\n";
    ss << std::left 
       << std::setw(12) << "Algorithm" 
       << std::setw(24) << "Total Head Movement" 
       << std::setw(18) << "Avg Seek Length" 
       << "\n";
    ss << "------------------------------------------------------------------------\n";
    ss << std::left << std::setw(12) << "FCFS"   << std::setw(24) << (std::to_string(fcfs.total_head_movement) + " cyl")  << std::fixed << std::setprecision(2) << fcfs.average_seek_length << "\n";
    ss << std::left << std::setw(12) << "SSTF"   << std::setw(24) << (std::to_string(sstf.total_head_movement) + " cyl")  << std::fixed << std::setprecision(2) << sstf.average_seek_length << "\n";
    ss << std::left << std::setw(12) << "SCAN"   << std::setw(24) << (std::to_string(scan.total_head_movement) + " cyl")  << std::fixed << std::setprecision(2) << scan.average_seek_length << "\n";
    ss << std::left << std::setw(12) << "C-SCAN" << std::setw(24) << (std::to_string(cscan.total_head_movement) + " cyl") << std::fixed << std::setprecision(2) << cscan.average_seek_length << "\n";
    ss << "------------------------------------------------------------------------\n";
    
    int min_mov = std::min({fcfs.total_head_movement, sstf.total_head_movement, scan.total_head_movement, cscan.total_head_movement});
    std::string optimal = (min_mov == sstf.total_head_movement ? "SSTF" : (min_mov == scan.total_head_movement ? "SCAN" : "C-SCAN"));
    ss << "[Conclusion] " << optimal << " provided minimum head movement (" << min_mov << " cylinders) for this queue.\n";
    ss << "========================================================================\n";
    return ss.str();
}
