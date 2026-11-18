#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

struct LeaderboardEntry {
    std::string username;
    int score;
    int solved_count;
    uint64_t total_penalty_sec;
    int rank;
};

class Leaderboard {
public:
    Leaderboard();
    
    // Update or record score for a user
    void record_solution(const std::string& username, int points, uint64_t submission_time_sec, bool is_new_solve = true);
    
    // Retrieve formatted leaderboard (thread-safe)
    std::string get_formatted_table();
    
    // Retrieve structured list
    std::vector<LeaderboardEntry> get_ranked_entries();

    // Reset leaderboard (for demos)
    void clear();

private:
    struct UserScoreData {
        std::string username;
        int score{0};
        int solved_count{0};
        uint64_t total_penalty_sec{0};
    };

    std::map<std::string, UserScoreData> scores_;
};

#endif // LEADERBOARD_H
