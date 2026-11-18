#include "bankers_algo.h"
#include <sstream>
#include <iomanip>
#include <iostream>

BankersManager::BankersManager() {
    initialize_demo_state();
}

void BankersManager::initialize_demo_state() {
    state_.num_processes = 5; // 5 Contest Evaluation Tasks (P0 to P4)
    state_.resource_names = {
        "Compiler_Slot",    // R0
        "Sandbox_RAM_Block",// R1
        "Pipe_Descriptor",  // R2
        "CPU_Core"          // R3
    };

    // Available Vector: Total free units of R0, R1, R2, R3
    state_.available = {3, 3, 2, 2};

    // Maximum Demand Matrix (N x M)
    state_.max_claim = {
        {7, 5, 3, 2}, // P0 (Heavy C++ Submission)
        {3, 2, 2, 2}, // P1 (Light Math Submission)
        {9, 0, 2, 2}, // P2 (Memory-heavy Submission)
        {2, 2, 2, 1}, // P3 (Quick I/O Submission)
        {4, 3, 3, 2}  // P4 (Standard Submission)
    };

    // Current Allocation Matrix (N x M)
    state_.allocation = {
        {0, 1, 0, 0}, // P0
        {2, 0, 0, 1}, // P1
        {3, 0, 2, 1}, // P2
        {2, 1, 1, 0}, // P3
        {0, 0, 2, 0}  // P4
    };

    calculate_need();
}

void BankersManager::calculate_need() {
    state_.need.resize(state_.num_processes, std::vector<int>(NUM_RESOURCES, 0));
    for (int i = 0; i < state_.num_processes; ++i) {
        for (int j = 0; j < NUM_RESOURCES; ++j) {
            state_.need[i][j] = state_.max_claim[i][j] - state_.allocation[i][j];
            if (state_.need[i][j] < 0) state_.need[i][j] = 0;
        }
    }
}

bool BankersManager::check_safety(std::vector<int>& safe_sequence, std::string& log) {
    calculate_need();

    std::vector<int> work = state_.available;
    std::vector<bool> finish(state_.num_processes, false);
    safe_sequence.clear();

    std::stringstream ss;
    ss << "=== BANKER'S SAFETY ALGORITHM EXECUTION TRACE ===\n";
    ss << "Initial Work Vector: [ ";
    for (int v : work) ss << v << " ";
    ss << "]\n";

    int count = 0;
    while (count < state_.num_processes) {
        bool found = false;
        for (int p = 0; p < state_.num_processes; ++p) {
            if (!finish[p]) {
                // Check if Need[p] <= Work
                bool can_allocate = true;
                for (int j = 0; j < NUM_RESOURCES; ++j) {
                    if (state_.need[p][j] > work[j]) {
                        can_allocate = false;
                        break;
                    }
                }

                if (can_allocate) {
                    ss << "-> Process P" << p << " can finish. Reclaiming allocated resources: [ ";
                    for (int j = 0; j < NUM_RESOURCES; ++j) {
                        work[j] += state_.allocation[p][j];
                        ss << state_.allocation[p][j] << " ";
                    }
                    ss << "] | New Work: [ ";
                    for (int j = 0; j < NUM_RESOURCES; ++j) ss << work[j] << " ";
                    ss << "]\n";

                    safe_sequence.push_back(p);
                    finish[p] = true;
                    found = true;
                    count++;
                }
            }
        }

        if (!found) {
            // Cannot find any process that can finish: State is UNSAFE (Deadlock danger)
            ss << "\n[RESULT] SYSTEM IS IN AN UNSAFE STATE! No safe sequence exists.\n";
            ss << "Potential deadlock detected among unfinishable processes.\n";
            log = ss.str();
            return false;
        }
    }

    ss << "\n[RESULT] SYSTEM IS IN A SAFE STATE.\n";
    ss << "Safe Execution Sequence: < ";
    for (size_t i = 0; i < safe_sequence.size(); ++i) {
        ss << "P" << safe_sequence[i] << (i + 1 < safe_sequence.size() ? " -> " : " ");
    }
    ss << ">\n";
    log = ss.str();
    return true;
}

bool BankersManager::request_resources(int process_id, const std::vector<int>& request, std::string& result_log) {
    std::stringstream ss;
    ss << "=== BANKER'S RESOURCE REQUEST ALGORITHM ===\n";
    ss << "Process P" << process_id << " requests: [ ";
    for (int r : request) ss << r << " ";
    ss << "]\n";

    // Step 1: Check if Request <= Need
    for (int j = 0; j < NUM_RESOURCES; ++j) {
        if (request[j] > state_.need[process_id][j]) {
            ss << "ERROR: Process P" << process_id << " exceeded its maximum declared claim!\n";
            result_log = ss.str();
            return false;
        }
    }

    // Step 2: Check if Request <= Available
    for (int j = 0; j < NUM_RESOURCES; ++j) {
        if (request[j] > state_.available[j]) {
            ss << "BLOCKED: Resources unavailable. Process P" << process_id << " must wait.\n";
            result_log = ss.str();
            return false;
        }
    }

    // Step 3: Pretend to allocate
    for (int j = 0; j < NUM_RESOURCES; ++j) {
        state_.available[j] -= request[j];
        state_.allocation[process_id][j] += request[j];
        state_.need[process_id][j] -= request[j];
    }

    // Step 4: Run safety test
    std::vector<int> safe_seq;
    std::string safety_trace;
    bool is_safe = check_safety(safe_seq, safety_trace);

    if (is_safe) {
        ss << "REQUEST GRANTED: State remains SAFE.\n";
        ss << safety_trace;
        result_log = ss.str();
        return true;
    } else {
        // Rollback allocation
        for (int j = 0; j < NUM_RESOURCES; ++j) {
            state_.available[j] += request[j];
            state_.allocation[process_id][j] -= request[j];
            state_.need[process_id][j] += request[j];
        }
        ss << "REQUEST DENIED: Granting request leads to an UNSAFE state.\n";
        ss << "Rollback executed. Process P" << process_id << " must wait to avoid deadlock.\n";
        result_log = ss.str();
        return false;
    }
}

bool BankersManager::detect_deadlock(std::vector<int>& deadlocked_processes, std::string& report) {
    std::stringstream ss;
    ss << "=== DEADLOCK DETECTION REPORT ===\n";

    std::vector<int> safe_seq;
    std::string log;
    bool safe = check_safety(safe_seq, log);

    deadlocked_processes.clear();
    if (!safe) {
        for (int i = 0; i < state_.num_processes; ++i) {
            bool in_seq = false;
            for (int p : safe_seq) {
                if (p == i) { in_seq = true; break; }
            }
            if (!in_seq) {
                deadlocked_processes.push_back(i);
            }
        }
        ss << "Deadlocked / Blocked processes: ";
        for (int p : deadlocked_processes) ss << "P" << p << " ";
        ss << "\n";
    } else {
        ss << "No deadlock detected. All processes can complete.\n";
    }

    report = ss.str();
    return !deadlocked_processes.empty();
}

bool BankersManager::recover_by_preemption(int process_id, std::string& recovery_log) {
    std::stringstream ss;
    ss << "=== DEADLOCK RECOVERY VIA RESOURCE PREEMPTION ===\n";
    ss << "Preempting resources from victim process P" << process_id << "...\n";

    for (int j = 0; j < NUM_RESOURCES; ++j) {
        state_.available[j] += state_.allocation[process_id][j];
        state_.allocation[process_id][j] = 0;
    }
    calculate_need();

    ss << "Resources reclaimed. Re-running safety algorithm...\n";
    std::vector<int> seq;
    std::string log;
    bool now_safe = check_safety(seq, log);
    ss << log;
    recovery_log = ss.str();
    return now_safe;
}

std::string BankersManager::print_matrices() {
    std::stringstream ss;
    ss << "------------------------------------------------------------------------\n";
    ss << "                  BANKER'S RESOURCE ALLOCATION TABLE                    \n";
    ss << "------------------------------------------------------------------------\n";
    ss << "Process |    Allocation    |       Max        |       Need       \n";
    ss << "        | R0  R1  R2  R3   |  R0  R1  R2  R3  |  R0  R1  R2  R3  \n";
    ss << "--------+------------------+------------------+------------------\n";

    for (int i = 0; i < state_.num_processes; ++i) {
        ss << "  P" << i << "   | ";
        for (int j = 0; j < NUM_RESOURCES; ++j) ss << std::setw(3) << state_.allocation[i][j] << " ";
        ss << "|  ";
        for (int j = 0; j < NUM_RESOURCES; ++j) ss << std::setw(3) << state_.max_claim[i][j] << " ";
        ss << "|  ";
        for (int j = 0; j < NUM_RESOURCES; ++j) ss << std::setw(3) << state_.need[i][j] << " ";
        ss << "\n";
    }
    ss << "------------------------------------------------------------------------\n";
    ss << "Available Vector: [ ";
    for (int j = 0; j < NUM_RESOURCES; ++j) {
        ss << state_.resource_names[j] << "=" << state_.available[j] << " ";
    }
    ss << "]\n";
    ss << "------------------------------------------------------------------------\n";
    return ss.str();
}
