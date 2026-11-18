#include "monitor_dashboard.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

MonitorDashboard::MonitorDashboard() {}
MonitorDashboard::~MonitorDashboard() {}

std::string MonitorDashboard::format_dashboard_text(const ServerTelemetry* t) {
    if (!t) return "[Dashboard] Telemetry unavailable (server not running?)\n";

    std::stringstream ss;
    ss << "\033[1;36m"; // Cyan
    ss << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    ss << "║          OPERATING SYSTEMS REAL-TIME CONTEST SERVER MONITOR               ║\n";
    ss << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    ss << "\033[0m"; // Reset

    // Process & Thread Info
    ss << "\033[1;33m[PROCESS & THREADING SUBSYSTEM - MODULE 1 & 3]\033[0m\n";
    ss << "  • Server PID:              " << t->server_pid << "\n";
    ss << "  • Connected TCP Clients:   " << t->active_clients << "\n";
    ss << "  • Active Pthread Workers:  " << t->active_worker_threads << " (POSIX libpthread)\n";
    ss << "  • Total Submissions:       " << t->total_submissions << "\n\n";

    // Synchronization & Locks Info
    ss << "\033[1;32m[SYNCHRONIZATION & CRITICAL SECTIONS - MODULE 3]\033[0m\n";
    ss << "  • Mutex Acquisitions:     " << t->mutex_acquisitions << " (pthread_mutex_t)\n";
    ss << "  • Available Judge Slots:   " << t->judge_semaphore_val << " / 3 (sem_t counting semaphore)\n";
    ss << "  • Race Condition Mode:     ";
    if (t->race_condition_mode) {
        ss << "\033[1;31m[ON: LOCKS BYPASS - UNSYNCHRONIZED]\033[0m\n";
    } else {
        ss << "\033[1;32m[OFF: FULL MUTEX PROTECTION]\033[0m\n";
    }
    ss << "  • Shared Demo Counter:     " << t->shared_counter_val << "\n\n";

    // Simulated OS Resource Subsystems
    ss << "\033[1;35m[MEMORY & STORAGE SIMULATION METRICS - MODULE 5 & 6]\033[0m\n";
    ss << "  • Total Paging Faults:     " << t->page_faults << " (Paging Simulation)\n";
    ss << "  • Total Disk Head Traveled:" << t->total_disk_head_movement << " cylinders (Disk Simulation)\n\n";

    // Recent Log Stream from Shared Memory Ring Buffer
    ss << "\033[1;34m[IPC ACTIVITY LOG STREAM - POSIX SHARED MEMORY RING BUFFER]\033[0m\n";
    ss << "-----------------------------------------------------------------------------\n";
    if (t->log_count == 0) {
        ss << "  (No activity logged yet)\n";
    } else {
        uint32_t start = t->log_head;
        uint32_t count = t->log_count;
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t idx = (start + i) % MAX_LOG_ENTRIES;
            ss << "  [" << t->logs[idx].timestamp << "] " << t->logs[idx].message << "\n";
        }
    }
    ss << "-----------------------------------------------------------------------------\n";

    return ss.str();
}

void MonitorDashboard::render_terminal_ui(const ServerTelemetry* t) {
    // Clear screen via ANSI escape code
    std::cout << "\033[2J\033[1;1H";
    std::cout << format_dashboard_text(t);
    std::cout.flush();
}

void MonitorDashboard::run_live_monitor(int refresh_rate_sec) {
    if (!SharedMemoryManager::instance().init_as_client()) {
        std::cerr << "[Monitor Error] Could not connect to Server Shared Memory.\n";
        std::cerr << "Ensure the contest server is running first!\n";
        return;
    }

    std::cout << "Connected to Contest Server Shared Memory telemetry.\n";
    while (true) {
        ServerTelemetry* t = SharedMemoryManager::instance().get_telemetry();
        render_terminal_ui(t);
#ifdef _WIN32
        posix_sleep_ms(refresh_rate_sec * 1000);
#else
        sleep(refresh_rate_sec);
#endif
    }
}
