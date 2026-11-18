# Real-Time Coding Contest Server (OS Lab Project)

A real-time, multi-client coding contest evaluation system engineered to demonstrate core **Operating Systems** concepts in C++ on Linux.

Developed for 3rd-Year B.Tech Computer Science & Engineering (Operating Systems Lab).

---

## Key Highlights & Syllabus Alignment

- **Processes & Threads:** Multithreaded TCP server using POSIX Threads (`pthreads`), Producer-Consumer worker pool, and process-isolated compilation/execution sandboxing using `fork()`, `execvp()`, `dup2()`, and `waitpid()`.
- **Synchronization & Critical Sections:** Mutual exclusion with `pthread_mutex_t`, reader-writer coordination with `pthread_rwlock_t`, and execution throttling with `sem_t` counting semaphores. Includes an explicit toggleable **Race Condition experiment** proving lost updates.
- **Inter-Process Communication (IPC):** Anonymous UNIX `pipe()` descriptors for judge stream redirection and POSIX Shared Memory (`shm_open()`, `mmap()`) for live system telemetry.
- **Deadlock Management:** Dijkstra's **Banker's Algorithm** with Safety checks, Resource Allocation matrices, and Deadlock Detection/Recovery.
- **Virtual Memory & Paging:** Integrated simulator modeling frame allocation, page faults, and comparing **FIFO** vs. **LRU** replacement policies.
- **Secondary Storage:** Disk head scheduling simulator comparing **FCFS**, **SSTF**, **SCAN (Elevator)**, and **C-SCAN** track seek distances.
- **Security & Fault Containment:** `setrlimit()` enforcement preventing infinite loops (`RLIMIT_CPU` / TLE) and memory exhaustion (`RLIMIT_AS` / MLE), with robust TCP packet framing and signal handling (`SIGPIPE`, `SIGINT`).

---

## Directory Structure

```
OS_PROJECT/
├── Makefile                     # Linux POSIX compilation build script
├── README.md                    # Project documentation
├── include/                     # C++ Header Files
│   ├── protocol.h               # Binary framing protocol & status codes
│   ├── contest.h                # Problems, submissions, users data models
│   ├── thread_pool.h            # POSIX Worker Thread Pool (pthreads & condvars)
│   ├── sync_manager.h           # Mutexes, RWLocks, counting semaphores, race harness
│   ├── leaderboard.h            # Synchronized leaderboard with ranking logic
│   ├── judge_sandbox.h          # Process isolation: fork, execvp, pipes, setrlimit
│   ├── ipc_shared_memory.h      # POSIX Shared Memory (/os_contest_telemetry)
│   ├── bankers_algo.h           # Banker's Deadlock Avoidance, Safety, Detection
│   ├── memory_sim.h             # Virtual Memory & Paging (FIFO vs LRU)
│   ├── disk_sim.h               # Secondary Storage Disk Scheduling (FCFS, SSTF, SCAN, C-SCAN)
│   └── monitor_dashboard.h      # Terminal UI / ANSI Live Dashboard
├── src/                         # C++ Implementation Files
│   ├── server_main.cpp          # Contest Server entry point & TCP loop
│   ├── client_main.cpp          # Interactive Client CLI application
│   ├── monitor_main.cpp         # Standalone Live Dashboard process
│   ├── protocol.cpp             # Network packet serialization & framing
│   ├── contest.cpp              # Problem parser and user authentication
│   ├── sync_manager.cpp         # Synchronization & race condition implementation
│   ├── leaderboard.cpp          # Thread-safe scoring & ASCII table formatter
│   ├── thread_pool.cpp          # Pthread worker pool implementation
│   ├── judge_sandbox.cpp        # Sandboxed evaluation via fork, exec, pipes, rlimit
│   ├── ipc_shared_memory.cpp    # POSIX Shared Memory ring buffer & telemetry
│   ├── bankers_algo.cpp         # Banker's algorithm matrix calculation
│   ├── memory_sim.cpp           # Paging frame table simulation & fault tracking
│   ├── disk_sim.cpp             # Disk arm seek calculations
│   └── monitor_dashboard.cpp    # ANSI terminal metrics renderer
├── problems/                    # Contest Problem Suites
│   ├── p1_two_sum/              # Problem 1: Two Sum
│   ├── p2_palindrome/           # Problem 2: Palindrome Check
│   └── p3_fibonacci/            # Problem 3: N-th Fibonacci Number
├── scripts/                     # Demonstration & Testing Shell Scripts
│   ├── run_server.sh            # Build and launch server
│   ├── run_client.sh            # Launch interactive client CLI
│   ├── stress_test_race.sh      # Automated multi-client concurrency stress test
│   └── demo_all.sh              # Master test script running all 11 scenarios
└── docs/                        # Academic Evaluation & Viva Docs
    ├── SYLLABUS_MAPPING.md      # Detailed mapping of all 9 syllabus modules
    ├── TEAM_ALLOCATION.md       # 4-Member breakdown and presentation script
    └── VIVA_PREPARATION.md      # Comprehensive Viva Q&A covering all OS topics
```

---

## Web Frontend & Execution

The primary user interface is a modern, responsive web application built with **React.js, HTML5, CSS3, and JavaScript** communicating directly with the C++ backend over an embedded HTTP REST API (`localhost:8080/api/...`).

### Required User Interface Pages
1. **Home / Contest Page:** Contest status, duration countdown, available problem cards, and participant login/registration.
2. **Code Editor Page:** Problem statement viewer, code submission area with presets (AC, WA, TLE, MLE), live submission evaluation, execution metrics (runtime ms, memory KB), and compiler output.
3. **Live Leaderboard:** Dynamically updated participant rankings, scores, and penalty times synchronized via `pthread_rwlock_t`.
4. **OS Monitoring Dashboard:** Live system dials (Connected clients, Pthread workers, `sem_t` judge slots, `pthread_mutex_t` acquisitions), shared memory telemetry ring buffer, and interactive controls for Concurrency Race experiments, Banker's Algorithm, Paging (FIFO vs LRU), and Disk scheduling (FCFS/SSTF/SCAN/C-SCAN).

---

## Compilation & Execution

### Prerequisites
- Linux OS (Ubuntu 20.04+, Debian, Fedora, Arch, or WSL2) or Windows
- GCC / G++ supporting C++17 (`g++ -std=c++17 -pthread`)
- Node.js (v18+) and npm

### 1. Launch C++ Server & React Web Frontend Together
```bash
chmod +x scripts/run_web.sh
./scripts/run_web.sh
```
Or on Windows:
```cmd
run_web.bat
```
- **Web Application URL:** `http://localhost:5173` (or `http://localhost:3000`)
- **C++ Backend Server:** `http://localhost:8080`

### 2. Standalone C++ Compilation (Makefile)
```bash
make all
./bin/contest_server
```

### 3. Optional Terminal CLI Client & Live Monitor
```bash
./bin/contest_client 127.0.0.1 8080
./bin/monitor

---

## 11 Demonstration Scenarios for Lab Viva

You can run the automated master test suite:
```bash
chmod +x scripts/demo_all.sh
./scripts/demo_all.sh
```

Or demonstrate individual scenarios interactively via the Client CLI:

| # | Demonstration Scenario | Core OS Concept | Where to View / Observe |
|---|---|---|---|
| **1** | Multiple Clients Connecting | TCP Sockets, Non-blocking Accept, Thread Pool | Server console & Monitor Dashboard |
| **2** | Concurrent Code Submissions | Multithreading (`pthreads`), Worker Queue | Monitor Dashboard (Active Worker Threads) |
| **3** | Race Condition (WITH vs. WITHOUT Locks) | Critical Section Problem, Lost Updates | Client Menu Option 5 (generates lost updates) |
| **4** | Synchronized Leaderboard Data | `pthread_rwlock_t` (Readers-Writers Problem) | Client Menu Option 4 |
| **5** | Counting Semaphore Throttling | POSIX Semaphores (`sem_t`) for Judge Slots | Monitor Dashboard (`Judge Slots: 3 / 3`) |
| **6** | Inter-Process Communication | Anonymous Pipes (`pipe()`) & POSIX SHM (`shm_open`) | Server log stream & sandbox stdout |
| **7** | Banker's Algorithm (Deadlock Avoidance) | Dijkstra's Safety Algorithm & Request Check | Client Menu Option 6 |
| **8** | Virtual Memory & Paging | FIFO vs. LRU, Page Faults, Locality of Reference | Client Menu Option 7 |
| **9** | Secondary Storage Disk Scheduling | FCFS, SSTF, SCAN (Elevator), C-SCAN | Client Menu Option 8 |
| **10**| Sandboxed Process Containment | `setrlimit(RLIMIT_CPU)` catching infinite loops (TLE) | Client Menu Option 3 -> Sub-option 3 |
| **11**| Client Disconnection & Fault Recovery | Socket EOF (`recv == 0`), `SIGPIPE` ignore | Close client terminal abruptly; server continues |

---

## Team Allocation (4 Members)

| Member | Module Focus | Primary Source Files |
|---|---|---|
| **Member 1** | Server Core, Concurrency & Thread Pool | `server_main.cpp`, `thread_pool.cpp`, `thread_pool.h` |
| **Member 2** | Network Protocol, IPC & Execution Sandbox | `client_main.cpp`, `protocol.cpp`, `judge_sandbox.cpp` |
| **Member 3** | Synchronization, Shared State & Deadlock Management | `sync_manager.cpp`, `leaderboard.cpp`, `bankers_algo.cpp` |
| **Member 4** | Virtual Memory, Disk Scheduling & OS Dashboard | `memory_sim.cpp`, `disk_sim.cpp`, `ipc_shared_memory.cpp`, `monitor_dashboard.cpp` |

See [docs/TEAM_ALLOCATION.md](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/docs/TEAM_ALLOCATION.md) for full individual presentation scripts and viva defense talking points.
