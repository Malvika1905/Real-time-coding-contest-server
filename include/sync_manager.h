#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include "posix_compat.h"
#include <atomic>
#include <cstdint>
#include <string>

// Global toggle for demonstrating race conditions vs synchronized execution
extern std::atomic<bool> g_race_condition_mode;

// Statistics for OS Synchronization demonstration
struct SyncStats {
    std::atomic<uint64_t> mutex_acquisitions{0};
    std::atomic<uint64_t> rwlock_read_acquisitions{0};
    std::atomic<uint64_t> rwlock_write_acquisitions{0};
    std::atomic<uint64_t> semaphore_wait_count{0};
    std::atomic<uint64_t> race_condition_updates{0};
    std::atomic<uint64_t> lost_updates_detected{0};
};

class SyncManager {
public:
    static SyncManager& instance();

    // Mutex operations
    void lock_leaderboard();
    void unlock_leaderboard();

    void lock_users();
    void unlock_users();

    void lock_submissions();
    void unlock_submissions();

    // Reader-Writer Lock for Leaderboard (Module 3: Reader-Writer Problem)
    void read_lock_leaderboard();
    void read_unlock_leaderboard();
    void write_lock_leaderboard();
    void write_unlock_leaderboard();

    // Counting Semaphore for Judge Worker Slots (Module 3: Counting Semaphores)
    bool acquire_judge_slot();
    void release_judge_slot();
    int get_available_judge_slots();

    // Race Condition Demonstration Counter
    // Multiple threads will increment this concurrently to prove lost updates
    void demo_increment_counter_unsynchronized(int iterations);
    void demo_increment_counter_synchronized(int iterations);
    int64_t get_demo_counter() const;
    void reset_demo_counter();

    // Statistics getter
    const SyncStats& stats() const { return stats_; }

private:
    SyncManager();
    ~SyncManager();

    // Prevent copies
    SyncManager(const SyncManager&) = delete;
    SyncManager& operator=(const SyncManager&) = delete;

    pthread_mutex_t leaderboard_mutex_;
    pthread_mutex_t users_mutex_;
    pthread_mutex_t submissions_mutex_;

    pthread_rwlock_t leaderboard_rwlock_;
    sem_t judge_semaphore_;

    int64_t volatile demo_shared_counter_{0};
    pthread_mutex_t demo_counter_mutex_;

    SyncStats stats_;
};

// RAII Mutex Guard
class ScopedMutex {
public:
    explicit ScopedMutex(pthread_mutex_t* mtx) : mtx_(mtx) {
        pthread_mutex_lock(mtx_);
    }
    ~ScopedMutex() {
        pthread_mutex_unlock(mtx_);
    }
private:
    pthread_mutex_t* mtx_;
};

#endif // SYNC_MANAGER_H
