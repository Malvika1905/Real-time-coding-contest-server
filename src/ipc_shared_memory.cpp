#include "ipc_shared_memory.h"
#include <iostream>
#include <cstring>
#include <chrono>

#ifndef _WIN32
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

SharedMemoryManager& SharedMemoryManager::instance() {
    static SharedMemoryManager mgr;
    return mgr;
}

SharedMemoryManager::SharedMemoryManager() : shm_fd_(-1), telemetry_ptr_(nullptr), is_server_(false) {}

SharedMemoryManager::~SharedMemoryManager() {
    cleanup();
}

bool SharedMemoryManager::init_as_server() {
    is_server_ = true;

#ifndef _WIN32
    // Unlink any preexisting segment
    shm_unlink(SHM_SERVER_NAME);

    shm_fd_ = shm_open(SHM_SERVER_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd_ < 0) {
        std::cerr << "[IPC Error] Failed to create POSIX shared memory.\n";
        return false;
    }

    if (ftruncate(shm_fd_, sizeof(ServerTelemetry)) == -1) {
        std::cerr << "[IPC Error] ftruncate failed on shared memory.\n";
        return false;
    }

    void* addr = mmap(nullptr, sizeof(ServerTelemetry), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
    if (addr == MAP_FAILED) {
        std::cerr << "[IPC Error] mmap failed.\n";
        return false;
    }

    telemetry_ptr_ = static_cast<ServerTelemetry*>(addr);
    std::memset(telemetry_ptr_, 0, sizeof(ServerTelemetry));

    // Initialize cross-process mutex
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&telemetry_ptr_->shm_mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);

    telemetry_ptr_->server_pid = static_cast<uint32_t>(getpid());
    return true;

#else
    // Windows in-memory emulation
    static ServerTelemetry local_shm;
    telemetry_ptr_ = &local_shm;
    std::memset(telemetry_ptr_, 0, sizeof(ServerTelemetry));
    pthread_mutex_init(&telemetry_ptr_->shm_mutex, nullptr);
    telemetry_ptr_->server_pid = 1000;
    return true;
#endif
}

bool SharedMemoryManager::init_as_client() {
    is_server_ = false;

#ifndef _WIN32
    shm_fd_ = shm_open(SHM_SERVER_NAME, O_RDWR, 0666);
    if (shm_fd_ < 0) {
        return false;
    }

    void* addr = mmap(nullptr, sizeof(ServerTelemetry), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
    if (addr == MAP_FAILED) {
        return false;
    }
    telemetry_ptr_ = static_cast<ServerTelemetry*>(addr);
    return true;

#else
    return init_as_server();
#endif
}

void SharedMemoryManager::cleanup() {
    if (telemetry_ptr_) {
#ifndef _WIN32
        munmap(telemetry_ptr_, sizeof(ServerTelemetry));
        if (is_server_) {
            shm_unlink(SHM_SERVER_NAME);
        }
#endif
        telemetry_ptr_ = nullptr;
    }
    if (shm_fd_ >= 0) {
#ifndef _WIN32
        close(shm_fd_);
#endif
        shm_fd_ = -1;
    }
}

void SharedMemoryManager::update_telemetry(
    uint32_t clients, uint32_t workers, uint32_t submissions,
    uint32_t sem_val, bool race_mode, uint64_t mtx_count,
    int64_t counter_val, uint32_t faults, uint32_t disk_mov
) {
    if (!telemetry_ptr_) return;

    pthread_mutex_lock(&telemetry_ptr_->shm_mutex);
    telemetry_ptr_->active_clients = clients;
    telemetry_ptr_->active_worker_threads = workers;
    telemetry_ptr_->total_submissions = submissions;
    telemetry_ptr_->judge_semaphore_val = sem_val;
    telemetry_ptr_->race_condition_mode = race_mode ? 1 : 0;
    telemetry_ptr_->mutex_acquisitions = mtx_count;
    telemetry_ptr_->shared_counter_val = counter_val;
    telemetry_ptr_->page_faults = faults;
    telemetry_ptr_->total_disk_head_movement = disk_mov;
    pthread_mutex_unlock(&telemetry_ptr_->shm_mutex);
}

void SharedMemoryManager::append_log(const std::string& msg) {
    if (!telemetry_ptr_) return;

    pthread_mutex_lock(&telemetry_ptr_->shm_mutex);
    uint32_t idx = (telemetry_ptr_->log_head + telemetry_ptr_->log_count) % MAX_LOG_ENTRIES;
    if (telemetry_ptr_->log_count == MAX_LOG_ENTRIES) {
        telemetry_ptr_->log_head = (telemetry_ptr_->log_head + 1) % MAX_LOG_ENTRIES;
    } else {
        telemetry_ptr_->log_count++;
    }

    auto now = std::chrono::system_clock::now().time_since_epoch();
    telemetry_ptr_->logs[idx].timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();
    std::strncpy(telemetry_ptr_->logs[idx].message, msg.c_str(), MAX_LOG_MSG_LEN - 1);
    telemetry_ptr_->logs[idx].message[MAX_LOG_MSG_LEN - 1] = '\0';

    pthread_mutex_unlock(&telemetry_ptr_->shm_mutex);
}

ServerTelemetry* SharedMemoryManager::get_telemetry() {
    return telemetry_ptr_;
}
