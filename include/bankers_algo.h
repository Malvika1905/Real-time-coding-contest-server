#ifndef BANKERS_ALGO_H
#define BANKERS_ALGO_H

#include <vector>
#include <string>

constexpr int NUM_RESOURCES = 4; // R0: Compiler, R1: RAM Slots, R2: Pipes, R3: CPU Cores

struct BankersState {
    int num_processes;
    std::vector<std::string> resource_names;
    std::vector<int> available;
    std::vector<std::vector<int>> max_claim;
    std::vector<std::vector<int>> allocation;
    std::vector<std::vector<int>> need;
};

class BankersManager {
public:
    BankersManager();

    void initialize_demo_state();

    // Banker's Safety Algorithm
    // Returns true if state is safe; fills safe_sequence with the order of processes
    bool check_safety(std::vector<int>& safe_sequence, std::string& step_by_step_log);

    // Banker's Resource Request Algorithm
    bool request_resources(int process_id, const std::vector<int>& request, std::string& result_log);

    // Deadlock Detection Algorithm (for currently held allocations vs requests)
    bool detect_deadlock(std::vector<int>& deadlocked_processes, std::string& report);

    // Deadlock Recovery (preempts resources from process_id to resolve deadlock)
    bool recover_by_preemption(int process_id, std::string& recovery_log);

    // Generate formatted matrices for visualization
    std::string print_matrices();

    const BankersState& get_state() const { return state_; }

private:
    void calculate_need();
    BankersState state_;
};

#endif // BANKERS_ALGO_H
