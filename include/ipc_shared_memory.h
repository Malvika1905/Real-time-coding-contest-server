#ifndef IPC_SHARED_MEMORY_H
#define IPC_SHARED_MEMORY_H

#include "posix_compat.h"
#include <cstdint>
#include <string>

constexpr const char* SHM_SERVER_NAME = "/os_contest_telemetry";
constexpr size_t MAX_LOG_ENTRIES = 32;
constexpr size_t MAX_LOG_MSG_LEN = 128;

struct ShmLogEntry {
    uint64_t timestamp;
    char message[MAX_LOG_MSG_LEN];
};

// POSIX Shared Memory Block
struct ServerTelemetry {
    // Process and Thread Counts
    uint32_t server_pid;
    uint32_t active_clients;
    uint32_t active_worker_threads;
    uint32_t total_submissions;
    
    // Concurrency & Synchronization
    uint32_t judge_semaphore_val;
    uint32_t race_condition_mode;
    uint64_t mutex_acquisitions;
    int64_t  shared_counter_val;

    // Simulation metrics
    uint32_t page_faults;
    uint32_t total_disk_head_movement;

    // Ring buffer of server activity logs
    uint32_t log_head;
    uint32_t log_count;
    ShmLogEntry logs[MAX_LOG_ENTRIES];

    // Shared Mutex for coordinating multiprocess SHM read/writes
    pthread_mutex_t shm_mutex;
};

class SharedMemoryManager {
public:
    static SharedMemoryManager& instance();

    bool init_as_server();
    bool init_as_client();
    void cleanup();

    void update_telemetry(uint32_t clients, uint32_t workers, uint32_t submissions,
                          uint32_t sem_val, bool race_mode, uint64_t mtx_count,
                          int64_t counter_val, uint32_t faults, uint32_t disk_mov);
    void append_log(const std::string& msg);

    ServerTelemetry* get_telemetry();

private:
    SharedMemoryManager();
    ~SharedMemoryManager();

    int shm_fd_;
    ServerTelemetry* telemetry_ptr_;
    bool is_server_;
};

#endif // IPC_SHARED_MEMORY_H
