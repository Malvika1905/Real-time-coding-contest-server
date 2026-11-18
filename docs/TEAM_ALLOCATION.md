# Four-Member Team Division & Presentation Plan

This project is partitioned into four modular, balanced engineering roles. Each member has dedicated source files, clear OS theoretical responsibilities, and specific demonstration scripts for the evaluation.

```
+---------------------------------------------------------------------------------+
|                                TEAM ARCHITECTURE                                |
+---------------------------------------------------------------------------------+
|  MEMBER 1: Server Core, Concurrency & Thread Pool                               |
|  MEMBER 2: Networking Protocol, IPC & Execution Sandbox                         |
|  MEMBER 3: Synchronization, Shared State & Deadlock Management                  |
|  MEMBER 4: Virtual Memory, Storage Scheduling & OS Monitoring Dashboard        |
+---------------------------------------------------------------------------------+
```

---

## Member 1: Server Core, Concurrency & Thread Pool

### Primary Responsibilities
- Architect the master TCP network server and connection lifecycle.
- Design and implement the multi-threaded worker pool using POSIX Threads (`libpthread`).
- Implement the task queue with thread synchronization using condition variables (`pthread_cond_t`) and mutexes (`pthread_mutex_t`).
- Handle Linux signals (`SIGINT`, `SIGPIPE`) and graceful connection teardown.

### Assigned Code Files
- [src/server_main.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/server_main.cpp)
- [src/thread_pool.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/thread_pool.cpp)
- [include/thread_pool.h](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/include/thread_pool.h)

### Key Demonstration Talking Points
1. **Thread Pool Architecture:** Explain why spawning an unbounded number of threads per request causes thread exhaustion and excessive context-switching overhead in Linux. Explain how a fixed worker pool implementing a **Producer-Consumer model** with `pthread_cond_wait()` solves this.
2. **Signal Handling:** Explain why `SIGPIPE` must be ignored using `signal(SIGPIPE, SIG_IGN)` when writing to closed sockets, and how `SIGINT` coordinates graceful shutdown across all worker threads.
3. **Viva Questions to Defend:** Difference between user-level and kernel-level threads (NPTL 1:1 model in Linux), `pthread_join()` mechanics, thread stacks vs. heap memory, and socket descriptor table management.

---

## Member 2: Networking Protocol, IPC & Execution Sandbox

### Primary Responsibilities
- Design the binary-header TCP application protocol with magic bytes and packet framing.
- Implement the interactive Client CLI application.
- Build the sandboxed code execution engine using native Linux process isolation: `fork()`, `execvp()`, `dup2()`, and `pipe()`.
- Enforce process resource limits using `setrlimit()` (CPU and Memory quotas).

### Assigned Code Files
- [src/client_main.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/client_main.cpp)
- [src/protocol.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/protocol.cpp)
- [src/judge_sandbox.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/judge_sandbox.cpp)
- [include/protocol.h](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/include/protocol.h)
- [include/judge_sandbox.h](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/include/judge_sandbox.h)

### Key Demonstration Talking Points
1. **Process Isolation vs. Threading:** Explain why untrusted student code must run in a separate process (`fork()`) rather than inside the server process (preventing `SIGSEGV` or segmentation faults from crashing the contest server).
2. **IPC via Pipes:** Walk through how anonymous pipes (`pipe()`) connect parent and child processes, and how `dup2()` redirects `STDIN_FILENO` and `STDOUT_FILENO`.
3. **Sandboxing with `setrlimit()`:** Demonstrate how the server prevents infinite loops (`while(1)`) via `RLIMIT_CPU` and memory exhaustion via `RLIMIT_AS`.
4. **Viva Questions to Defend:** Copy-on-Write (COW) semantics in `fork()`, zombie processes vs. orphan processes, `waitpid()` status macros (`WIFEXITED`, `WEXITSTATUS`, `WTERMSIG`), and TCP packet framing.

---

## Member 3: Synchronization, Shared State & Deadlock Management

### Primary Responsibilities
- Implement thread-safe contest state structures and the dynamically ranked Leaderboard.
- Design and implement the **Race Condition Demonstration Engine** comparing synchronized vs. unsynchronized critical section execution.
- Implement counting semaphores (`sem_t`) to throttle concurrent evaluation slots.
- Implement Dijkstra’s **Banker's Algorithm** for Deadlock Avoidance, Safety Verification, and Deadlock Detection/Recovery.

### Assigned Code Files
- [src/sync_manager.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/sync_manager.cpp)
- [src/leaderboard.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/leaderboard.cpp)
- [src/bankers_algo.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/bankers_algo.cpp)
- [include/sync_manager.h](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/include/sync_manager.h)
- [include/leaderboard.h](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/include/leaderboard.h)
- [include/bankers_algo.h](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/include/bankers_algo.h)

### Key Demonstration Talking Points
1. **Critical Section Problem:** Demonstrate the race condition live using the client menu (Option 5). Show how unsynchronized increments drop ~20% of updates due to interleaved read-modify-write instructions, while `pthread_mutex_t` produces exactly 100,000 updates.
2. **Readers-Writers Problem:** Explain how `pthread_rwlock_t` allows concurrent read access to the leaderboard while ensuring exclusive write access during score updates.
3. **Deadlock Management:** Walk through Banker’s Algorithm matrices (Allocation, Max, Need, Available) and explain how safety checks prevent unsafe states.
4. **Viva Questions to Defend:** Coffman's 4 conditions for deadlock, Mutex vs. Binary Semaphore, Peterson's Algorithm, hardware test-and-set instructions, and Banker's algorithm time complexity $O(m \times n^2)$.

---

## Member 4: Virtual Memory, Storage Scheduling & OS Monitoring Dashboard

### Primary Responsibilities
- Implement the Virtual Memory / Paging Simulator with **FIFO** and **LRU (Least Recently Used)** replacement algorithms.
- Implement the Secondary Storage Disk Scheduling Simulator supporting **FCFS**, **SSTF**, **SCAN (Elevator)**, and **C-SCAN**.
- Implement POSIX Shared Memory (`shm_open`, `mmap`) IPC for publishing real-time telemetry.
- Build the real-time Terminal Monitoring Dashboard ([monitor_dashboard.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/monitor_dashboard.cpp)).

### Assigned Code Files
- [src/memory_sim.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/memory_sim.cpp)
- [src/disk_sim.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/disk_sim.cpp)
- [src/ipc_shared_memory.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/ipc_shared_memory.cpp)
- [src/monitor_dashboard.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/monitor_dashboard.cpp)
- [src/monitor_main.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/monitor_main.cpp)
- [include/memory_sim.h](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/include/memory_sim.h)
- [include/disk_sim.h](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/include/disk_sim.h)
- [include/ipc_shared_memory.h](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/include/ipc_shared_memory.h)
- [include/monitor_dashboard.h](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/include/monitor_dashboard.h)

### Key Demonstration Talking Points
1. **Virtual Memory & Page Replacement:** Demonstrate FIFO vs LRU using Client Menu Option 7. Explain why LRU achieves a lower page fault rate by exploiting the principle of temporal locality, and explain Belady's anomaly in FIFO.
2. **Disk Head Scheduling:** Demonstrate FCFS, SSTF, SCAN, and C-SCAN using Client Menu Option 8. Compare total head movement (cylinders traversed) and explain why SCAN prevents starvation compared to SSTF.
3. **POSIX Shared Memory:** Explain how `shm_open()` and `mmap()` create a memory region accessible across distinct processes without kernel copy overhead, synchronized via a process-shared mutex (`PTHREAD_PROCESS_SHARED`).
4. **Viva Questions to Defend:** Virtual to physical address translation, Page Tables, TLB (Translation Lookaside Buffer), Page fault interrupt lifecycle (`int 14`), and disk rotational latency vs. seek time.
