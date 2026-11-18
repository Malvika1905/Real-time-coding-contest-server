#include "sync_manager.h"
#include <iostream>

#ifndef _WIN32
#include <sched.h>
#include <unistd.h>
#endif

std::atomic<bool> g_race_condition_mode{false};

SyncManager& SyncManager::instance() {
    static SyncManager mgr;
    return mgr;
}

SyncManager::SyncManager() {
    pthread_mutex_init(&leaderboard_mutex_, nullptr);
    pthread_mutex_init(&users_mutex_, nullptr);
    pthread_mutex_init(&submissions_mutex_, nullptr);
    pthread_mutex_init(&demo_counter_mutex_, nullptr);

    pthread_rwlock_init(&leaderboard_rwlock_, nullptr);

    // Module 3: POSIX Counting Semaphore
    // Allow up to 3 concurrent judge sandbox evaluation processes
    sem_init(&judge_semaphore_, 0, 3);
}

SyncManager::~SyncManager() {
    pthread_mutex_destroy(&leaderboard_mutex_);
    pthread_mutex_destroy(&users_mutex_);
    pthread_mutex_destroy(&submissions_mutex_);
    pthread_mutex_destroy(&demo_counter_mutex_);

    pthread_rwlock_destroy(&leaderboard_rwlock_);
    sem_destroy(&judge_semaphore_);
}

void SyncManager::lock_leaderboard() {
    pthread_mutex_lock(&leaderboard_mutex_);
    stats_.mutex_acquisitions++;
}

void SyncManager::unlock_leaderboard() {
    pthread_mutex_unlock(&leaderboard_mutex_);
}

void SyncManager::lock_users() {
    pthread_mutex_lock(&users_mutex_);
    stats_.mutex_acquisitions++;
}

void SyncManager::unlock_users() {
    pthread_mutex_unlock(&users_mutex_);
}

void SyncManager::lock_submissions() {
    pthread_mutex_lock(&submissions_mutex_);
    stats_.mutex_acquisitions++;
}

void SyncManager::unlock_submissions() {
    pthread_mutex_unlock(&submissions_mutex_);
}

void SyncManager::read_lock_leaderboard() {
    pthread_rwlock_rdlock(&leaderboard_rwlock_);
    stats_.rwlock_read_acquisitions++;
}

void SyncManager::read_unlock_leaderboard() {
    pthread_rwlock_unlock(&leaderboard_rwlock_);
}

void SyncManager::write_lock_leaderboard() {
    pthread_rwlock_wrlock(&leaderboard_rwlock_);
    stats_.rwlock_write_acquisitions++;
}

void SyncManager::write_unlock_leaderboard() {
    pthread_rwlock_unlock(&leaderboard_rwlock_);
}

bool SyncManager::acquire_judge_slot() {
    stats_.semaphore_wait_count++;
    return (sem_wait(&judge_semaphore_) == 0);
}

void SyncManager::release_judge_slot() {
    sem_post(&judge_semaphore_);
}

int SyncManager::get_available_judge_slots() {
    int val = 0;
    sem_getvalue(&judge_semaphore_, &val);
    return val;
}

// -------------------------------------------------------------------------
// Module 3 Demonstration: Race Conditions vs Synchronized Execution
// -------------------------------------------------------------------------

void SyncManager::demo_increment_counter_unsynchronized(int iterations) {
    stats_.race_condition_updates += iterations;
    for (int i = 0; i < iterations; ++i) {
        // Intentionally non-atomic read-modify-write to illustrate critical section violation
        int64_t current = demo_shared_counter_;
        
        // Context switch pressure: yield processor or introduce tiny delay
#ifndef _WIN32
        sched_yield();
#endif
        demo_shared_counter_ = current + 1;
    }
}

void SyncManager::demo_increment_counter_synchronized(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        pthread_mutex_lock(&demo_counter_mutex_);
        demo_shared_counter_ = demo_shared_counter_ + 1;
        stats_.mutex_acquisitions++;
        pthread_mutex_unlock(&demo_counter_mutex_);
    }
}

int64_t SyncManager::get_demo_counter() const {
    return demo_shared_counter_;
}

void SyncManager::reset_demo_counter() {
    pthread_mutex_lock(&demo_counter_mutex_);
    demo_shared_counter_ = 0;
    pthread_mutex_unlock(&demo_counter_mutex_);
}
