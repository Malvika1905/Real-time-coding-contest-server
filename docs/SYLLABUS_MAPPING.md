# Operating Systems Syllabus Mapping & Code Cross-Reference

This document connects every theoretical module in the Operating Systems syllabus to its concrete implementation in the **Real-Time Coding Contest Server** codebase.

---

## Module 1: Introduction and Historical Context of OS

### Syllabus Topics
- Multiprogramming and Time-Sharing systems.
- Parallel and Distributed systems.
- Real-time systems and concurrent execution.

### How Our Project Demonstrates It
- **Multiprogramming & Time-Sharing:** The server manages a fixed worker thread pool (`ThreadPool` in [thread_pool.h](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/include/thread_pool.h) / [thread_pool.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/thread_pool.cpp)) that services multiple incoming client connections simultaneously, time-slicing between active evaluation tasks.
- **Real-Time Responsiveness:** The live monitoring dashboard ([monitor_dashboard.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/monitor_dashboard.cpp)) queries POSIX Shared Memory telemetry every 1 second, showing active connections, throughput, and system resource distribution.

---

## Module 2: Operating System Structure and Architecture

### Syllabus Topics
- Monolithic vs. Microkernel concepts.
- Layered system design.
- Client-Server Model.
- System calls, interrupts, and signal handling.

### How Our Project Demonstrates It
- **Client-Server Model:** Implemented over standard TCP/IP Berkeley sockets in [server_main.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/server_main.cpp) and [client_main.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/client_main.cpp).
- **Direct System Calls:** The application invokes native POSIX kernel syscalls:
  - Sockets: `socket()`, `bind()`, `listen()`, `accept()`, `send()`, `recv()`
  - Processes: `fork()`, `execvp()`, `waitpid()`, `wait4()`
  - IPC: `pipe()`, `dup2()`, `shm_open()`, `mmap()`, `ftruncate()`
  - Resource Control: `setrlimit()`
  - Synchronization: `pthread_mutex_lock()`, `pthread_rwlock_rdlock()`, `sem_wait()`, `sem_post()`
- **Signal Handling:**
  - `signal(SIGPIPE, SIG_IGN)` prevents server crash when clients abruptly disconnect.
  - `signal(SIGINT, signal_handler)` performs graceful server teardown.
  - `SIGXCPU` is intercepted by the judge sandbox when submissions exceed their CPU quota.

---

## Module 3: Processes, Threads, Concurrency and Synchronization (Core Focus)

### Syllabus Topics
- Process creation and management (`fork()`, `exec()`, `wait()`).
- POSIX Threads (`pthreads`).
- Critical section problem and race conditions.
- Mutual exclusion (`pthread_mutex_t`), Readers-Writers (`pthread_rwlock_t`), Counting Semaphores (`sem_t`).
- Inter-Process Communication (Pipes & Shared Memory).

### How Our Project Demonstrates It
1. **Process Isolation in Sandboxing:**
   - In [judge_sandbox.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/judge_sandbox.cpp), untrusted student code is never executed inside the server's thread space. Instead, `fork()` creates an isolated child process, `dup2()` redirects standard I/O to pipes, and `execvp()` loads the compiled binary.
2. **Thread Concurrency:**
   - In [thread_pool.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/thread_pool.cpp), worker threads are spawned via `pthread_create()` and coordinate on a producer-consumer task queue using `pthread_cond_wait()` and `pthread_cond_signal()`.
3. **The Race Condition Experiment:**
   - In [sync_manager.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/sync_manager.cpp), an explicit demonstration toggle (`g_race_condition_mode`) allows students to run concurrent increments either with `pthread_mutex_t` (yielding exact 100,000 counts) or without locks (yielding ~78,000 due to non-atomic read-modify-write lost updates).
4. **Counting Semaphores:**
   - `sem_t judge_semaphore_` throttles maximum concurrent compilation processes to 3, demonstrating how semaphores regulate access to finite system resource pools.
5. **IPC (Pipes & POSIX Shared Memory):**
   - Anonymous pipes (`pipe()`) stream test data into child process stdin and extract stdout.
   - POSIX Shared Memory (`shm_open`, `mmap`) in [ipc_shared_memory.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/ipc_shared_memory.cpp) establishes a cross-process communication channel between the server and the live monitoring dashboard.

---

## Module 4: Deadlocks

### Syllabus Topics
- Coffman's four conditions for deadlock (Mutual Exclusion, Hold & Wait, No Preemption, Circular Wait).
- Deadlock Avoidance: Dijkstra's Banker's Algorithm.
- Resource Allocation Graph (RAG).
- Deadlock Detection and Recovery through Preemption.

### How Our Project Demonstrates It
- Implemented in [bankers_algo.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/bankers_algo.cpp).
- Models 4 contest server resources:
  - $R_0$: Compiler License Slots
  - $R_1$: Sandbox RAM Blocks
  - $R_2$: Pipe Descriptors
  - $R_3$: CPU Execution Cores
- Features:
  - Computes the $Need$ matrix ($Need = Max - Allocation$).
  - **Safety Algorithm:** Verifies if a safe sequence $\langle P_0, P_1, \dots \rangle$ exists.
  - **Resource Request Algorithm:** Simulates resource requests and grants them only if the post-allocation state remains safe; rolls back if unsafe.
  - **Deadlock Detection & Recovery:** Detects cyclic dependency and resolves deadlocks by preempting allocated resources from lower-priority submissions.

---

## Module 5: Memory Management

### Syllabus Topics
- Contiguous memory allocation vs. Paging.
- Page faults and Virtual Memory.
- Page replacement policies: FIFO and LRU.
- Locality of reference and Belady’s anomaly.

### How Our Project Demonstrates It
- Implemented in [memory_sim.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/memory_sim.cpp).
- Models a simulated physical memory manager with a configurable frame pool.
- Maps submission code access patterns into page reference strings.
- Implements both **FIFO** and **LRU (Least Recently Used)** page replacement policies.
- Generates step-by-step frame state tables showing page hits, misses, page faults, and fault percentage rates.

---

## Module 6: Secondary Storage Management

### Syllabus Topics
- Disk geometry (cylinders, tracks, sectors).
- Disk scheduling algorithms: FCFS, SSTF, SCAN (Elevator), C-SCAN (Circular SCAN).
- Total head travel distance and seek time optimization.

### How Our Project Demonstrates It
- Implemented in [disk_sim.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/disk_sim.cpp).
- Simulates disk persistence for submission logs and test suites across cylinders (0 to 199).
- Evaluates **FCFS**, **SSTF**, **SCAN**, and **C-SCAN** against identical I/O request queues.
- Computes seek path, total cylinders traversed, and average seek length.

---

## Module 7: Fault Tolerance and Security

### Syllabus Topics
- User authentication and access control.
- Process isolation and runaway resource containment.
- Socket fault handling and robust network protocols.

### How Our Project Demonstrates It
- **Authentication:** Token and credential verification in [contest.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/contest.cpp).
- **Malicious Submission Containment:**
  - `setrlimit(RLIMIT_CPU)` restricts child CPU consumption, catching infinite loops (`while(1)`) via `SIGXCPU` (Time Limit Exceeded).
  - `setrlimit(RLIMIT_AS)` restricts virtual address space, catching memory bombs (`std::vector` allocations) via `SIGSEGV` or allocation failure.
- **Network Resilience:**
  - Robust framing protocol with magic numbers and length checks in [protocol.cpp](file:///c:/Users/Malvika%20Saxena/OneDrive/Desktop/OS_PROJECT/src/protocol.cpp).
  - Handles client drops via `recv() == 0` without blocking other worker threads.

---

## Module 8: Distributed Operating Systems

### Syllabus Topics
- Sockets and RPC fundamentals.
- Concurrency challenges in distributed environments.
- Network communication failures.

### How Our Project Demonstrates It
- Client and server interact via custom binary-header TCP socket packets.
- Multiple remote client CLIs can connect to the server concurrently across LAN/WAN or localhost.

---

## Module 9: Case Studies (Linux vs. Windows)

| Dimension | Linux (POSIX / NPTL) | Windows (Win32) |
|---|---|---|
| **Process Creation** | `fork()` creates an exact COW copy of address space; `execvp()` replaces it. | `CreateProcess()` creates a new process from an executable binary directly. |
| **Threading Model** | POSIX Threads (`pthread_create`), mapped 1:1 to kernel task structs via `clone()`. | Win32 Threads (`CreateThread`), managed by the Windows Executive. |
| **Synchronization** | `pthread_mutex_t`, `pthread_cond_t`, `sem_t`. | `CRITICAL_SECTION` (user-space), Mutex / Semaphore handles (kernel-space). |
| **Inter-Process Comm.** | Anonymous pipes (`pipe()`), POSIX SHM (`shm_open()`, `mmap()`). | Named pipes (`CreateNamedPipe`), File Mapping (`CreateFileMapping`, `MapViewOfFile`). |
| **Resource Limits** | Native POSIX `setrlimit()` (`RLIMIT_CPU`, `RLIMIT_AS`). | Windows Job Objects (`SetInformationJobObject`). |
