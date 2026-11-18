#ifndef CONTEST_H
#define CONTEST_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

#include "posix_compat.h"

struct Problem {
    int id;
    std::string title;
    std::string description;
    std::string difficulty;     // "Easy", "Medium", "Hard"
    std::string input_format;
    std::string output_format;
    std::string constraints;
    double time_limit_sec;
    int memory_limit_mb;
    std::string sample_input;
    std::string sample_output;
    std::string hidden_input;   // Sandboxed judge test data (Never exposed to ordinary participants)
    std::string hidden_output;  // Sandboxed judge expected output (Never exposed to ordinary participants)
    int points;
};

struct Contest {
    int id;
    std::string title;
    std::string description;
    std::string status;         // "DRAFT", "ACTIVE", "STOPPED", "PUBLISHED"
    uint64_t start_time;
    uint64_t end_time;
    int duration_minutes;
    std::vector<int> problem_ids;
    std::vector<std::string> registered_participants;
};

enum SubmissionVerdict {
    VERDICT_PENDING,
    VERDICT_ACCEPTED,
    VERDICT_WRONG_ANSWER,
    VERDICT_TIME_LIMIT_EXCEEDED,
    VERDICT_MEMORY_LIMIT_EXCEEDED,
    VERDICT_COMPILATION_ERROR,
    VERDICT_RUNTIME_ERROR,
    VERDICT_INTERNAL_ERROR
};

struct Submission {
    uint64_t id;
    std::string username;
    int problem_id;
    std::string code;
    SubmissionVerdict verdict;
    double execution_time_ms;
    long memory_used_kb;
    std::string compiler_output;
    uint64_t timestamp;
};

struct User {
    std::string username;
    std::string password;
    std::string role;           // "admin" or "participant"
    bool is_suspended;          // If true, user is locked out
    int total_score;
    int solved_problems_count;
    std::map<int, bool> solved_problems; // problem_id -> solved
    int total_submissions;
    uint64_t created_at;
};

class ContestManager {
public:
    ContestManager();
    ~ContestManager();
    bool load_problems_from_dir(const std::string& problems_dir);
    bool authenticate_user(const std::string& username, const std::string& password);
    const Problem* get_problem(int id) const;
    std::vector<Problem> get_all_problems() const;
    
    // Problem Management (Admin)
    bool add_or_update_problem(const Problem& prob);
    bool delete_problem(int id);

    // Contest Management (Admin)
    int create_contest(const Contest& contest);
    bool update_contest(const Contest& contest);
    bool delete_contest(int id);
    const Contest* get_contest(int id) const;
    std::vector<Contest> get_all_contests() const;
    bool set_contest_status(int id, const std::string& status);

    // User State & Participant Management (Admin & Auth)
    bool register_or_login(const std::string& username, const std::string& password, std::string& out_role);
    bool register_or_login(const std::string& username, const std::string& password);
    User get_user_copy(const std::string& username);
    std::vector<User> get_all_users() const;
    bool set_user_suspended(const std::string& username, bool suspended);
    bool is_admin_user(const std::string& username) const;

    // Session Management for Multi-User Concurrency
    std::string create_user_session(const std::string& username);
    bool validate_user_session(const std::string& username, const std::string& token);
    bool get_username_from_session(const std::string& token, std::string& out_username);
    void revoke_user_session(const std::string& token);

    // Solve Deduplication for Clean Leaderboard Scores
    bool record_problem_solve_if_new(const std::string& username, int problem_id, int points);

    // Admin Token RBAC
    std::string generate_admin_token(const std::string& username);
    bool validate_admin_token(const std::string& token, std::string& out_username) const;
    void revoke_admin_token(const std::string& token);

private:
    mutable pthread_mutex_t mtx_;
    std::map<int, Problem> problems_;
    std::map<int, Contest> contests_;
    std::map<std::string, User> users_;
    std::map<std::string, std::string> admin_tokens_; // token -> username
    std::map<std::string, std::string> user_sessions_; // session_token -> username
    int next_contest_id_{1};
};

const char* verdict_to_string(SubmissionVerdict v);

#endif // CONTEST_H
