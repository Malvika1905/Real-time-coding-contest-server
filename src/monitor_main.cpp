#include "monitor_dashboard.h"
#include <iostream>

int main(int argc, char* argv[]) {
    int refresh_rate = 1; // 1 second refresh
    if (argc >= 2) {
        try {
            refresh_rate = std::stoi(argv[1]);
        } catch (...) {
            refresh_rate = 1;
        }
    }

    std::cout << "[Monitor] Starting real-time OS metrics inspector (refresh every " << refresh_rate << "s)...\n";
    MonitorDashboard::run_live_monitor(refresh_rate);
    return 0;
}
