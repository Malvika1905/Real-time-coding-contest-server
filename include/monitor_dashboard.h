#ifndef MONITOR_DASHBOARD_H
#define MONITOR_DASHBOARD_H

#include "ipc_shared_memory.h"
#include <string>

class MonitorDashboard {
public:
    MonitorDashboard();
    ~MonitorDashboard();

    // Render snapshot of server state to terminal
    static void render_terminal_ui(const ServerTelemetry* telemetry);

    // Format single-string dashboard for remote queries or CLI
    static std::string format_dashboard_text(const ServerTelemetry* telemetry);

    // Continuous live monitor loop (runs in its own console/process)
    static void run_live_monitor(int refresh_rate_sec);
};

#endif // MONITOR_DASHBOARD_H
