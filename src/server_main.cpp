#include "protocol.h"
#include "contest.h"
#include "sync_manager.h"
#include "leaderboard.h"
#include "thread_pool.h"
#include "judge_sandbox.h"
#include "ipc_shared_memory.h"
#include "bankers_algo.h"
#include "memory_sim.h"
#include "disk_sim.h"
#include "submission_manager.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

constexpr int SERVER_PORT = 8080;
constexpr int THREAD_POOL_SIZE = 16;

static std::atomic<bool> g_server_running{true};
std::atomic<uint32_t> g_connected_clients{0};
std::atomic<uint32_t> g_total_submissions{0};
std::atomic<uint32_t> g_total_page_faults{0};
std::atomic<uint32_t> g_total_disk_movement{0};

ContestManager g_contest;
Leaderboard    g_leaderboard;
BankersManager g_bankers;

#include "http_server.h"

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "\n[Server] Caught shutdown signal (" << sig << "). Stopping gracefully...\n";
        g_server_running.store(false);
    }
}

// Client Session Handler executed by worker thread in ThreadPool
void handle_client_connection(int client_fd) {
    g_connected_clients++;

    // Configure safe socket timeouts to prevent hanging threads
#ifdef _WIN32
    DWORD sock_to = 8000; // 8 seconds timeout
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&sock_to, sizeof(sock_to));
    setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sock_to, sizeof(sock_to));
#else
    struct timeval sock_tv;
    sock_tv.tv_sec = 8;
    sock_tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&sock_tv, sizeof(sock_tv));
    setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sock_tv, sizeof(sock_tv));
#endif

    std::string current_user = "";
    bool authenticated = false;
    PacketHeader header;
    std::string payload;

    // Check if incoming connection is an HTTP request from the React frontend
    char peek_buf[8];
    int bytes_peeked = recv(client_fd, peek_buf, 4, MSG_PEEK);
    if (bytes_peeked >= 4 && is_http_request(peek_buf, bytes_peeked)) {
        std::string http_req;
        char buf[2048];
        int n;
        while ((n = recv(client_fd, buf, sizeof(buf) - 1, 0)) > 0) {
            buf[n] = '\0';
            http_req += buf;
            if (http_req.find("\r\n\r\n") != std::string::npos) {
                // If it's a POST request, ensure Content-Length bytes are read
                size_t cl_pos = http_req.find("Content-Length:");
                if (cl_pos == std::string::npos) cl_pos = http_req.find("content-length:");
                if (cl_pos != std::string::npos) {
                    size_t line_end = http_req.find("\r\n", cl_pos);
                    int cl = 0;
                    try {
                        cl = std::stoi(http_req.substr(cl_pos + 15, line_end - (cl_pos + 15)));
                    } catch (...) { cl = 0; }
                    size_t body_start = http_req.find("\r\n\r\n") + 4;
                    while (http_req.length() < body_start + cl) {
                        int m = recv(client_fd, buf, sizeof(buf) - 1, 0);
                        if (m <= 0) break;
                        buf[m] = '\0';
                        http_req += buf;
                    }
                }
                break;
            }
        }
        handle_http_connection(client_fd, http_req);
        goto client_teardown;
    }

    SharedMemoryManager::instance().append_log("TCP Client connected (fd=" + std::to_string(client_fd) + ")");

    while (g_server_running.load() && recv_packet(client_fd, header, payload)) {
        switch (header.msg_type) {
            case MSG_LOGIN: {
                // Format: "username:password"
                size_t sep = payload.find(':');
                if (sep == std::string::npos) {
                    send_packet(client_fd, MSG_LOGIN_RESP, STATUS_INVALID_REQUEST, "Invalid format. Expected user:pass");
                    break;
                }
                std::string u = payload.substr(0, sep);
                std::string p = payload.substr(sep + 1);

                SyncManager::instance().lock_users();
                bool ok = g_contest.register_or_login(u, p);
                SyncManager::instance().unlock_users();

                if (ok) {
                    current_user = u;
                    authenticated = true;
                    send_packet(client_fd, MSG_LOGIN_RESP, STATUS_OK, "Login successful. Welcome, " + u + "!");
                    SharedMemoryManager::instance().append_log("User logged in: " + u);
                } else {
                    send_packet(client_fd, MSG_LOGIN_RESP, STATUS_UNAUTHORIZED, "Invalid credentials.");
                }
                break;
            }

            case MSG_LIST_PROBLEMS: {
                std::vector<Problem> probs = g_contest.get_all_problems();
                std::stringstream ss;
                ss << "========================================================================\n";
                ss << "                       AVAILABLE CONTEST PROBLEMS                       \n";
                ss << "========================================================================\n";
                for (const auto& pr : probs) {
                    ss << "[Problem #" << pr.id << "] " << pr.title 
                       << " | Points: " << pr.points 
                       << " | Time Limit: " << pr.time_limit_sec << "s"
                       << " | Mem: " << pr.memory_limit_mb << "MB\n";
                }
                ss << "========================================================================\n";
                send_packet(client_fd, MSG_LIST_PROBLEMS_RESP, STATUS_OK, ss.str());
                break;
            }

            case MSG_GET_PROBLEM: {
                int prob_id = 0;
                try {
                    prob_id = std::stoi(payload);
                } catch (...) {
                    send_packet(client_fd, MSG_GET_PROBLEM_RESP, STATUS_INVALID_REQUEST, "Invalid problem ID");
                    break;
                }

                const Problem* pr = g_contest.get_problem(prob_id);
                if (!pr) {
                    send_packet(client_fd, MSG_GET_PROBLEM_RESP, STATUS_PROBLEM_NOT_FOUND, "Problem not found.");
                } else {
                    std::stringstream ss;
                    ss << "========================================================================\n";
                    ss << "PROBLEM " << pr->id << ": " << pr->title << "\n";
                    ss << "Points: " << pr->points << " | Time Limit: " << pr->time_limit_sec << "s | Memory: " << pr->memory_limit_mb << "MB\n";
                    ss << "------------------------------------------------------------------------\n";
                    ss << pr->description << "\n";
                    ss << "========================================================================\n";
                    send_packet(client_fd, MSG_GET_PROBLEM_RESP, STATUS_OK, ss.str());
                }
                break;
            }

            case MSG_SUBMIT_CODE: {
                if (!authenticated) {
                    send_packet(client_fd, MSG_SUBMIT_RESP, STATUS_UNAUTHORIZED, "Please login first!");
                    break;
                }

                // Format: "<prob_id>#<code>"
                size_t sep = payload.find('#');
                if (sep == std::string::npos) {
                    send_packet(client_fd, MSG_SUBMIT_RESP, STATUS_INVALID_REQUEST, "Malformed submission payload.");
                    break;
                }

                int prob_id = std::stoi(payload.substr(0, sep));
                std::string code = payload.substr(sep + 1);

                const Problem* pr = g_contest.get_problem(prob_id);
                if (!pr) {
                    send_packet(client_fd, MSG_SUBMIT_RESP, STATUS_PROBLEM_NOT_FOUND, "Problem not found.");
                    break;
                }

                uint64_t sub_id = ++g_total_submissions;
                SharedMemoryManager::instance().append_log("Evaluating submission #" + std::to_string(sub_id) + " for " + current_user);

                std::string judge_in = !pr->hidden_input.empty() ? pr->hidden_input : pr->sample_input;
                std::string judge_exp = !pr->hidden_output.empty() ? pr->hidden_output : pr->sample_output;

                // Module 3 & 7: Fork & Exec Sandbox Evaluation
                JudgeResult res = JudgeSandbox::evaluate_submission(
                    sub_id,
                    code,
                    judge_in,
                    judge_exp,
                    pr->time_limit_sec,
                    pr->memory_limit_mb
                );

                auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                if (res.verdict == VERDICT_ACCEPTED) {
                    // Update synchronized leaderboard
                    bool is_new_solve = g_contest.record_problem_solve_if_new(current_user, pr->id, pr->points);
                    g_leaderboard.record_solution(current_user, pr->points, now_sec % 10000, is_new_solve);
                }

                SubmissionRecord record;
                record.id = sub_id;
                record.username = current_user;
                record.problem_id = pr->id;
                record.problem_title = pr->title;
                record.verdict = verdict_to_string(res.verdict);
                record.execution_time_ms = res.execution_time_ms;
                record.memory_used_kb = res.memory_used_kb;
                record.compiler_output = res.compiler_output;
                record.runtime_output = res.runtime_output;
                record.timestamp = now_sec;
                record.code = code;
                SubmissionManager::instance().record_submission(record);

                // Module 5 & 6 Integration: Feed submission memory profile into Paging & Disk schedulers
                std::vector<int> mem_refs = MemorySimulator::generate_submission_references(
                    static_cast<int>(code.size() / 10 + 20));
                PagingResult paging = MemorySimulator::simulate_lru(mem_refs, 4);
                g_total_page_faults += paging.page_faults;

                std::vector<int> disk_reqs = {45, 120, 15, 180, 92};
                DiskResult disk_res = DiskSimulator::run_scan(disk_reqs, 50, 200, true);
                g_total_disk_movement += disk_res.total_head_movement;

                // Format response
                std::stringstream ss;
                ss << "================== SUBMISSION RESULT ==================\n";
                ss << "Submission ID:   #" << sub_id << "\n";
                ss << "User:            " << current_user << "\n";
                ss << "Problem:         #" << pr->id << " (" << pr->title << ")\n";
                ss << "Verdict:         " << verdict_to_string(res.verdict) << "\n";
                ss << "Execution Time:  " << res.execution_time_ms << " ms\n";
                ss << "Memory Used:     " << res.memory_used_kb << " KB\n";
                if (!res.compiler_output.empty()) {
                    ss << "\n[Compiler Messages]:\n" << res.compiler_output << "\n";
                }
                if (!res.runtime_output.empty() && res.verdict != VERDICT_ACCEPTED) {
                    ss << "\n[Program Output]:\n" << res.runtime_output << "\n";
                }
                ss << "=======================================================\n";

                send_packet(client_fd, MSG_SUBMIT_RESP, STATUS_OK, ss.str());
                SharedMemoryManager::instance().append_log("Verdict for #" + std::to_string(sub_id) + ": " + verdict_to_string(res.verdict));
                break;
            }

            case MSG_GET_LEADERBOARD: {
                std::string table = g_leaderboard.get_formatted_table();
                send_packet(client_fd, MSG_GET_LEADERBOARD_RESP, STATUS_OK, table);
                break;
            }

            case MSG_RACE_STRESS_TEST: {
                // Module 3 Demonstration: Race condition test
                // Format: "WITH_LOCKS" or "WITHOUT_LOCKS"
                bool use_locks = (payload != "WITHOUT_LOCKS");
                g_race_condition_mode.store(!use_locks);

                SyncManager::instance().reset_demo_counter();
                const int num_threads = 4;
                const int iters_per_thread = 25000;
                const int expected_total = num_threads * iters_per_thread;

                struct StressThreadArg {
                    bool use_locks;
                    int iters;
                };

                auto stress_worker = [](void* arg) -> void* {
                    auto* sa = static_cast<StressThreadArg*>(arg);
                    if (sa->use_locks) {
                        SyncManager::instance().demo_increment_counter_synchronized(sa->iters);
                    } else {
                        SyncManager::instance().demo_increment_counter_unsynchronized(sa->iters);
                    }
                    delete sa;
                    return nullptr;
                };

                std::vector<pthread_t> stress_threads(num_threads);
                for (int t = 0; t < num_threads; ++t) {
                    auto* sa = new StressThreadArg{use_locks, iters_per_thread};
                    pthread_create(&stress_threads[t], nullptr, stress_worker, sa);
                }

                for (int t = 0; t < num_threads; ++t) {
                    pthread_join(stress_threads[t], nullptr);
                }

                int64_t final_val = SyncManager::instance().get_demo_counter();
                int64_t lost_updates = expected_total - final_val;

                std::stringstream ss;
                ss << "========================================================================\n";
                ss << "       CONCURRENCY EXPERIMENT: RACE CONDITION & MUTEX DEMONSTRATION     \n";
                ss << "========================================================================\n";
                ss << "Mode Configured:         " << (use_locks ? "SYNCHRONIZED (WITH MUTEX)" : "UNSYNCHRONIZED (RACE CONDITION)") << "\n";
                ss << "Concurrent Threads:      " << num_threads << "\n";
                ss << "Iterations Per Thread:   " << iters_per_thread << "\n";
                ss << "Expected Final Count:    " << expected_total << "\n";
                ss << "Actual Final Count:      " << final_val << "\n";
                ss << "Lost Updates / Drift:    " << lost_updates << "\n";
                ss << "------------------------------------------------------------------------\n";
                if (!use_locks) {
                    ss << "[OS ANALYSIS]: Because mutual exclusion locks were bypassed, multiple threads\n"
                       << "interleaved during the non-atomic read-modify-write critical section.\n"
                       << "This produced a classical race condition with " << lost_updates << " lost updates.\n";
                } else {
                    ss << "[OS ANALYSIS]: Mutex locks enforced mutual exclusion on the shared variable.\n"
                       << "All concurrent increments serialized safely with 0 lost updates.\n";
                }
                ss << "========================================================================\n";

                send_packet(client_fd, MSG_RACE_STRESS_RESP, STATUS_OK, ss.str());
                SharedMemoryManager::instance().append_log("Race demo executed: " + std::string(use_locks ? "WITH_LOCKS" : "WITHOUT_LOCKS"));
                break;
            }

            case MSG_DEADLOCK_DEMO: {
                // Module 4: Banker's Algorithm Demonstration
                std::stringstream ss;
                ss << g_bankers.print_matrices() << "\n";

                std::vector<int> safe_seq;
                std::string safety_trace;
                bool is_safe = g_bankers.check_safety(safe_seq, safety_trace);
                ss << safety_trace << "\n";

                // Demonstrate a safe resource request
                std::string req_trace;
                g_bankers.request_resources(1, {1, 0, 2, 0}, req_trace);
                ss << req_trace << "\n";

                send_packet(client_fd, MSG_DEADLOCK_DEMO_RESP, STATUS_OK, ss.str());
                SharedMemoryManager::instance().append_log("Banker's deadlock demo executed.");
                break;
            }

            case MSG_MEMORY_DEMO: {
                // Module 5: Paging simulation (FIFO vs LRU)
                std::vector<int> sample_refs = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
                std::string report = MemorySimulator::compare_algorithms(sample_refs, 3);
                send_packet(client_fd, MSG_MEMORY_DEMO_RESP, STATUS_OK, report);
                SharedMemoryManager::instance().append_log("Memory management (FIFO vs LRU) demo executed.");
                break;
            }

            case MSG_DISK_DEMO: {
                // Module 6: Disk scheduling simulation
                std::vector<int> sample_reqs = {98, 183, 37, 122, 14, 124, 65, 67};
                std::string report = DiskSimulator::compare_all(sample_reqs, 53, 200);
                send_packet(client_fd, MSG_DISK_DEMO_RESP, STATUS_OK, report);
                SharedMemoryManager::instance().append_log("Disk scheduling (FCFS/SSTF/SCAN/C-SCAN) demo executed.");
                break;
            }

            case MSG_DISCONNECT: {
                goto client_teardown;
            }

            default:
                send_packet(client_fd, MSG_ERROR, STATUS_INVALID_REQUEST, "Unknown message type");
                break;
        }
    }

client_teardown:
#ifdef _WIN32
    closesocket(client_fd);
#else
    close(client_fd);
#endif
    g_connected_clients--;
    SharedMemoryManager::instance().append_log("Client disconnected (user=" + (current_user.empty() ? "anonymous" : current_user) + ")");
}

int main(int argc, char* argv[]) {
    std::cout << "========================================================================\n";
    std::cout << "      REAL-TIME CODING CONTEST SERVER - OPERATING SYSTEMS PROJECT       \n";
    std::cout << "========================================================================\n";

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[Winsock Error] Failed to initialize Winsock.\n";
        return 1;
    }
#else
    // Ignore SIGPIPE so write to severed socket doesn't terminate the process
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#endif

    // Initialize Shared Memory Telemetry
    if (!SharedMemoryManager::instance().init_as_server()) {
        std::cerr << "[Warning] Running with in-memory telemetry fallback.\n";
    }

    // Load Contest Problems
    g_contest.load_problems_from_dir("problems");
    std::cout << "[Init] Loaded contest problems successfully.\n";

    // Initialize Worker Thread Pool (Module 1 & 3)
    ThreadPool pool(THREAD_POOL_SIZE);
    std::cout << "[Init] Initialized POSIX Pthread worker pool (" << THREAD_POOL_SIZE << " threads).\n";

    // Start Telemetry Sync Background Thread (Module 1 & 3: POSIX Thread)
    struct TelemetryArgs { ThreadPool* pool; };
    auto* targs = new TelemetryArgs{&pool};
    pthread_t telemetry_th;
    pthread_create(&telemetry_th, nullptr, [](void* arg) -> void* {
        auto* ta = static_cast<TelemetryArgs*>(arg);
        while (g_server_running.load()) {
            SyncManager& sm = SyncManager::instance();
            SharedMemoryManager::instance().update_telemetry(
                g_connected_clients.load(),
                static_cast<uint32_t>(ta->pool->active_workers()),
                g_total_submissions.load(),
                sm.get_available_judge_slots(),
                g_race_condition_mode.load(),
                sm.stats().mutex_acquisitions.load(),
                sm.get_demo_counter(),
                g_total_page_faults.load(),
                g_total_disk_movement.load()
            );
#ifdef _WIN32
            posix_sleep_ms(500);
#else
            usleep(500000);
#endif
        }
        delete ta;
        return nullptr;
    }, targs);

    // Create Master TCP Listening Socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[Socket Error] Failed to create socket.\n";
        return 1;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[Socket Error] Bind failed on port " << SERVER_PORT << ".\n";
        return 1;
    }

    if (listen(server_fd, 64) < 0) {
        std::cerr << "[Socket Error] Listen failed.\n";
        return 1;
    }

    std::cout << "[Server] TCP Server actively listening on port " << SERVER_PORT << "...\n";
    std::cout << "[Server] Ready for client connections and OS concept demonstrations.\n";
    std::cout << "------------------------------------------------------------------------\n";

    while (g_server_running.load()) {
        sockaddr_in client_addr;
#ifdef _WIN32
        int addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
#else
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
#endif
        if (client_fd < 0) {
            if (!g_server_running.load()) break;
            continue;
        }

        // Dispatch client handling task to Pthread Worker Pool (Producer-Consumer)
        pool.enqueue([client_fd]() {
            handle_client_connection(client_fd);
        });
    }

    std::cout << "[Server] Shutting down...\n";
#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif

    pthread_join(telemetry_th, nullptr);
    pool.shutdown();
    SharedMemoryManager::instance().cleanup();

    std::cout << "[Server] Stopped successfully.\n";
    return 0;
}
