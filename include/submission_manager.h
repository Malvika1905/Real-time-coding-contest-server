#ifndef SUBMISSION_MANAGER_H
#define SUBMISSION_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "posix_compat.h"

struct SubmissionRecord {
    uint64_t id{0};
    std::string username;
    int problem_id{0};
    std::string problem_title;
    std::string verdict;
    double execution_time_ms{0.0};
    long memory_used_kb{0};
    std::string compiler_output;
    std::string runtime_output;
    uint64_t timestamp{0};
    std::string code;
};

class SubmissionManager {
public:
    static SubmissionManager& instance();

    void record_submission(const SubmissionRecord& record);
    std::vector<SubmissionRecord> get_all_submissions() const;
    std::vector<SubmissionRecord> get_user_submissions(const std::string& username) const;
    bool get_submission(uint64_t id, SubmissionRecord& out_record) const;
    size_t total_submissions() const;
    void get_verdict_stats(int& accepted, int& rejected) const;
    void clear();

private:
    SubmissionManager();
    ~SubmissionManager();

    SubmissionManager(const SubmissionManager&) = delete;
    SubmissionManager& operator=(const SubmissionManager&) = delete;

    mutable pthread_mutex_t mtx_;
    std::vector<SubmissionRecord> submissions_;
};

#endif // SUBMISSION_MANAGER_H
