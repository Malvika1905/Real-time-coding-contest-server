#include "http_server.h"
#include "contest.h"
#include "leaderboard.h"
#include "judge_sandbox.h"
#include "sync_manager.h"
#include "ipc_shared_memory.h"
#include "bankers_algo.h"
#include "memory_sim.h"
#include "disk_sim.h"

#include "submission_manager.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <cstring>
#include <thread>
#include <algorithm>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif

extern ContestManager g_contest;
extern Leaderboard    g_leaderboard;
extern BankersManager g_bankers;
extern std::atomic<uint32_t> g_connected_clients;
extern std::atomic<uint32_t> g_total_submissions;
extern std::atomic<uint32_t> g_total_page_faults;
extern std::atomic<uint32_t> g_total_disk_movement;

static std::atomic<uint64_t> g_run_counter{900000};

// Helper to check if packet is HTTP
bool is_http_request(const char* buffer, size_t length) {
    if (length < 4) return false;
    return (std::strncmp(buffer, "GET ", 4) == 0 ||
            std::strncmp(buffer, "POST", 4) == 0 ||
            std::strncmp(buffer, "OPTI", 4) == 0 ||
            std::strncmp(buffer, "PUT ", 4) == 0 ||
            std::strncmp(buffer, "DELE", 4) == 0 ||
            std::strncmp(buffer, "HEAD", 4) == 0);
}

static std::string escape_json(const std::string& input) {
    std::string output;
    for (char c : input) {
        if (c == '"') output += "\\\"";
        else if (c == '\\') output += "\\\\";
        else if (c == '\b') output += "\\b";
        else if (c == '\f') output += "\\f";
        else if (c == '\n') output += "\\n";
        else if (c == '\r') output += "\\r";
        else if (c == '\t') output += "\\t";
        else if (static_cast<unsigned char>(c) < 32) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", c);
            output += buf;
        } else {
            output += c;
        }
    }
    return output;
}

static std::string unescape_json(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char next = input[i + 1];
            if (next == 'n') { out += '\n'; i++; }
            else if (next == 'r') { out += '\r'; i++; }
            else if (next == 't') { out += '\t'; i++; }
            else if (next == '"') { out += '"'; i++; }
            else if (next == '\\') { out += '\\'; i++; }
            else { out += next; i++; }
        } else {
            out += input[i];
        }
    }
    return out;
}

static std::string extract_json_field(const std::string& json, const std::string& field) {
    std::string needle = "\"" + field + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";

    pos += needle.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) pos++;
    if (pos >= json.length()) return "";

    if (json[pos] == '"') {
        pos++;
        std::string raw;
        while (pos < json.length()) {
            if (json[pos] == '\\' && pos + 1 < json.length()) {
                raw += json[pos];
                raw += json[pos + 1];
                pos += 2;
            } else if (json[pos] == '"') {
                break;
            } else {
                raw += json[pos++];
            }
        }
        return unescape_json(raw);
    } else {
        size_t end = json.find_first_of(",}\r\n ", pos);
        if (end == std::string::npos) end = json.length();
        return json.substr(pos, end - pos);
    }
}

static std::string extract_header(const std::string& req, const std::string& header_name) {
    std::string needle = header_name + ":";
    size_t pos = req.find(needle);
    if (pos == std::string::npos) {
        std::string lower_req = req;
        for (char& c : lower_req) c = tolower(c);
        std::string lower_needle = header_name + ":";
        for (char& c : lower_needle) c = tolower(c);
        pos = lower_req.find(lower_needle);
        if (pos == std::string::npos) return "";
    }
    pos += needle.length();
    size_t end = req.find("\r\n", pos);
    if (end == std::string::npos) end = req.length();
    std::string val = req.substr(pos, end - pos);
    size_t f = val.find_first_not_of(" \t");
    size_t l = val.find_last_not_of(" \t\r\n");
    if (f == std::string::npos) return "";
    return val.substr(f, l - f + 1);
}

static bool is_authorized_admin(const std::string& req, std::string& out_admin_user) {
    std::string auth = extract_header(req, "Authorization");
    std::string token;
    if (auth.rfind("Bearer ", 0) == 0) {
        token = auth.substr(7);
    } else {
        token = extract_header(req, "X-Admin-Token");
    }
    size_t f = token.find_first_not_of(" \t\r\n");
    size_t l = token.find_last_not_of(" \t\r\n");
    if (f == std::string::npos) token = "";
    else token = token.substr(f, l - f + 1);

    SyncManager::instance().lock_users();
    bool ok = g_contest.validate_admin_token(token, out_admin_user);
    SyncManager::instance().unlock_users();
    return ok;
}

static bool extract_session_user(const std::string& req, std::string& out_user) {
    std::string auth = extract_header(req, "Authorization");
    std::string token;
    if (auth.rfind("Bearer ", 0) == 0) {
        token = auth.substr(7);
    } else {
        token = extract_header(req, "X-Session-Token");
    }
    size_t f = token.find_first_not_of(" \t\r\n");
    size_t l = token.find_last_not_of(" \t\r\n");
    if (f == std::string::npos) token = "";
    else token = token.substr(f, l - f + 1);

    if (token.empty()) return false;
    return g_contest.get_username_from_session(token, out_user);
}

static void send_http_response(int client_fd, int status_code, const std::string& status_text,
                               const std::string& content_type, const std::string& body) {
    std::stringstream ss;
    ss << "HTTP/1.1 " << status_code << " " << status_text << "\r\n";
    ss << "Content-Type: " << content_type << "\r\n";
    ss << "Content-Length: " << body.length() << "\r\n";
    ss << "Access-Control-Allow-Origin: *\r\n";
    ss << "Access-Control-Allow-Methods: GET, POST, OPTIONS, PUT, DELETE\r\n";
    ss << "Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With, X-Admin-Token, X-Session-Token, *\r\n";
    ss << "Connection: close\r\n\r\n";
    ss << body;

    std::string resp = ss.str();
#ifdef _WIN32
    send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
#else
    send(client_fd, resp.c_str(), resp.length(), MSG_NOSIGNAL);
#endif
}

bool handle_http_connection(int client_fd, const std::string& req) {
    std::istringstream stream(req);
    std::string method, path, version;
    stream >> method >> path >> version;

    // Handle CORS Pre-flight Options Request
    if (method == "OPTIONS") {
        send_http_response(client_fd, 200, "OK", "text/plain", "");
        return true;
    }

    // Locate HTTP Body
    size_t body_pos = req.find("\r\n\r\n");
    std::string body = (body_pos != std::string::npos) ? req.substr(body_pos + 4) : "";

    // Strip Query Parameters from path for routing
    std::string clean_path = path;
    std::string query_params = "";
    size_t qpos = path.find('?');
    if (qpos != std::string::npos) {
        clean_path = path.substr(0, qpos);
        query_params = path.substr(qpos + 1);
    }

    // =========================================================================
    // ADMIN ROUTES (ROLE-BASED ACCESS CONTROL)
    // =========================================================================

    // -------------------------------------------------------------------------
    // Admin Route 1: GET /api/admin/verify
    // -------------------------------------------------------------------------
    if (method == "GET" && clean_path == "/api/admin/verify") {
        std::string admin_user;
        if (!is_authorized_admin(req, admin_user)) {
            send_http_response(client_fd, 403, "Forbidden", "application/json",
                "{\"success\":false,\"error\":\"Access denied. Administrator privileges required.\"}");
            return true;
        }
        std::stringstream ss;
        ss << "{\n"
           << "  \"success\": true,\n"
           << "  \"username\": \"" << escape_json(admin_user) << "\",\n"
           << "  \"role\": \"admin\"\n"
           << "}";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // -------------------------------------------------------------------------
    // Admin Route 2: GET /api/admin/dashboard
    // -------------------------------------------------------------------------
    if (method == "GET" && clean_path == "/api/admin/dashboard") {
        std::string admin_user;
        if (!is_authorized_admin(req, admin_user)) {
            send_http_response(client_fd, 403, "Forbidden", "application/json",
                "{\"error\":\"Access denied. Administrator privileges required.\"}");
            return true;
        }

        SyncManager::instance().lock_users();
        std::vector<User> all_users = g_contest.get_all_users();
        SyncManager::instance().unlock_users();

        std::vector<Contest> all_contests = g_contest.get_all_contests();
        std::vector<Problem> all_problems = g_contest.get_all_problems();

        int active_contests = 0;
        for (const auto& c : all_contests) {
            if (c.status == "ACTIVE") active_contests++;
        }

        int total_submissions = static_cast<int>(SubmissionManager::instance().total_submissions());
        int accepted_submissions = 0;
        int rejected_submissions = 0;
        SubmissionManager::instance().get_verdict_stats(accepted_submissions, rejected_submissions);

        ServerTelemetry* t = SharedMemoryManager::instance().get_telemetry();
        SyncManager& sm = SyncManager::instance();

        std::stringstream ss;
        ss << "{\n"
           << "  \"total_participants\": " << all_users.size() << ",\n"
           << "  \"total_contests\": " << all_contests.size() << ",\n"
           << "  \"active_contests\": " << active_contests << ",\n"
           << "  \"total_problems\": " << all_problems.size() << ",\n"
           << "  \"total_submissions\": " << total_submissions << ",\n"
           << "  \"accepted_submissions\": " << accepted_submissions << ",\n"
           << "  \"rejected_submissions\": " << rejected_submissions << ",\n"
           << "  \"connected_clients\": " << (t ? t->active_clients : g_connected_clients.load()) << ",\n"
           << "  \"active_worker_threads\": " << (t ? t->active_worker_threads : 2) << ",\n"
           << "  \"total_worker_threads\": 8,\n"
           << "  \"judge_semaphore_slots\": " << (t ? t->judge_semaphore_val : sm.get_available_judge_slots()) << ",\n"
           << "  \"judge_semaphore_max\": 3,\n"
           << "  \"recent_activity\": [\n";

        if (t && t->log_count > 0) {
            uint32_t count = t->log_count;
            uint32_t head = t->log_head;
            uint32_t show_count = count > 15 ? 15 : count;
            for (uint32_t i = 0; i < show_count; ++i) {
                uint32_t idx = (head + count - 1 - i) % MAX_LOG_ENTRIES;
                ss << "    {\n"
                   << "      \"timestamp\": " << t->logs[idx].timestamp << ",\n"
                   << "      \"message\": \"" << escape_json(t->logs[idx].message) << "\"\n"
                   << "    }" << (i + 1 < show_count ? ",\n" : "\n");
            }
        }
        ss << "  ]\n"
           << "}";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // -------------------------------------------------------------------------
    // Admin Route 3: Contest Management (GET, POST, PUT, DELETE)
    // -------------------------------------------------------------------------
    if (clean_path == "/api/admin/contests" || clean_path == "/api/admin/contests/") {
        std::string admin_user;
        if (!is_authorized_admin(req, admin_user)) {
            send_http_response(client_fd, 403, "Forbidden", "application/json", "{\"error\":\"Unauthorized\"}");
            return true;
        }

        if (method == "GET") {
            std::vector<Contest> list = g_contest.get_all_contests();
            std::stringstream ss;
            ss << "[\n";
            for (size_t i = 0; i < list.size(); ++i) {
                const auto& c = list[i];
                ss << "  {\n"
                   << "    \"id\": " << c.id << ",\n"
                   << "    \"title\": \"" << escape_json(c.title) << "\",\n"
                   << "    \"description\": \"" << escape_json(c.description) << "\",\n"
                   << "    \"status\": \"" << escape_json(c.status) << "\",\n"
                   << "    \"start_time\": " << c.start_time << ",\n"
                   << "    \"end_time\": " << c.end_time << ",\n"
                   << "    \"duration_minutes\": " << c.duration_minutes << ",\n"
                   << "    \"problem_count\": " << c.problem_ids.size() << ",\n"
                   << "    \"participant_count\": " << c.registered_participants.size() << "\n"
                   << "  }" << (i + 1 < list.size() ? ",\n" : "\n");
            }
            ss << "]";
            send_http_response(client_fd, 200, "OK", "application/json", ss.str());
            return true;
        }

        if (method == "POST") {
            Contest c;
            c.title = extract_json_field(body, "title");
            c.description = extract_json_field(body, "description");
            c.status = extract_json_field(body, "status");
            std::string dur_str = extract_json_field(body, "duration_minutes");
            try { c.duration_minutes = std::stoi(dur_str); } catch (...) { c.duration_minutes = 120; }
            if (c.title.empty()) c.title = "Untitled Contest";
            if (c.status.empty()) c.status = "DRAFT";

            uint64_t now_sec = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            c.start_time = now_sec;
            c.end_time = now_sec + c.duration_minutes * 60;
            c.problem_ids = {1, 2, 3};

            int cid = g_contest.create_contest(c);
            SharedMemoryManager::instance().append_log("Contest created: " + c.title + " (ID " + std::to_string(cid) + ")");

            std::stringstream ss;
            ss << "{\"success\":true,\"id\":" << cid << ",\"message\":\"Contest created successfully\"}";
            send_http_response(client_fd, 201, "Created", "application/json", ss.str());
            return true;
        }
    }

    // Admin Contest Operations on specific ID (/api/admin/contests/:id)
    if (clean_path.rfind("/api/admin/contests/", 0) == 0) {
        std::string admin_user;
        if (!is_authorized_admin(req, admin_user)) {
            send_http_response(client_fd, 403, "Forbidden", "application/json", "{\"error\":\"Unauthorized\"}");
            return true;
        }

        std::string subpath = clean_path.substr(20);
        size_t slash_pos = subpath.find('/');
        int cid = 0;
        try { cid = std::stoi(slash_pos == std::string::npos ? subpath : subpath.substr(0, slash_pos)); } catch (...) {}

        if (slash_pos != std::string::npos && subpath.substr(slash_pos) == "/status" && method == "POST") {
            std::string new_status = extract_json_field(body, "status");
            bool ok = g_contest.set_contest_status(cid, new_status);
            if (ok) {
                SharedMemoryManager::instance().append_log("Contest #" + std::to_string(cid) + " status changed to " + new_status);
                send_http_response(client_fd, 200, "OK", "application/json", "{\"success\":true}");
            } else {
                send_http_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Contest not found\"}");
            }
            return true;
        }

        if (method == "DELETE") {
            bool ok = g_contest.delete_contest(cid);
            if (ok) {
                SharedMemoryManager::instance().append_log("Contest #" + std::to_string(cid) + " deleted by admin");
                send_http_response(client_fd, 200, "OK", "application/json", "{\"success\":true}");
            } else {
                send_http_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Contest not found\"}");
            }
            return true;
        }

        if (method == "PUT") {
            const Contest* exist = g_contest.get_contest(cid);
            if (!exist) {
                send_http_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Contest not found\"}");
                return true;
            }
            Contest c = *exist;
            std::string t = extract_json_field(body, "title");
            std::string d = extract_json_field(body, "description");
            std::string s = extract_json_field(body, "status");
            std::string dur = extract_json_field(body, "duration_minutes");

            if (!t.empty()) c.title = t;
            if (!d.empty()) c.description = d;
            if (!s.empty()) c.status = s;
            try { if (!dur.empty()) c.duration_minutes = std::stoi(dur); } catch (...) {}

            g_contest.update_contest(c);
            SharedMemoryManager::instance().append_log("Contest #" + std::to_string(cid) + " updated by admin");
            send_http_response(client_fd, 200, "OK", "application/json", "{\"success\":true}");
            return true;
        }
    }

    // -------------------------------------------------------------------------
    // Admin Route 4: Problem Management (GET, POST, PUT, DELETE)
    // -------------------------------------------------------------------------
    if (clean_path == "/api/admin/problems" || clean_path == "/api/admin/problems/") {
        std::string admin_user;
        if (!is_authorized_admin(req, admin_user)) {
            send_http_response(client_fd, 403, "Forbidden", "application/json", "{\"error\":\"Unauthorized\"}");
            return true;
        }

        if (method == "GET") {
            std::vector<Problem> list = g_contest.get_all_problems();
            std::stringstream ss;
            ss << "[\n";
            for (size_t i = 0; i < list.size(); ++i) {
                const auto& p = list[i];
                ss << "  {\n"
                   << "    \"id\": " << p.id << ",\n"
                   << "    \"title\": \"" << escape_json(p.title) << "\",\n"
                   << "    \"difficulty\": \"" << escape_json(p.difficulty) << "\",\n"
                   << "    \"points\": " << p.points << ",\n"
                   << "    \"time_limit_sec\": " << p.time_limit_sec << ",\n"
                   << "    \"memory_limit_mb\": " << p.memory_limit_mb << ",\n"
                   << "    \"description\": \"" << escape_json(p.description) << "\",\n"
                   << "    \"input_format\": \"" << escape_json(p.input_format) << "\",\n"
                   << "    \"output_format\": \"" << escape_json(p.output_format) << "\",\n"
                   << "    \"constraints\": \"" << escape_json(p.constraints) << "\",\n"
                   << "    \"sample_input\": \"" << escape_json(p.sample_input) << "\",\n"
                   << "    \"sample_output\": \"" << escape_json(p.sample_output) << "\",\n"
                   << "    \"hidden_input\": \"" << escape_json(p.hidden_input) << "\",\n"
                   << "    \"hidden_output\": \"" << escape_json(p.hidden_output) << "\"\n"
                   << "  }" << (i + 1 < list.size() ? ",\n" : "\n");
            }
            ss << "]";
            send_http_response(client_fd, 200, "OK", "application/json", ss.str());
            return true;
        }

        if (method == "POST") {
            Problem p;
            p.id = 0; // Assigned automatically
            p.title = extract_json_field(body, "title");
            p.description = extract_json_field(body, "description");
            p.difficulty = extract_json_field(body, "difficulty");
            p.input_format = extract_json_field(body, "input_format");
            p.output_format = extract_json_field(body, "output_format");
            p.constraints = extract_json_field(body, "constraints");
            p.sample_input = extract_json_field(body, "sample_input");
            p.sample_output = extract_json_field(body, "sample_output");
            p.hidden_input = extract_json_field(body, "hidden_input");
            p.hidden_output = extract_json_field(body, "hidden_output");

            std::string pts_str = extract_json_field(body, "points");
            std::string time_str = extract_json_field(body, "time_limit_sec");
            std::string mem_str = extract_json_field(body, "memory_limit_mb");

            try { p.points = std::stoi(pts_str); } catch (...) { p.points = 100; }
            try { p.time_limit_sec = std::stod(time_str); } catch (...) { p.time_limit_sec = 3.0; }
            try { p.memory_limit_mb = std::stoi(mem_str); } catch (...) { p.memory_limit_mb = 64; }

            if (p.title.empty()) p.title = "New Problem";
            if (p.hidden_input.empty()) p.hidden_input = p.sample_input;
            if (p.hidden_output.empty()) p.hidden_output = p.sample_output;

            g_contest.add_or_update_problem(p);
            SharedMemoryManager::instance().append_log("Problem added: " + p.title);
            send_http_response(client_fd, 201, "Created", "application/json", "{\"success\":true}");
            return true;
        }
    }

    if (clean_path.rfind("/api/admin/problems/", 0) == 0) {
        std::string admin_user;
        if (!is_authorized_admin(req, admin_user)) {
            send_http_response(client_fd, 403, "Forbidden", "application/json", "{\"error\":\"Unauthorized\"}");
            return true;
        }

        int pid = 0;
        try { pid = std::stoi(clean_path.substr(20)); } catch (...) {}

        if (method == "DELETE") {
            bool ok = g_contest.delete_problem(pid);
            if (ok) {
                SharedMemoryManager::instance().append_log("Problem #" + std::to_string(pid) + " deleted by admin");
                send_http_response(client_fd, 200, "OK", "application/json", "{\"success\":true}");
            } else {
                send_http_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Problem not found\"}");
            }
            return true;
        }

        if (method == "PUT") {
            const Problem* exist = g_contest.get_problem(pid);
            if (!exist) {
                send_http_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Problem not found\"}");
                return true;
            }
            Problem p = *exist;
            std::string t = extract_json_field(body, "title");
            std::string d = extract_json_field(body, "description");
            std::string diff = extract_json_field(body, "difficulty");
            std::string ifmt = extract_json_field(body, "input_format");
            std::string ofmt = extract_json_field(body, "output_format");
            std::string cons = extract_json_field(body, "constraints");
            std::string sin = extract_json_field(body, "sample_input");
            std::string sout = extract_json_field(body, "sample_output");
            std::string hin = extract_json_field(body, "hidden_input");
            std::string hout = extract_json_field(body, "hidden_output");
            std::string pts = extract_json_field(body, "points");
            std::string tlim = extract_json_field(body, "time_limit_sec");
            std::string mlim = extract_json_field(body, "memory_limit_mb");

            if (!t.empty()) p.title = t;
            if (!d.empty()) p.description = d;
            if (!diff.empty()) p.difficulty = diff;
            if (!ifmt.empty()) p.input_format = ifmt;
            if (!ofmt.empty()) p.output_format = ofmt;
            if (!cons.empty()) p.constraints = cons;
            if (!sin.empty()) p.sample_input = sin;
            if (!sout.empty()) p.sample_output = sout;
            if (!hin.empty()) p.hidden_input = hin;
            if (!hout.empty()) p.hidden_output = hout;
            try { if (!pts.empty()) p.points = std::stoi(pts); } catch (...) {}
            try { if (!tlim.empty()) p.time_limit_sec = std::stod(tlim); } catch (...) {}
            try { if (!mlim.empty()) p.memory_limit_mb = std::stoi(mlim); } catch (...) {}

            g_contest.add_or_update_problem(p);
            SharedMemoryManager::instance().append_log("Problem #" + std::to_string(pid) + " updated by admin");
            send_http_response(client_fd, 200, "OK", "application/json", "{\"success\":true}");
            return true;
        }
    }

    // -------------------------------------------------------------------------
    // Admin Route 5: Participant Management (GET, POST suspend)
    // -------------------------------------------------------------------------
    if (clean_path == "/api/admin/participants" || clean_path == "/api/admin/participants/") {
        std::string admin_user;
        if (!is_authorized_admin(req, admin_user)) {
            send_http_response(client_fd, 403, "Forbidden", "application/json", "{\"error\":\"Unauthorized\"}");
            return true;
        }

        SyncManager::instance().lock_users();
        std::vector<User> users = g_contest.get_all_users();
        SyncManager::instance().unlock_users();

        std::stringstream ss;
        ss << "[\n";
        for (size_t i = 0; i < users.size(); ++i) {
            const auto& u = users[i];
            ss << "  {\n"
               << "    \"username\": \"" << escape_json(u.username) << "\",\n"
               << "    \"role\": \"" << escape_json(u.role) << "\",\n"
               << "    \"is_suspended\": " << (u.is_suspended ? "true" : "false") << ",\n"
               << "    \"total_score\": " << u.total_score << ",\n"
               << "    \"solved_problems_count\": " << u.solved_problems_count << ",\n"
               << "    \"total_submissions\": " << u.total_submissions << ",\n"
               << "    \"created_at\": " << u.created_at << "\n"
               << "  }" << (i + 1 < users.size() ? ",\n" : "\n");
        }
        ss << "]";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    if (clean_path.rfind("/api/admin/participants/", 0) == 0 && clean_path.find("/suspend") != std::string::npos && method == "POST") {
        std::string admin_user;
        if (!is_authorized_admin(req, admin_user)) {
            send_http_response(client_fd, 403, "Forbidden", "application/json", "{\"error\":\"Unauthorized\"}");
            return true;
        }

        std::string target_user = clean_path.substr(24);
        size_t end_slash = target_user.find("/suspend");
        if (end_slash != std::string::npos) target_user = target_user.substr(0, end_slash);

        std::string susp_val = extract_json_field(body, "suspended");
        bool suspend = (susp_val == "true" || susp_val == "1");

        SyncManager::instance().lock_users();
        bool ok = g_contest.set_user_suspended(target_user, suspend);
        SyncManager::instance().unlock_users();

        if (ok) {
            SharedMemoryManager::instance().append_log("User " + target_user + (suspend ? " suspended" : " reactivated") + " by admin");
            send_http_response(client_fd, 200, "OK", "application/json", "{\"success\":true}");
        } else {
            send_http_response(client_fd, 400, "Bad Request", "application/json", "{\"error\":\"Cannot suspend administrator or user not found\"}");
        }
        return true;
    }

    // -------------------------------------------------------------------------
    // Admin Route 6: Submissions Management with Filtering
    // -------------------------------------------------------------------------
    if (clean_path == "/api/admin/submissions" || clean_path == "/api/admin/submissions/") {
        std::string admin_user;
        if (!is_authorized_admin(req, admin_user)) {
            send_http_response(client_fd, 403, "Forbidden", "application/json", "{\"error\":\"Unauthorized\"}");
            return true;
        }

        std::vector<SubmissionRecord> all_subs = SubmissionManager::instance().get_all_submissions();

        std::stringstream ss;
        ss << "[\n";
        bool first = true;
        for (int i = static_cast<int>(all_subs.size()) - 1; i >= 0; --i) {
            const auto& s = all_subs[i];
            if (!first) ss << ",\n";
            first = false;
            ss << "  {\n"
               << "    \"id\": " << s.id << ",\n"
               << "    \"username\": \"" << escape_json(s.username) << "\",\n"
               << "    \"problem_id\": " << s.problem_id << ",\n"
               << "    \"problem_title\": \"" << escape_json(s.problem_title) << "\",\n"
               << "    \"verdict\": \"" << escape_json(s.verdict) << "\",\n"
               << "    \"execution_time_ms\": " << s.execution_time_ms << ",\n"
               << "    \"memory_used_kb\": " << s.memory_used_kb << ",\n"
               << "    \"compiler_output\": \"" << escape_json(s.compiler_output) << "\",\n"
               << "    \"runtime_output\": \"" << escape_json(s.runtime_output) << "\",\n"
               << "    \"timestamp\": " << s.timestamp << ",\n"
               << "    \"code\": \"" << escape_json(s.code) << "\"\n"
               << "  }";
        }
        ss << "\n]";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // -------------------------------------------------------------------------
    // Admin Route 7: Server Activity & Diagnostics
    // -------------------------------------------------------------------------
    if (clean_path == "/api/admin/server-activity") {
        std::string admin_user;
        if (!is_authorized_admin(req, admin_user)) {
            send_http_response(client_fd, 403, "Forbidden", "application/json", "{\"error\":\"Unauthorized\"}");
            return true;
        }

        ServerTelemetry* t = SharedMemoryManager::instance().get_telemetry();
        SyncManager& sm = SyncManager::instance();

        std::stringstream ss;
        ss << "{\n";
        if (t) {
            ss << "  \"server_pid\": " << t->server_pid << ",\n"
               << "  \"active_clients\": " << t->active_clients << ",\n"
               << "  \"active_worker_threads\": " << t->active_worker_threads << ",\n"
               << "  \"total_worker_threads\": 8,\n"
               << "  \"total_submissions\": " << t->total_submissions << ",\n"
               << "  \"judge_semaphore_slots\": " << t->judge_semaphore_val << ",\n"
               << "  \"judge_semaphore_max\": 3,\n"
               << "  \"mutex_acquisitions\": " << t->mutex_acquisitions << ",\n"
               << "  \"page_faults\": " << t->page_faults << ",\n"
               << "  \"total_disk_movement\": " << t->total_disk_head_movement << ",\n"
               << "  \"logs\": [\n";

            uint32_t count = t->log_count;
            uint32_t head = t->log_head;
            for (uint32_t i = 0; i < count; ++i) {
                uint32_t idx = (head + i) % MAX_LOG_ENTRIES;
                ss << "    {\n"
                   << "      \"timestamp\": " << t->logs[idx].timestamp << ",\n"
                   << "      \"message\": \"" << escape_json(t->logs[idx].message) << "\"\n"
                   << "    }" << (i + 1 < count ? ",\n" : "\n");
            }
            ss << "  ]\n";
        } else {
            ss << "  \"server_pid\": 0,\n"
               << "  \"active_clients\": " << g_connected_clients.load() << ",\n"
               << "  \"active_worker_threads\": 2,\n"
               << "  \"total_worker_threads\": 8,\n"
               << "  \"total_submissions\": " << g_total_submissions.load() << ",\n"
               << "  \"judge_semaphore_slots\": " << sm.get_available_judge_slots() << ",\n"
               << "  \"judge_semaphore_max\": 3,\n"
               << "  \"mutex_acquisitions\": " << sm.stats().mutex_acquisitions.load() << ",\n"
               << "  \"page_faults\": " << g_total_page_faults.load() << ",\n"
               << "  \"total_disk_movement\": " << g_total_disk_movement.load() << ",\n"
               << "  \"logs\": []\n";
        }
        ss << "}";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // =========================================================================
    // PUBLIC / PARTICIPANT ROUTES
    // =========================================================================

    // -------------------------------------------------------------------------
    // Route 1: GET /api/contest
    // -------------------------------------------------------------------------
    if (method == "GET" && clean_path == "/api/contest") {
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::vector<Contest> contests = g_contest.get_all_contests();
        std::string title = "Operating Systems Lab Championship";
        std::string status = "ACTIVE";
        int duration = 180;
        if (!contests.empty()) {
            title = contests[0].title;
            status = contests[0].status;
            duration = contests[0].duration_minutes;
        }

        std::stringstream ss;
        ss << "{\n"
           << "  \"title\": \"" << escape_json(title) << "\",\n"
           << "  \"status\": \"" << escape_json(status) << "\",\n"
           << "  \"duration_minutes\": " << duration << ",\n"
           << "  \"server_time\": " << now << ",\n"
           << "  \"problems_count\": " << g_contest.get_all_problems().size() << "\n"
           << "}";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // -------------------------------------------------------------------------
    // Route 2: GET /api/problems (Public list - NO hidden test cases leaked)
    // -------------------------------------------------------------------------
    if (method == "GET" && (clean_path == "/api/problems" || clean_path == "/api/problems/")) {
        std::vector<Problem> probs = g_contest.get_all_problems();
        std::stringstream ss;
        ss << "[\n";
        for (size_t i = 0; i < probs.size(); ++i) {
            const auto& p = probs[i];
            ss << "  {\n"
               << "    \"id\": " << p.id << ",\n"
               << "    \"title\": \"" << escape_json(p.title) << "\",\n"
               << "    \"difficulty\": \"" << escape_json(p.difficulty) << "\",\n"
               << "    \"points\": " << p.points << ",\n"
               << "    \"time_limit_sec\": " << p.time_limit_sec << ",\n"
               << "    \"memory_limit_mb\": " << p.memory_limit_mb << ",\n"
               << "    \"description\": \"" << escape_json(p.description) << "\",\n"
               << "    \"input_format\": \"" << escape_json(p.input_format) << "\",\n"
               << "    \"output_format\": \"" << escape_json(p.output_format) << "\",\n"
               << "    \"constraints\": \"" << escape_json(p.constraints) << "\",\n"
               << "    \"sample_input\": \"" << escape_json(p.sample_input) << "\",\n"
               << "    \"sample_output\": \"" << escape_json(p.sample_output) << "\"\n"
               << "  }" << (i + 1 < probs.size() ? ",\n" : "\n");
        }
        ss << "]";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // -------------------------------------------------------------------------
    // Route 3: GET /api/problems/:id (Public problem details - NO hidden test cases)
    // -------------------------------------------------------------------------
    if (method == "GET" && clean_path.rfind("/api/problems/", 0) == 0) {
        int pid = 1;
        try { pid = std::stoi(clean_path.substr(14)); } catch (...) { pid = 1; }

        const Problem* p = g_contest.get_problem(pid);
        if (!p) {
            send_http_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Problem not found\"}");
            return true;
        }

        std::stringstream ss;
        ss << "{\n"
           << "  \"id\": " << p->id << ",\n"
           << "  \"title\": \"" << escape_json(p->title) << "\",\n"
           << "  \"difficulty\": \"" << escape_json(p->difficulty) << "\",\n"
           << "  \"points\": " << p->points << ",\n"
           << "  \"time_limit_sec\": " << p->time_limit_sec << ",\n"
           << "  \"memory_limit_mb\": " << p->memory_limit_mb << ",\n"
           << "  \"description\": \"" << escape_json(p->description) << "\",\n"
           << "  \"input_format\": \"" << escape_json(p->input_format) << "\",\n"
           << "  \"output_format\": \"" << escape_json(p->output_format) << "\",\n"
           << "  \"constraints\": \"" << escape_json(p->constraints) << "\",\n"
           << "  \"sample_input\": \"" << escape_json(p->sample_input) << "\",\n"
           << "  \"sample_output\": \"" << escape_json(p->sample_output) << "\"\n"
           << "}";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // -------------------------------------------------------------------------
    // Route 4: POST /api/login (Authentication & Role Resolution)
    // -------------------------------------------------------------------------
    if (method == "POST" && clean_path == "/api/login") {
        std::string u = extract_json_field(body, "username");
        std::string p = extract_json_field(body, "password");
        if (u.empty()) u = "student_guest";
        if (p.empty()) p = "password";

        SyncManager::instance().lock_users();
        User exist_user = g_contest.get_user_copy(u);
        if (exist_user.is_suspended) {
            SyncManager::instance().unlock_users();
            send_http_response(client_fd, 403, "Forbidden", "application/json",
                "{\"success\":false,\"error\":\"Your account has been suspended by an administrator.\"}");
            return true;
        }

        std::string role;
        bool ok = g_contest.register_or_login(u, p, role);
        std::string admin_token = "";
        std::string session_token = "";
        if (ok) {
            session_token = g_contest.create_user_session(u);
            if (role == "admin") {
                admin_token = g_contest.generate_admin_token(u);
            }
        }
        SyncManager::instance().unlock_users();

        if (ok) {
            SharedMemoryManager::instance().append_log("User logged in: " + u + " (" + role + ")");
            std::stringstream ss;
            ss << "{\n"
               << "  \"success\": true,\n"
               << "  \"username\": \"" << escape_json(u) << "\",\n"
               << "  \"role\": \"" << escape_json(role) << "\",\n"
               << "  \"session_token\": \"" << escape_json(session_token) << "\",\n"
               << "  \"admin_token\": \"" << escape_json(admin_token) << "\",\n"
               << "  \"message\": \"Welcome, " << escape_json(u) << "!\"\n"
               << "}";
            send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        } else {
            send_http_response(client_fd, 401, "Unauthorized", "application/json",
                "{\"success\":false,\"error\":\"Invalid credentials.\"}");
        }
        return true;
    }

    // -------------------------------------------------------------------------
    // Route 5: POST /api/submit (Official Code Evaluation with Hidden Test Cases)
    // -------------------------------------------------------------------------
    if (method == "POST" && clean_path == "/api/submit") {
        std::string u = extract_json_field(body, "username");
        std::string session_user;
        if (extract_session_user(req, session_user)) {
            u = session_user;
        }
        std::string pid_str = extract_json_field(body, "problem_id");
        std::string code = extract_json_field(body, "code");

        if (u.empty()) u = "web_participant";

        // Check Account Suspension
        SyncManager::instance().lock_users();
        User ucopy = g_contest.get_user_copy(u);
        SyncManager::instance().unlock_users();
        if (ucopy.is_suspended) {
            send_http_response(client_fd, 403, "Forbidden", "application/json",
                "{\"error\":\"Submission rejected: Account is suspended.\"}");
            return true;
        }

        int prob_id = 1;
        try { if (!pid_str.empty()) prob_id = std::stoi(pid_str); } catch (...) {}

        const Problem* pr = g_contest.get_problem(prob_id);
        if (!pr) {
            send_http_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Problem not found\"}");
            return true;
        }

        uint64_t sub_id = ++g_total_submissions;
        SharedMemoryManager::instance().append_log("Submission #" + std::to_string(sub_id) + " by " + u + " for Problem #" + std::to_string(prob_id));

        // Use hidden test case for judging; fallback to sample if hidden is empty
        std::string judge_in = !pr->hidden_input.empty() ? pr->hidden_input : pr->sample_input;
        std::string judge_exp = !pr->hidden_output.empty() ? pr->hidden_output : pr->sample_output;

        JudgeResult res = JudgeSandbox::evaluate_submission(
            sub_id,
            code,
            judge_in,
            judge_exp,
            pr->time_limit_sec,
            pr->memory_limit_mb
        );

        if (res.verdict == VERDICT_ACCEPTED) {
            bool is_first_solve = g_contest.record_problem_solve_if_new(u, pr->id, pr->points);
            auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            g_leaderboard.record_solution(u, pr->points, now_sec % 10000, is_first_solve);
        }

        // Record in Thread-Safe Submission Manager
        SubmissionRecord record;
        record.id = sub_id;
        record.username = u;
        record.problem_id = pr->id;
        record.problem_title = pr->title;
        record.verdict = verdict_to_string(res.verdict);
        record.execution_time_ms = res.execution_time_ms;
        record.memory_used_kb = res.memory_used_kb;
        record.compiler_output = res.compiler_output;
        record.runtime_output = res.runtime_output;
        record.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        record.code = code;
        SubmissionManager::instance().record_submission(record);

        // Paging and Disk Simulation trace
        std::vector<int> mem_refs = MemorySimulator::generate_submission_references(
            static_cast<int>(code.size() / 10 + 20));
        PagingResult paging = MemorySimulator::simulate_lru(mem_refs, 4);
        g_total_page_faults += paging.page_faults;

        std::vector<int> disk_reqs = {35, 110, 18, 175, 88};
        DiskResult disk_res = DiskSimulator::run_scan(disk_reqs, 50, 200, true);
        g_total_disk_movement += disk_res.total_head_movement;

        std::stringstream ss;
        ss << "{\n"
           << "  \"submission_id\": " << sub_id << ",\n"
           << "  \"username\": \"" << escape_json(u) << "\",\n"
           << "  \"problem_id\": " << pr->id << ",\n"
           << "  \"problem_title\": \"" << escape_json(pr->title) << "\",\n"
           << "  \"verdict\": \"" << escape_json(verdict_to_string(res.verdict)) << "\",\n"
           << "  \"execution_time_ms\": " << res.execution_time_ms << ",\n"
           << "  \"memory_used_kb\": " << res.memory_used_kb << ",\n"
           << "  \"compiler_output\": \"" << escape_json(res.compiler_output) << "\",\n"
           << "  \"runtime_output\": \"" << escape_json(res.runtime_output) << "\"\n"
           << "}";

        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        SharedMemoryManager::instance().append_log("Verdict #" + std::to_string(sub_id) + ": " + verdict_to_string(res.verdict));
        return true;
    }

    // -------------------------------------------------------------------------
    // Route 6: POST /api/run (Test code against sample or custom input)
    // -------------------------------------------------------------------------
    if (method == "POST" && clean_path == "/api/run") {
        std::string u = extract_json_field(body, "username");
        std::string pid_str = extract_json_field(body, "problem_id");
        std::string code = extract_json_field(body, "code");
        std::string custom_in = extract_json_field(body, "custom_input");

        int prob_id = 1;
        try { if (!pid_str.empty()) prob_id = std::stoi(pid_str); } catch (...) {}

        const Problem* pr = g_contest.get_problem(prob_id);
        if (!pr) {
            send_http_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Problem not found\"}");
            return true;
        }

        std::string test_in = custom_in.empty() ? pr->sample_input : custom_in;
        uint64_t run_id = ++g_run_counter;

        JudgeResult res = JudgeSandbox::evaluate_submission(
            run_id,
            code,
            test_in,
            pr->sample_output,
            pr->time_limit_sec,
            pr->memory_limit_mb
        );

        std::stringstream ss;
        ss << "{\n"
           << "  \"verdict\": \"" << escape_json(verdict_to_string(res.verdict)) << "\",\n"
           << "  \"execution_time_ms\": " << res.execution_time_ms << ",\n"
           << "  \"memory_used_kb\": " << res.memory_used_kb << ",\n"
           << "  \"input_used\": \"" << escape_json(test_in) << "\",\n"
           << "  \"expected_output\": \"" << escape_json(pr->sample_output) << "\",\n"
           << "  \"runtime_output\": \"" << escape_json(res.runtime_output) << "\",\n"
           << "  \"compiler_output\": \"" << escape_json(res.compiler_output) << "\"\n"
           << "}";

        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // -------------------------------------------------------------------------
    // Route 7: GET /api/submissions (Submission History with Optional User Filter)
    // -------------------------------------------------------------------------
    if (method == "GET" && (clean_path == "/api/submissions" || clean_path == "/api/submissions/")) {
        std::string filter_user = "";
        size_t u_pos = query_params.find("username=");
        if (u_pos != std::string::npos) {
            size_t u_end = query_params.find('&', u_pos);
            filter_user = query_params.substr(u_pos + 9, (u_end == std::string::npos ? query_params.length() : u_end) - (u_pos + 9));
        }

        std::vector<SubmissionRecord> subs = filter_user.empty() 
            ? SubmissionManager::instance().get_all_submissions()
            : SubmissionManager::instance().get_user_submissions(filter_user);

        std::stringstream ss;
        ss << "[\n";
        bool first = true;
        for (int i = static_cast<int>(subs.size()) - 1; i >= 0; --i) {
            const auto& s = subs[i];
            if (!first) ss << ",\n";
            first = false;
            ss << "  {\n"
               << "    \"id\": " << s.id << ",\n"
               << "    \"username\": \"" << escape_json(s.username) << "\",\n"
               << "    \"problem_id\": " << s.problem_id << ",\n"
               << "    \"problem_title\": \"" << escape_json(s.problem_title) << "\",\n"
               << "    \"verdict\": \"" << escape_json(s.verdict) << "\",\n"
               << "    \"execution_time_ms\": " << s.execution_time_ms << ",\n"
               << "    \"memory_used_kb\": " << s.memory_used_kb << ",\n"
               << "    \"compiler_output\": \"" << escape_json(s.compiler_output) << "\",\n"
               << "    \"runtime_output\": \"" << escape_json(s.runtime_output) << "\",\n"
               << "    \"timestamp\": " << s.timestamp << ",\n"
               << "    \"code\": \"" << escape_json(s.code) << "\"\n"
               << "  }";
        }
        ss << "\n]";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // -------------------------------------------------------------------------
    // Route 8: GET /api/leaderboard
    // -------------------------------------------------------------------------
    if (method == "GET" && clean_path == "/api/leaderboard") {
        std::vector<LeaderboardEntry> entries = g_leaderboard.get_ranked_entries();
        std::stringstream ss;
        ss << "[\n";
        for (size_t i = 0; i < entries.size(); ++i) {
            const auto& e = entries[i];
            ss << "  {\n"
               << "    \"rank\": " << e.rank << ",\n"
               << "    \"username\": \"" << escape_json(e.username) << "\",\n"
               << "    \"score\": " << e.score << ",\n"
               << "    \"solved_count\": " << e.solved_count << ",\n"
               << "    \"penalty_sec\": " << e.total_penalty_sec << "\n"
               << "  }" << (i + 1 < entries.size() ? ",\n" : "\n");
        }
        ss << "]";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // -------------------------------------------------------------------------
    // Route 9: GET /api/metrics (OS Kernel Telemetry)
    // -------------------------------------------------------------------------
    if (method == "GET" && clean_path == "/api/metrics") {
        ServerTelemetry* t = SharedMemoryManager::instance().get_telemetry();
        SyncManager& sm = SyncManager::instance();

        std::stringstream ss;
        ss << "{\n";
        if (t) {
            ss << "  \"server_pid\": " << t->server_pid << ",\n"
               << "  \"active_clients\": " << t->active_clients << ",\n"
               << "  \"active_worker_threads\": " << t->active_worker_threads << ",\n"
               << "  \"total_worker_threads\": 8,\n"
               << "  \"total_submissions\": " << t->total_submissions << ",\n"
               << "  \"judge_semaphore_slots\": " << t->judge_semaphore_val << ",\n"
               << "  \"judge_semaphore_max\": 3,\n"
               << "  \"race_condition_mode\": " << (t->race_condition_mode ? "true" : "false") << ",\n"
               << "  \"mutex_acquisitions\": " << t->mutex_acquisitions << ",\n"
               << "  \"shared_counter_val\": " << t->shared_counter_val << ",\n"
               << "  \"page_faults\": " << t->page_faults << ",\n"
               << "  \"total_disk_movement\": " << t->total_disk_head_movement << ",\n"
               << "  \"logs\": [\n";

            uint32_t count = t->log_count;
            uint32_t head = t->log_head;
            for (uint32_t i = 0; i < count; ++i) {
                uint32_t idx = (head + i) % MAX_LOG_ENTRIES;
                ss << "    {\n"
                   << "      \"timestamp\": " << t->logs[idx].timestamp << ",\n"
                   << "      \"message\": \"" << escape_json(t->logs[idx].message) << "\"\n"
                   << "    }" << (i + 1 < count ? ",\n" : "\n");
            }
            ss << "  ]\n";
        } else {
            ss << "  \"server_pid\": 0,\n"
               << "  \"active_clients\": " << g_connected_clients.load() << ",\n"
               << "  \"active_worker_threads\": 2,\n"
               << "  \"total_worker_threads\": 8,\n"
               << "  \"total_submissions\": " << g_total_submissions.load() << ",\n"
               << "  \"judge_semaphore_slots\": " << sm.get_available_judge_slots() << ",\n"
               << "  \"judge_semaphore_max\": 3,\n"
               << "  \"race_condition_mode\": " << (g_race_condition_mode.load() ? "true" : "false") << ",\n"
               << "  \"mutex_acquisitions\": " << sm.stats().mutex_acquisitions.load() << ",\n"
               << "  \"shared_counter_val\": " << sm.get_demo_counter() << ",\n"
               << "  \"page_faults\": " << g_total_page_faults.load() << ",\n"
               << "  \"total_disk_movement\": " << g_total_disk_movement.load() << ",\n"
               << "  \"logs\": []\n";
        }
        ss << "}";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // -------------------------------------------------------------------------
    // Route 10: POST /api/demo/race-condition (WITH_LOCKS vs WITHOUT_LOCKS Demo)
    // -------------------------------------------------------------------------
    if (method == "POST" && clean_path == "/api/demo/race-condition") {
        std::string mode = extract_json_field(body, "mode"); // "WITH_LOCKS" or "WITHOUT_LOCKS"
        if (mode.empty()) mode = "WITH_LOCKS";
        bool use_locks = (mode != "WITHOUT_LOCKS");

        std::string th_str = extract_json_field(body, "threads");
        int num_threads = 4;
        try { if (!th_str.empty()) num_threads = std::stoi(th_str); } catch (...) {}
        if (num_threads <= 0 || num_threads > 16) num_threads = 4;

        std::string it_str = extract_json_field(body, "iterations");
        int iters_per_thread = 25000;
        try { if (!it_str.empty()) iters_per_thread = std::stoi(it_str); } catch (...) {}
        if (iters_per_thread <= 0 || iters_per_thread > 100000) iters_per_thread = 25000;

        int64_t expected = static_cast<int64_t>(num_threads) * iters_per_thread;

        SyncManager::instance().reset_demo_counter();
        auto start_t = std::chrono::high_resolution_clock::now();

        struct RaceArg {
            bool use_locks;
            int iters;
        };

        std::vector<pthread_t> workers(num_threads);
        for (int i = 0; i < num_threads; ++i) {
            auto* arg = new RaceArg{use_locks, iters_per_thread};
            pthread_create(&workers[i], nullptr, [](void* p) -> void* {
                auto* a = static_cast<RaceArg*>(p);
                if (a->use_locks) {
                    SyncManager::instance().demo_increment_counter_synchronized(a->iters);
                } else {
                    SyncManager::instance().demo_increment_counter_unsynchronized(a->iters);
                }
                delete a;
                return nullptr;
            }, arg);
        }
        for (int i = 0; i < num_threads; ++i) {
            pthread_join(workers[i], nullptr);
        }

        auto end_t = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end_t - start_t).count();
        int64_t actual = SyncManager::instance().get_demo_counter();
        int64_t lost = expected - actual;
        if (lost < 0) lost = 0;

        std::stringstream ss;
        ss << "{\n"
           << "  \"mode\": \"" << (use_locks ? "WITH_LOCKS" : "WITHOUT_LOCKS") << "\",\n"
           << "  \"threads\": " << num_threads << ",\n"
           << "  \"iterations_per_thread\": " << iters_per_thread << ",\n"
           << "  \"expected_counter\": " << expected << ",\n"
           << "  \"actual_counter\": " << actual << ",\n"
           << "  \"lost_updates\": " << lost << ",\n"
           << "  \"race_detected\": " << (lost > 0 ? "true" : "false") << ",\n"
           << "  \"execution_time_ms\": " << elapsed_ms << "\n"
           << "}";
        send_http_response(client_fd, 200, "OK", "application/json", ss.str());
        return true;
    }

    // Default 404 for unknown routes
    send_http_response(client_fd, 404, "Not Found", "application/json", "{\"error\":\"Route not found\"}");
    return true;
}
