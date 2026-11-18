#ifndef JUDGE_SANDBOX_H
#define JUDGE_SANDBOX_H

#include "contest.h"
#include <string>

struct JudgeResult {
    SubmissionVerdict verdict;
    double execution_time_ms;
    long memory_used_kb;
    std::string compiler_output;
    std::string runtime_output;
};

class JudgeSandbox {
public:
    JudgeSandbox();
    ~JudgeSandbox();

    // Sandboxed compilation and execution using Linux fork(), execvp(), pipes, and setrlimit()
    static JudgeResult evaluate_submission(
        uint64_t submission_id,
        const std::string& source_code,
        const std::string& input_data,
        const std::string& expected_output,
        double time_limit_sec,
        int memory_limit_mb
    );

private:
    static bool compile_code(
        const std::string& source_file,
        const std::string& binary_file,
        std::string& compiler_output
    );

    static bool execute_sandboxed(
        const std::string& binary_file,
        const std::string& input_data,
        std::string& program_output,
        double time_limit_sec,
        int memory_limit_mb,
        double& elapsed_ms,
        long& max_rss_kb,
        SubmissionVerdict& verdict
    );
};

#endif // JUDGE_SANDBOX_H
