#include "contest.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cstdlib>

ContestManager::ContestManager() {
    pthread_mutex_init(&mtx_, nullptr);

    uint64_t now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // 1. Pre-populate Standard Contest Problems
    Problem p1;
    p1.id = 1;
    p1.title = "Two Sum";
    p1.description = "Given two integers A and B, print their sum A + B.";
    p1.difficulty = "Easy";
    p1.input_format = "Two integers A and B separated by space.";
    p1.output_format = "A single integer representing A + B.";
    p1.constraints = "-10^9 <= A, B <= 10^9";
    p1.time_limit_sec = 3.0;
    p1.memory_limit_mb = 64;
    p1.sample_input = "15 27\n";
    p1.sample_output = "42\n";
    p1.hidden_input = "15 27\n";
    p1.hidden_output = "42\n";
    p1.points = 100;
    problems_[p1.id] = p1;

    Problem p2;
    p2.id = 2;
    p2.title = "Palindrome Check";
    p2.description = "Given a string, print 'YES' if palindrome else 'NO'.";
    p2.difficulty = "Easy";
    p2.input_format = "A single string of lowercase English letters.";
    p2.output_format = "Print 'YES' if palindrome, else 'NO'.";
    p2.constraints = "1 <= string length <= 1000";
    p2.time_limit_sec = 3.0;
    p2.memory_limit_mb = 64;
    p2.sample_input = "racecar\n";
    p2.sample_output = "YES\n";
    p2.hidden_input = "racecar\n";
    p2.hidden_output = "YES\n";
    p2.points = 150;
    problems_[p2.id] = p2;

    Problem p3;
    p3.id = 3;
    p3.title = "N-th Fibonacci Number";
    p3.description = "Given N, print the N-th Fibonacci number (0-indexed, F(0)=0, F(1)=1).";
    p3.difficulty = "Medium";
    p3.input_format = "A single integer N.";
    p3.output_format = "A single integer F(N).";
    p3.constraints = "0 <= N <= 30";
    p3.time_limit_sec = 3.0;
    p3.memory_limit_mb = 64;
    p3.sample_input = "10\n";
    p3.sample_output = "55\n";
    p3.hidden_input = "10\n";
    p3.hidden_output = "55\n";
    p3.points = 200;
    problems_[p3.id] = p3;

    Problem p4;
    p4.id = 4;
    p4.title = "Array Min-Max and Sum";
    p4.description = "Given an integer N followed by an array of N integers, find and print the minimum element, maximum element, and total sum separated by single spaces.";
    p4.difficulty = "Easy";
    p4.input_format = "First line: integer N. Second line: N space-separated integers.";
    p4.output_format = "Three space-separated integers: min, max, sum.";
    p4.constraints = "1 <= N <= 10000";
    p4.time_limit_sec = 3.0;
    p4.memory_limit_mb = 64;
    p4.sample_input = "5\n10 4 25 1 8\n";
    p4.sample_output = "1 25 48\n";
    p4.hidden_input = "5\n10 4 25 1 8\n";
    p4.hidden_output = "1 25 48\n";
    p4.points = 150;
    problems_[p4.id] = p4;

    Problem p5;
    p5.id = 5;
    p5.title = "Prime Counter (Sieve)";
    p5.description = "Given an integer N, count how many prime numbers exist less than or equal to N.";
    p5.difficulty = "Medium";
    p5.input_format = "A single integer N.";
    p5.output_format = "Count of prime numbers <= N.";
    p5.constraints = "1 <= N <= 100000";
    p5.time_limit_sec = 3.0;
    p5.memory_limit_mb = 64;
    p5.sample_input = "30\n";
    p5.sample_output = "10\n";
    p5.hidden_input = "30\n";
    p5.hidden_output = "10\n";
    p5.points = 200;
    problems_[p5.id] = p5;

    Problem p6;
    p6.id = 6;
    p6.title = "Balanced Parentheses";
    p6.description = "Given a string of brackets '()', '[]', and '{}', determine if the bracket sequence is valid and balanced. Print 'VALID' if balanced, else 'INVALID'.";
    p6.difficulty = "Medium";
    p6.input_format = "A single line containing bracket string.";
    p6.output_format = "Print 'VALID' or 'INVALID'.";
    p6.constraints = "1 <= length <= 5000";
    p6.time_limit_sec = 3.0;
    p6.memory_limit_mb = 64;
    p6.sample_input = "{[()]}\n";
    p6.sample_output = "VALID\n";
    p6.hidden_input = "{[()]}\n";
    p6.hidden_output = "VALID\n";
    p6.points = 250;
    problems_[p6.id] = p6;

    Problem p7;
    p7.id = 7;
    p7.title = "FCFS CPU Scheduling Simulation";
    p7.description = "An operating system uses First-Come-First-Served (FCFS) CPU scheduling. Given N processes with their burst times in order of arrival (all arriving at time 0), calculate the total turnaround time for all processes (sum of completion times).";
    p7.difficulty = "Hard";
    p7.input_format = "First line: integer N. Second line: N space-separated burst times.";
    p7.output_format = "Single integer: total turnaround time.";
    p7.constraints = "1 <= N <= 1000";
    p7.time_limit_sec = 3.0;
    p7.memory_limit_mb = 64;
    p7.sample_input = "3\n3 5 2\n";
    p7.sample_output = "21\n";
    p7.hidden_input = "3\n3 5 2\n";
    p7.hidden_output = "21\n";
    p7.points = 300;
    problems_[p7.id] = p7;

    // 2. Pre-populate Sample Contest
    Contest c1;
    c1.id = 1;
    c1.title = "Operating Systems Lab Championship";
    c1.description = "Official real-time coding contest demonstrating POSIX process sandboxing, multi-threading, and resource management.";
    c1.status = "ACTIVE";
    c1.duration_minutes = 180;
    c1.start_time = now_sec - 1800;
    c1.end_time = c1.start_time + 180 * 60;
    c1.problem_ids = {1, 2, 3, 4, 5, 6, 7};
    c1.registered_participants = {"alice_coder", "angel", "student_1"};
    contests_[c1.id] = c1;
    next_contest_id_ = 2;

    // 3. Pre-populate Administrator Account
    User admin;
    admin.username = "admin";
    admin.password = "admin123";
    admin.role = "admin";
    admin.is_suspended = false;
    admin.total_score = 0;
    admin.solved_problems_count = 0;
    admin.total_submissions = 0;
    admin.created_at = now_sec;
    users_["admin"] = admin;

    // 4. Pre-populate Initial Sample Participants
    User u1;
    u1.username = "alice_coder";
    u1.password = "pass123";
    u1.role = "participant";
    u1.is_suspended = false;
    u1.total_score = 100;
    u1.solved_problems_count = 1;
    u1.total_submissions = 1;
    u1.created_at = now_sec - 3600;
    users_["alice_coder"] = u1;

    User u2;
    u2.username = "student_1";
    u2.password = "pass123";
    u2.role = "participant";
    u2.is_suspended = false;
    u2.total_score = 0;
    u2.solved_problems_count = 0;
    u2.total_submissions = 0;
    u2.created_at = now_sec - 1200;
    users_["student_1"] = u2;
}

ContestManager::~ContestManager() {
    pthread_mutex_destroy(&mtx_);
}

static std::string read_file_string(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return "";
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

bool ContestManager::load_problems_from_dir(const std::string& base_dir) {
    std::vector<std::pair<int, std::string>> dirs = {
        {1, "p1_two_sum"},
        {2, "p2_palindrome"},
        {3, "p3_fibonacci"},
        {4, "p4_array_minmax"},
        {5, "p5_prime_counter"},
        {6, "p6_balanced_parentheses"},
        {7, "p7_fcfs_scheduling"}
    };

    pthread_mutex_lock(&mtx_);
    for (const auto& item : dirs) {
        int pid = item.first;
        const std::string& folder = item.second;
        std::string p_prob = read_file_string(base_dir + "/" + folder + "/problem.txt");
        std::string p_in   = read_file_string(base_dir + "/" + folder + "/input.txt");
        std::string p_exp  = read_file_string(base_dir + "/" + folder + "/expected.txt");

        if (!p_prob.empty()) {
            problems_[pid].description = p_prob;
        }
        if (!p_in.empty()) {
            problems_[pid].sample_input = p_in;
            problems_[pid].hidden_input = p_in;
        }
        if (!p_exp.empty()) {
            problems_[pid].sample_output = p_exp;
            problems_[pid].hidden_output = p_exp;
        }
        problems_[pid].time_limit_sec = 3.0;
    }
    pthread_mutex_unlock(&mtx_);
    return true;
}

bool ContestManager::authenticate_user(const std::string& username, const std::string& password) {
    pthread_mutex_lock(&mtx_);
    auto it = users_.find(username);
    if (it == users_.end() || it->second.is_suspended) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    bool ok = (it->second.password == password);
    pthread_mutex_unlock(&mtx_);
    return ok;
}

bool ContestManager::register_or_login(const std::string& username, const std::string& password, std::string& out_role) {
    if (username.empty() || password.empty()) return false;

    pthread_mutex_lock(&mtx_);
    auto it = users_.find(username);
    if (it != users_.end()) {
        if (it->second.is_suspended) {
            pthread_mutex_unlock(&mtx_);
            return false;
        }
        if (it->second.password == password) {
            out_role = it->second.role;
            pthread_mutex_unlock(&mtx_);
            return true;
        }
        pthread_mutex_unlock(&mtx_);
        return false;
    }

    // Register new participant
    User new_user;
    new_user.username = username;
    new_user.password = password;
    new_user.role = "participant";
    new_user.is_suspended = false;
    new_user.total_score = 0;
    new_user.solved_problems_count = 0;
    new_user.total_submissions = 0;
    new_user.created_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    users_[username] = new_user;
    out_role = new_user.role;
    pthread_mutex_unlock(&mtx_);
    return true;
}

bool ContestManager::register_or_login(const std::string& username, const std::string& password) {
    std::string role;
    return register_or_login(username, password, role);
}

const Problem* ContestManager::get_problem(int id) const {
    pthread_mutex_lock(&mtx_);
    auto it = problems_.find(id);
    const Problem* res = (it != problems_.end()) ? &(it->second) : nullptr;
    pthread_mutex_unlock(&mtx_);
    return res;
}

std::vector<Problem> ContestManager::get_all_problems() const {
    pthread_mutex_lock(&mtx_);
    std::vector<Problem> list;
    for (const auto& pair : problems_) {
        list.push_back(pair.second);
    }
    pthread_mutex_unlock(&mtx_);
    return list;
}

bool ContestManager::add_or_update_problem(const Problem& prob) {
    Problem p = prob;
    pthread_mutex_lock(&mtx_);
    if (p.id <= 0) {
        int max_id = 0;
        for (const auto& pair : problems_) {
            if (pair.first > max_id) max_id = pair.first;
        }
        p.id = max_id + 1;
    }
    if (p.time_limit_sec <= 0.0) p.time_limit_sec = 3.0;
    if (p.memory_limit_mb <= 0) p.memory_limit_mb = 64;
    if (p.difficulty.empty()) p.difficulty = "Medium";
    if (p.points <= 0) p.points = 100;

    problems_[p.id] = p;
    pthread_mutex_unlock(&mtx_);
    return true;
}

bool ContestManager::delete_problem(int id) {
    pthread_mutex_lock(&mtx_);
    auto it = problems_.find(id);
    if (it == problems_.end()) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    problems_.erase(it);

    // Remove from contests
    for (auto& pair : contests_) {
        auto& pids = pair.second.problem_ids;
        pids.erase(std::remove(pids.begin(), pids.end(), id), pids.end());
    }
    pthread_mutex_unlock(&mtx_);
    return true;
}

int ContestManager::create_contest(const Contest& contest) {
    Contest c = contest;
    pthread_mutex_lock(&mtx_);
    c.id = next_contest_id_++;
    if (c.status.empty()) c.status = "DRAFT";
    if (c.duration_minutes <= 0) c.duration_minutes = 120;
    contests_[c.id] = c;
    int id = c.id;
    pthread_mutex_unlock(&mtx_);
    return id;
}

bool ContestManager::update_contest(const Contest& contest) {
    pthread_mutex_lock(&mtx_);
    auto it = contests_.find(contest.id);
    if (it == contests_.end()) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    it->second = contest;
    pthread_mutex_unlock(&mtx_);
    return true;
}

bool ContestManager::delete_contest(int id) {
    pthread_mutex_lock(&mtx_);
    auto it = contests_.find(id);
    if (it == contests_.end()) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    contests_.erase(it);
    pthread_mutex_unlock(&mtx_);
    return true;
}

const Contest* ContestManager::get_contest(int id) const {
    pthread_mutex_lock(&mtx_);
    auto it = contests_.find(id);
    const Contest* res = (it != contests_.end()) ? &(it->second) : nullptr;
    pthread_mutex_unlock(&mtx_);
    return res;
}

std::vector<Contest> ContestManager::get_all_contests() const {
    pthread_mutex_lock(&mtx_);
    std::vector<Contest> list;
    for (const auto& pair : contests_) {
        list.push_back(pair.second);
    }
    pthread_mutex_unlock(&mtx_);
    return list;
}

bool ContestManager::set_contest_status(int id, const std::string& status) {
    pthread_mutex_lock(&mtx_);
    auto it = contests_.find(id);
    if (it == contests_.end()) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    it->second.status = status;
    pthread_mutex_unlock(&mtx_);
    return true;
}

User ContestManager::get_user_copy(const std::string& username) {
    pthread_mutex_lock(&mtx_);
    auto it = users_.find(username);
    User res = (it != users_.end()) ? it->second : User{username, "", "participant", false, 0, 0, {}, 0, 0};
    pthread_mutex_unlock(&mtx_);
    return res;
}

std::vector<User> ContestManager::get_all_users() const {
    pthread_mutex_lock(&mtx_);
    std::vector<User> list;
    for (const auto& pair : users_) {
        list.push_back(pair.second);
    }
    pthread_mutex_unlock(&mtx_);
    return list;
}

bool ContestManager::set_user_suspended(const std::string& username, bool suspended) {
    pthread_mutex_lock(&mtx_);
    auto it = users_.find(username);
    if (it == users_.end()) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    // Disallow suspending master admin
    if (it->second.role == "admin" && suspended) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    it->second.is_suspended = suspended;
    pthread_mutex_unlock(&mtx_);
    return true;
}

bool ContestManager::is_admin_user(const std::string& username) const {
    pthread_mutex_lock(&mtx_);
    auto it = users_.find(username);
    bool is_adm = (it != users_.end()) && (it->second.role == "admin") && (!it->second.is_suspended);
    pthread_mutex_unlock(&mtx_);
    return is_adm;
}

std::string ContestManager::create_user_session(const std::string& username) {
    pthread_mutex_lock(&mtx_);
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::string token = "sess_" + username + "_" + std::to_string(now) + "_" + std::to_string(rand() % 1000000);
    user_sessions_[token] = username;
    pthread_mutex_unlock(&mtx_);
    return token;
}

bool ContestManager::validate_user_session(const std::string& username, const std::string& token) {
    if (token.empty() || username.empty()) return false;
    pthread_mutex_lock(&mtx_);
    auto it = user_sessions_.find(token);
    if (it == user_sessions_.end()) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    bool match = (it->second == username);
    if (match) {
        auto u_it = users_.find(username);
        if (u_it != users_.end() && u_it->second.is_suspended) {
            match = false;
        }
    }
    pthread_mutex_unlock(&mtx_);
    return match;
}

bool ContestManager::get_username_from_session(const std::string& token, std::string& out_username) {
    if (token.empty()) return false;
    pthread_mutex_lock(&mtx_);
    auto it = user_sessions_.find(token);
    if (it == user_sessions_.end()) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    std::string uname = it->second;
    auto u_it = users_.find(uname);
    if (u_it != users_.end() && u_it->second.is_suspended) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    out_username = uname;
    pthread_mutex_unlock(&mtx_);
    return true;
}

void ContestManager::revoke_user_session(const std::string& token) {
    pthread_mutex_lock(&mtx_);
    user_sessions_.erase(token);
    pthread_mutex_unlock(&mtx_);
}

bool ContestManager::record_problem_solve_if_new(const std::string& username, int problem_id, int points) {
    pthread_mutex_lock(&mtx_);
    auto it = users_.find(username);
    if (it == users_.end()) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    it->second.total_submissions++;
    if (it->second.solved_problems[problem_id]) {
        // Already solved: do not duplicate points
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    // First time solving!
    it->second.solved_problems[problem_id] = true;
    it->second.solved_problems_count++;
    it->second.total_score += points;
    pthread_mutex_unlock(&mtx_);
    return true;
}

std::string ContestManager::generate_admin_token(const std::string& username) {
    pthread_mutex_lock(&mtx_);
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::string token = "adm_token_" + username + "_" + std::to_string(now) + "_" + std::to_string(rand() % 100000);
    admin_tokens_[token] = username;
    pthread_mutex_unlock(&mtx_);
    return token;
}

bool ContestManager::validate_admin_token(const std::string& token, std::string& out_username) const {
    if (token.empty()) return false;
    pthread_mutex_lock(&mtx_);
    auto it = admin_tokens_.find(token);
    if (it == admin_tokens_.end()) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }

    out_username = it->second;
    auto user_it = users_.find(out_username);
    if (user_it == users_.end()) {
        pthread_mutex_unlock(&mtx_);
        return false;
    }
    bool ok = (user_it->second.role == "admin" && !user_it->second.is_suspended);
    pthread_mutex_unlock(&mtx_);
    return ok;
}

void ContestManager::revoke_admin_token(const std::string& token) {
    pthread_mutex_lock(&mtx_);
    admin_tokens_.erase(token);
    pthread_mutex_unlock(&mtx_);
}

const char* verdict_to_string(SubmissionVerdict v) {
    switch (v) {
        case VERDICT_ACCEPTED:               return "ACCEPTED (AC)";
        case VERDICT_WRONG_ANSWER:           return "WRONG ANSWER (WA)";
        case VERDICT_TIME_LIMIT_EXCEEDED:    return "TIME LIMIT EXCEEDED (TLE)";
        case VERDICT_MEMORY_LIMIT_EXCEEDED:  return "MEMORY LIMIT EXCEEDED (MLE)";
        case VERDICT_COMPILATION_ERROR:      return "COMPILATION ERROR (CE)";
        case VERDICT_RUNTIME_ERROR:          return "RUNTIME ERROR (RE/SIGSEGV)";
        case VERDICT_PENDING:                return "PENDING";
        case VERDICT_INTERNAL_ERROR:         return "INTERNAL JUDGE ERROR";
        default:                             return "UNKNOWN";
    }
}
