#include "judge_sandbox.h"
#include "sync_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstring>

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#else
#include <windows.h>
#include <stdio.h>
extern "C" {
    FILE* _popen(const char* command, const char* mode);
    int   _pclose(FILE* stream);
}
#define popen _popen
#define pclose _pclose
#endif

JudgeSandbox::JudgeSandbox() {}
JudgeSandbox::~JudgeSandbox() {}

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool JudgeSandbox::compile_code(
    const std::string& source_file,
    const std::string& binary_file,
    std::string& compiler_output
) {
#ifndef _WIN32
    // Linux Process Isolation for Compilation
    int pipe_err[2];
    if (pipe(pipe_err) < 0) {
        compiler_output = "Internal error: Failed to create compiler pipe.";
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipe_err[0]);
        close(pipe_err[1]);
        compiler_output = "Internal error: Failed to fork compiler process.";
        return false;
    }

    if (pid == 0) {
        // Child Process: redirect stdout/stderr to pipe
        close(pipe_err[0]);
        dup2(pipe_err[1], STDOUT_FILENO);
        dup2(pipe_err[1], STDERR_FILENO);
        close(pipe_err[1]);

        char* const args[] = {
            (char*)"g++",
            (char*)"-O2",
            (char*)"-std=c++14",
            (char*)source_file.c_str(),
            (char*)"-o",
            (char*)binary_file.c_str(),
            nullptr
        };
        execvp("g++", args);
        _exit(127); // If execvp fails
    }

    // Parent Process
    close(pipe_err[1]);
    char buffer[512];
    ssize_t bytes_read;
    compiler_output.clear();
    while ((bytes_read = read(pipe_err[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        compiler_output += buffer;
    }
    close(pipe_err[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0);

#else
    // Windows/MinGW fallback compilation
    std::string cmd = "g++ -O2 -std=c++14 \"" + source_file + "\" -o \"" + binary_file + "\" 2>&1";
    FILE* pipe_fp = popen(cmd.c_str(), "r");
    if (!pipe_fp) {
        compiler_output = "Compilation pipe failed.";
        return false;
    }
    char buffer[256];
    compiler_output.clear();
    while (fgets(buffer, sizeof(buffer), pipe_fp) != nullptr) {
        compiler_output += buffer;
    }
    int rc = pclose(pipe_fp);
    return (rc == 0);
#endif
}

bool JudgeSandbox::execute_sandboxed(
    const std::string& binary_file,
    const std::string& input_data,
    std::string& program_output,
    double time_limit_sec,
    int memory_limit_mb,
    double& elapsed_ms,
    long& max_rss_kb,
    SubmissionVerdict& verdict
) {
    auto start_time = std::chrono::high_resolution_clock::now();

#ifndef _WIN32
    int in_pipe[2];
    int out_pipe[2];

    if (pipe(in_pipe) < 0 || pipe(out_pipe) < 0) {
        verdict = VERDICT_INTERNAL_ERROR;
        return false;
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        verdict = VERDICT_INTERNAL_ERROR;
        return false;
    }

    if (child_pid == 0) {
        // Child Process - Sandboxed Execution
        close(in_pipe[1]);
        close(out_pipe[0]);

        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(out_pipe[1], STDERR_FILENO);

        close(in_pipe[0]);
        close(out_pipe[1]);

        // Module 7 & Module 3: Security & Resource Isolation using setrlimit()
        // CPU Limit: Prevents infinite loops (while(1))
        struct rlimit cpu_lim;
        cpu_lim.rlim_cur = static_cast<rlim_t>(time_limit_sec + 0.99); // Ceiling in seconds
        cpu_lim.rlim_max = cpu_lim.rlim_cur + 1;
        setrlimit(RLIMIT_CPU, &cpu_lim);

        // Memory Limit: Address space restriction
        struct rlimit mem_lim;
        mem_lim.rlim_cur = static_cast<rlim_t>(memory_limit_mb) * 1024 * 1024;
        mem_lim.rlim_max = mem_lim.rlim_cur;
        setrlimit(RLIMIT_AS, &mem_lim);

        char* const args[] = { (char*)binary_file.c_str(), nullptr };
        execvp(binary_file.c_str(), args);
        _exit(127);
    }

    // Parent Process
    close(in_pipe[0]);
    close(out_pipe[1]);

    // Send input to child process via anonymous pipe
    if (!input_data.empty()) {
        write(in_pipe[1], input_data.data(), input_data.size());
    }
    close(in_pipe[1]); // Close to indicate EOF to child's stdin

    // Read child output from pipe
    char buffer[512];
    ssize_t n;
    program_output.clear();
    while ((n = read(out_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        program_output += buffer;
        if (program_output.size() > 512 * 1024) break; // Limit output to 512KB
    }
    close(out_pipe[0]);

    int status = 0;
    struct rusage usage;
    wait4(child_pid, &status, 0, &usage);

    auto end_time = std::chrono::high_resolution_clock::now();
    elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    max_rss_kb = usage.ru_maxrss;

    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        if (sig == SIGXCPU || sig == SIGKILL || sig == SIGALRM) {
            verdict = VERDICT_TIME_LIMIT_EXCEEDED;
        } else if (sig == SIGSEGV) {
            verdict = VERDICT_RUNTIME_ERROR;
        } else {
            verdict = VERDICT_RUNTIME_ERROR;
        }
        return false;
    }

    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) != 0) {
            verdict = VERDICT_RUNTIME_ERROR;
            return false;
        }
    }

    if (elapsed_ms > (time_limit_sec * 1000.0)) {
        verdict = VERDICT_TIME_LIMIT_EXCEEDED;
        return false;
    }

    verdict = VERDICT_ACCEPTED;
    return true;

#else
    // Windows Process Management with Enforced Time Limit via CreateProcess & WaitForSingleObject
    std::string in_file = binary_file + ".in";
    std::string out_file = binary_file + ".out_tmp";
    {
        std::ofstream in_ofs(in_file, std::ios::binary);
        in_ofs << input_data;
    }

    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hIn = CreateFileA(in_file.c_str(), GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hOut = CreateFileA(out_file.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hIn == INVALID_HANDLE_VALUE || hOut == INVALID_HANDLE_VALUE) {
        if (hIn != INVALID_HANDLE_VALUE) CloseHandle(hIn);
        if (hOut != INVALID_HANDLE_VALUE) CloseHandle(hOut);
        remove(in_file.c_str());
        verdict = VERDICT_INTERNAL_ERROR;
        return false;
    }

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = hIn;
    si.hStdOutput = hOut;
    si.hStdError = hOut;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::string cmd = "\"" + binary_file + "\"";
    char cmd_buf[512];
    strncpy(cmd_buf, cmd.c_str(), sizeof(cmd_buf) - 1);
    cmd_buf[sizeof(cmd_buf) - 1] = '\0';

    DWORD timeout_ms = static_cast<DWORD>(time_limit_sec * 1000.0);

    bool created = CreateProcessA(
        NULL,
        cmd_buf,
        NULL,
        NULL,
        TRUE, // inherit handles
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    // Close inherited file handles in parent so child owns them exclusively
    CloseHandle(hIn);
    CloseHandle(hOut);

    if (!created) {
        remove(in_file.c_str());
        remove(out_file.c_str());
        verdict = VERDICT_INTERNAL_ERROR;
        return false;
    }

    DWORD wait_res = WaitForSingleObject(pi.hProcess, timeout_ms);
    auto end_time = std::chrono::high_resolution_clock::now();
    elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    max_rss_kb = 4096;

    if (wait_res == WAIT_TIMEOUT) {
        // Enforce OS process termination on CPU timeout (analogous to Linux SIGXCPU)
        TerminateProcess(pi.hProcess, 9);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        remove(in_file.c_str());
        remove(out_file.c_str());
        verdict = VERDICT_TIME_LIMIT_EXCEEDED;
        return false;
    }

    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    remove(in_file.c_str());

    // Read sandboxed output from file
    {
        std::ifstream out_ifs(out_file, std::ios::binary);
        if (out_ifs.is_open()) {
            std::stringstream ss;
            ss << out_ifs.rdbuf();
            program_output = ss.str();
        }
    }
    remove(out_file.c_str());

    if (exit_code != 0) {
        verdict = VERDICT_RUNTIME_ERROR;
        return false;
    }
    if (elapsed_ms > (time_limit_sec * 1000.0)) {
        verdict = VERDICT_TIME_LIMIT_EXCEEDED;
        return false;
    }
    verdict = VERDICT_ACCEPTED;
    return true;
#endif
}

JudgeResult JudgeSandbox::evaluate_submission(
    uint64_t submission_id,
    const std::string& source_code,
    const std::string& input_data,
    const std::string& expected_output,
    double time_limit_sec,
    int memory_limit_mb
) {
    JudgeResult res;
    res.verdict = VERDICT_PENDING;
    res.execution_time_ms = 0.0;
    res.memory_used_kb = 0;

    // Acquire Counting Semaphore for Judge Slot (Module 3)
    SyncManager::instance().acquire_judge_slot();

    // Ensure build directory exists
#ifdef _WIN32
    CreateDirectoryA("build", NULL);
#else
    mkdir("build", 0777);
#endif

    std::string base_name = "build/sub_" + std::to_string(submission_id);
    std::string src_path = base_name + ".cpp";
    std::string bin_path = base_name +
#ifdef _WIN32
        ".exe";
#else
        ".out";
#endif

    // Write submission source file
    {
        std::ofstream ofs(src_path);
        if (!ofs.is_open()) {
            res.verdict = VERDICT_INTERNAL_ERROR;
            res.compiler_output = "Failed to write temporary source file.";
            SyncManager::instance().release_judge_slot();
            return res;
        }
        ofs << source_code;
    }

    // Compile code
    if (!compile_code(src_path, bin_path, res.compiler_output)) {
        res.verdict = VERDICT_COMPILATION_ERROR;
        remove(src_path.c_str());
        remove(bin_path.c_str());
        SyncManager::instance().release_judge_slot();
        return res;
    }

    // Execute sandboxed code
    SubmissionVerdict run_verdict;
    bool exec_ok = execute_sandboxed(
        bin_path,
        input_data,
        res.runtime_output,
        time_limit_sec,
        memory_limit_mb,
        res.execution_time_ms,
        res.memory_used_kb,
        run_verdict
    );

    if (!exec_ok) {
        res.verdict = run_verdict;
    } else {
        // Compare output against expected output
        if (trim(res.runtime_output) == trim(expected_output)) {
            res.verdict = VERDICT_ACCEPTED;
        } else {
            res.verdict = VERDICT_WRONG_ANSWER;
        }
    }

    // Clean up temporary compilation artifacts to prevent disk / resource leaks
    remove(src_path.c_str());
    remove(bin_path.c_str());

    // Release counting semaphore
    SyncManager::instance().release_judge_slot();

    return res;
}
