#include "thread_pool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t num_threads, size_t max_queue_size)
    : max_queue_size_(max_queue_size > 0 ? max_queue_size : 256) {
    pthread_mutex_init(&queue_mutex_, nullptr);
    pthread_cond_init(&queue_cond_, nullptr);
    pthread_cond_init(&queue_not_full_, nullptr);
    stop_flag_.store(false);
    active_workers_.store(0);
    completed_tasks_.store(0);

    workers_.resize(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        if (pthread_create(&workers_[i], nullptr, worker_routine, this) != 0) {
            std::cerr << "[ThreadPool Error] Failed to create pthread " << i << "\n";
        }
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    if (stop_flag_.load()) return;

    pthread_mutex_lock(&queue_mutex_);
    stop_flag_.store(true);
    pthread_cond_broadcast(&queue_cond_);
    pthread_cond_broadcast(&queue_not_full_);
    pthread_mutex_unlock(&queue_mutex_);

    for (size_t i = 0; i < workers_.size(); ++i) {
        pthread_join(workers_[i], nullptr);
    }
    workers_.clear();

    pthread_mutex_destroy(&queue_mutex_);
    pthread_cond_destroy(&queue_cond_);
    pthread_cond_destroy(&queue_not_full_);
}

bool ThreadPool::enqueue(std::function<void()> task) {
    if (stop_flag_.load()) return false;

    pthread_mutex_lock(&queue_mutex_);
    // Bounded Producer-Consumer: wait if queue is at max capacity
    while (task_queue_.size() >= max_queue_size_ && !stop_flag_.load()) {
        pthread_cond_wait(&queue_not_full_, &queue_mutex_);
    }

    if (stop_flag_.load()) {
        pthread_mutex_unlock(&queue_mutex_);
        return false;
    }

    task_queue_.push(std::move(task));
    pthread_cond_signal(&queue_cond_);
    pthread_mutex_unlock(&queue_mutex_);
    return true;
}

size_t ThreadPool::pending_tasks() const {
    auto* self = const_cast<ThreadPool*>(this);
    pthread_mutex_lock(&self->queue_mutex_);
    size_t sz = self->task_queue_.size();
    pthread_mutex_unlock(&self->queue_mutex_);
    return sz;
}

void* ThreadPool::worker_routine(void* arg) {
    auto* pool = static_cast<ThreadPool*>(arg);

    while (true) {
        std::function<void()> task;

        pthread_mutex_lock(&pool->queue_mutex_);
        while (pool->task_queue_.empty() && !pool->stop_flag_.load()) {
            pthread_cond_wait(&pool->queue_cond_, &pool->queue_mutex_);
        }

        if (pool->stop_flag_.load() && pool->task_queue_.empty()) {
            pthread_mutex_unlock(&pool->queue_mutex_);
            break;
        }

        task = std::move(pool->task_queue_.front());
        pool->task_queue_.pop();

        // Signal waiting producers that space is now available in bounded queue
        pthread_cond_signal(&pool->queue_not_full_);
        pthread_mutex_unlock(&pool->queue_mutex_);

        // Execute task
        pool->active_workers_++;
        try {
            task();
        } catch (const std::exception& e) {
            std::cerr << "[Worker Thread Exception] " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[Worker Thread Exception] Unknown error\n";
        }
        pool->active_workers_--;
        pool->completed_tasks_++;
    }

    return nullptr;
}
