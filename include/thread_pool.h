#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "posix_compat.h"
#include <vector>
#include <queue>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads, size_t max_queue_size = 256);
    ~ThreadPool();

    // Prevent copy
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Enqueue a new work task (bounded producer-consumer with blocking wait)
    bool enqueue(std::function<void()> task);

    // Thread pool metrics for the monitoring dashboard & evaluation
    size_t total_threads() const { return workers_.size(); }
    int active_workers() const { return active_workers_.load(); }
    size_t pending_tasks() const;
    size_t max_capacity() const { return max_queue_size_; }
    uint64_t completed_tasks() const { return completed_tasks_.load(); }

    void shutdown();

private:
    static void* worker_routine(void* arg);

    std::vector<pthread_t> workers_;
    std::queue<std::function<void()>> task_queue_;
    size_t max_queue_size_{256};

    pthread_mutex_t queue_mutex_;
    pthread_cond_t  queue_cond_;      // signals worker when task available (not empty)
    pthread_cond_t  queue_not_full_;  // signals producer when queue space freed (not full)

    std::atomic<bool>     stop_flag_{false};
    std::atomic<int>      active_workers_{0};
    std::atomic<uint64_t> completed_tasks_{0};
};

#endif // THREAD_POOL_H
