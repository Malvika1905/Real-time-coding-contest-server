#include "leaderboard.h"
#include "sync_manager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

Leaderboard::Leaderboard() {}

void Leaderboard::record_solution(const std::string& username, int points, uint64_t submission_time_sec, bool is_new_solve) {
    SyncManager::instance().write_lock_leaderboard();

    auto& entry = scores_[username];
    entry.username = username;
    if (is_new_solve) {
        entry.score += points;
        entry.solved_count += 1;
        entry.total_penalty_sec += submission_time_sec;
    }

    SyncManager::instance().write_unlock_leaderboard();
}

std::vector<LeaderboardEntry> Leaderboard::get_ranked_entries() {
    SyncManager::instance().read_lock_leaderboard();

    std::vector<LeaderboardEntry> entries;
    for (const auto& pair : scores_) {
        LeaderboardEntry item;
        item.username = pair.second.username;
        item.score = pair.second.score;
        item.solved_count = pair.second.solved_count;
        item.total_penalty_sec = pair.second.total_penalty_sec;
        item.rank = 0;
        entries.push_back(item);
    }

    SyncManager::instance().read_unlock_leaderboard();

    // Sort: Higher score first; if equal, lower penalty time first
    std::sort(entries.begin(), entries.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        return a.total_penalty_sec < b.total_penalty_sec;
    });

    for (size_t i = 0; i < entries.size(); ++i) {
        entries[i].rank = static_cast<int>(i + 1);
    }

    return entries;
}

std::string Leaderboard::get_formatted_table() {
    std::vector<LeaderboardEntry> ranked = get_ranked_entries();

    std::stringstream ss;
    ss << "========================================================================\n";
    ss << "                      LIVE CONTEST LEADERBOARD                          \n";
    ss << "========================================================================\n";
    ss << std::left 
       << std::setw(6)  << "RANK" 
       << std::setw(20) << "USERNAME" 
       << std::setw(10) << "SOLVED" 
       << std::setw(12) << "SCORE" 
       << std::setw(16) << "PENALTY (SEC)" 
       << "\n";
    ss << "------------------------------------------------------------------------\n";

    if (ranked.empty()) {
        ss << "           No submissions recorded yet on the leaderboard.             \n";
    } else {
        for (const auto& entry : ranked) {
            ss << std::left 
               << std::setw(6)  << entry.rank 
               << std::setw(20) << entry.username 
               << std::setw(10) << entry.solved_count 
               << std::setw(12) << entry.score 
               << std::setw(16) << entry.total_penalty_sec 
               << "\n";
        }
    }
    ss << "========================================================================\n";
    return ss.str();
}

void Leaderboard::clear() {
    SyncManager::instance().write_lock_leaderboard();
    scores_.clear();
    SyncManager::instance().write_unlock_leaderboard();
}
