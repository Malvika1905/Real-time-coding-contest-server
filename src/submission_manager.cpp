#include "submission_manager.h"
#include <algorithm>

SubmissionManager& SubmissionManager::instance() {
    static SubmissionManager mgr;
    return mgr;
}

SubmissionManager::SubmissionManager() {
    pthread_mutex_init(&mtx_, nullptr);
}

SubmissionManager::~SubmissionManager() {
    pthread_mutex_destroy(&mtx_);
}

void SubmissionManager::record_submission(const SubmissionRecord& record) {
    pthread_mutex_lock(&mtx_);
    submissions_.push_back(record);
    pthread_mutex_unlock(&mtx_);
}

std::vector<SubmissionRecord> SubmissionManager::get_all_submissions() const {
    pthread_mutex_lock(&mtx_);
    std::vector<SubmissionRecord> copy = submissions_;
    pthread_mutex_unlock(&mtx_);
    return copy;
}

std::vector<SubmissionRecord> SubmissionManager::get_user_submissions(const std::string& username) const {
    pthread_mutex_lock(&mtx_);
    std::vector<SubmissionRecord> filtered;
    for (const auto& sub : submissions_) {
        if (sub.username == username) {
            filtered.push_back(sub);
        }
    }
    pthread_mutex_unlock(&mtx_);
    return filtered;
}

bool SubmissionManager::get_submission(uint64_t id, SubmissionRecord& out_record) const {
    pthread_mutex_lock(&mtx_);
    for (const auto& sub : submissions_) {
        if (sub.id == id) {
            out_record = sub;
            pthread_mutex_unlock(&mtx_);
            return true;
        }
    }
    pthread_mutex_unlock(&mtx_);
    return false;
}

size_t SubmissionManager::total_submissions() const {
    pthread_mutex_lock(&mtx_);
    size_t sz = submissions_.size();
    pthread_mutex_unlock(&mtx_);
    return sz;
}

void SubmissionManager::get_verdict_stats(int& accepted, int& rejected) const {
    pthread_mutex_lock(&mtx_);
    accepted = 0;
    rejected = 0;
    for (const auto& sub : submissions_) {
        if (sub.verdict.find("ACCEPTED") != std::string::npos) {
            accepted++;
        } else {
            rejected++;
        }
    }
    pthread_mutex_unlock(&mtx_);
}

void SubmissionManager::clear() {
    pthread_mutex_lock(&mtx_);
    submissions_.clear();
    pthread_mutex_unlock(&mtx_);
}
