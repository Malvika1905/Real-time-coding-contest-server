# Comprehensive OS Lab Viva Preparation Guide

This guide compiles high-frequency Operating Systems viva and interview questions directly tied to the implementation of our **Real-Time Coding Contest Server**.

---

## Part 1: Processes, Threads, and Concurrency

### Q1: What is the difference between a Process and a Thread? How is each used in your project?
- **Process:** An independent executing program with its own dedicated virtual address space, file descriptor table, and memory segments (text, data, heap, stack). In our project, student code submissions are compiled and executed inside a **separate child process** using `fork()` and `execvp()` in `judge_sandbox.cpp`. This isolates untrusted code so that a runtime error (`SIGSEGV`) or crash cannot kill the server.
- **Thread:** A lightweight unit of execution within a process that shares the same virtual address space, heap, global variables, and file descriptors, but maintains its own stack and registers (`PC`, `SP`). In our server, client requests are handled by a **Pthread worker pool** (`ThreadPool` in `thread_pool.cpp`) to achieve high-throughput concurrency without the overhead of creating separate processes.

### Q2: What actually happens during `fork()` in Linux? What is Copy-on-Write (COW)?
- `fork()` invokes the Linux `clone()` system call. It duplicates the calling process, creating a child process with a new PID.
- **Copy-on-Write (COW):** Instead of duplicating all physical memory pages immediately (which is slow and memory-intensive), the Linux kernel marks the child's page table entries as read-only and points them to the parent's physical pages. Only when either process attempts to write to a page does the CPU generate a page-fault exception, prompting the kernel to allocate a new physical frame and copy the data.

### Q3: What is the difference between `execvp()` and `fork()`?
- `fork()` creates a **new child process** which continues running the same program from the point of the `fork()` call.
- `execvp()` does **not** create a new process. Instead, it replaces the current process's address space (code, data, heap, and stack) with a newly loaded executable binary. In our judge sandbox, the child process created by `fork()` calls `execvp()` to execute `g++` and the student binary.

### Q4: What is a zombie process and how does your project prevent them?
- A **zombie process** occurs when a child process terminates, but its exit status remains in the process table because the parent process has not yet read it via `wait()` or `waitpid()`.
- Our server calls `wait4()` / `waitpid(child_pid, &status, 0)` immediately following the execution of the sandboxed submission, ensuring child exit statuses are harvested and preventing zombie accumulation.

---

## Part 2: Synchronization and Critical Sections

### Q5: What is a Race Condition? How did you demonstrate it in your project?
- A **race condition** occurs when multiple threads concurrently access and modify shared data, and the final outcome depends on the non-deterministic timing and order of thread execution.
- In our project (`SyncManager::demo_increment_counter_unsynchronized` in `sync_manager.cpp`), 4 threads perform 25,000 increments each on a shared counter without locks. Because the assembly instruction `counter++` is actually three separate CPU operations (`MOV eax, [counter]`, `INC eax`, `MOV [counter], eax`), thread preemption between these steps causes **lost updates**, resulting in a final count significantly less than 100,000 (typically ~75,000–85,000). When `pthread_mutex_t` is enabled, all 100,000 increments complete accurately.

### Q6: What are the three requirements for a valid Critical Section solution?
1. **Mutual Exclusion:** If process $P_i$ is executing in its critical section, no other processes can be executing in their critical sections.
2. **Progress:** If no process is executing in its critical section and some wish to enter, only those processes not in their remainder sections can participate in deciding who enters next, and this selection cannot be postponed indefinitely.
3. **Bounded Waiting:** There must be a bound on the number of times other processes are allowed to enter their critical sections after a process has made a request to enter and before that request is granted (prevents starvation).

### Q7: What is the difference between a Mutex and a Counting Semaphore?
- **Mutex (`pthread_mutex_t`):** A locking mechanism with ownership semantics (only the thread that locked the mutex can unlock it). Used for binary mutual exclusion on critical data (e.g., our `Leaderboard` and `UserRegistry`).
- **Counting Semaphore (`sem_t`):** A signaling mechanism that maintains an integer counter representing available resource tokens. Any thread can signal (`sem_post`). In our server, `judge_semaphore_` is initialized to 3 to restrict concurrent sandbox processes, throttling resource utilization to hardware limits.

### Q8: What is a Reader-Writer lock (`pthread_rwlock_t`) and why is it used for the Leaderboard?
- The Reader-Writer Problem recognizes that multiple threads reading shared data concurrently do not cause data corruption.
- `pthread_rwlock_t` allows **concurrent read access** (`pthread_rwlock_rdlock`) to hundreds of clients querying the contest leaderboard simultaneously. Only when a student submits an accepted solution does the server acquire an **exclusive write lock** (`pthread_rwlock_wrlock`), preventing read starvation and maximizing read throughput.

---

## Part 3: Deadlocks

### Q9: What are Coffman's Four Necessary Conditions for Deadlock?
1. **Mutual Exclusion:** At least one resource must be held in a non-shareable mode.
2. **Hold and Wait:** A process must be holding at least one resource and waiting to acquire additional resources currently held by other processes.
3. **No Preemption:** Resources cannot be preempted; a resource can be released only voluntarily by the process holding it after completing its task.
4. **Circular Wait:** A closed chain of processes exists such that each process holds at least one resource needed by the next process in the chain ($P_0 \to P_1 \to \dots \to P_n \to P_0$).

### Q10: How does Dijkstra's Banker's Algorithm ensure Deadlock Avoidance?
- The Banker's Algorithm maintains four matrices/vectors:
  - $Available[m]$: Free instances of each resource type.
  - $Max[n][m]$: Maximum declared resource demand for each process.
  - $Allocation[n][m]$: Resources currently held by each process.
  - $Need[n][m] = Max[n][m] - Allocation[n][m]$.
- When a process requests resources, the system pretends to allocate them and runs the **Safety Algorithm**. It searches for a process whose $Need \le Work$. If a sequence exists where every process can finish and return its resources, the state is **SAFE**, and the request is granted. If no safe sequence exists, the allocation is rolled back, and the process is forced to wait.

---

## Part 4: Virtual Memory and Paging

### Q11: What is a Page Fault? Walk through the step-by-step kernel handling routine.
- A **page fault** is a hardware interrupt (interrupt vector 14 on x86) triggered by the CPU's Memory Management Unit (MMU) when a program accesses a virtual page whose **valid/invalid bit** in the Page Table is set to 0 (page not in physical RAM).
- **Handling Routine:**
  1. Trap to kernel mode; save user registers and state.
  2. Operating system inspects the internal page table to check whether the reference was a valid memory access or an illegal access (which generates `SIGSEGV`).
  3. Find a free physical frame in RAM (from the kernel's free-frame list).
  4. If no free frame exists, execute a **page replacement algorithm** (e.g., FIFO or LRU) to select a victim frame, and write it to swap disk if modified (dirty bit = 1).
  5. Issue a disk I/O request to load the requested page from secondary storage into the allocated physical frame.
  6. When I/O completes, update the Page Table entry with the frame number and set the valid bit to 1.
  7. Restore process registers and restart the exact CPU instruction that caused the page fault.

### Q12: What is Belady's Anomaly? Which algorithm exhibits it and which does not?
- **Belady's Anomaly** is the counter-intuitive phenomenon where increasing the number of physical frames results in an **increase** in the number of page faults for a given reference string.
- **FIFO** exhibits Belady's anomaly because it does not belong to the class of *stack algorithms*.
- **LRU (Least Recently Used)** never exhibits Belady's anomaly because the set of pages in an $N$-frame memory is always a strict subset of the pages in an $(N+1)$-frame memory for any reference sequence.

---

## Part 5: Secondary Storage & Disk Scheduling

### Q13: What are the components of Disk Access Time?
$$\text{Disk Access Time} = \text{Seek Time} + \text{Rotational Latency} + \text{Transfer Time}$$
1. **Seek Time:** The time required for the physical read/write head arm to move to the targeted cylinder/track (dominant factor, typically 3–10 ms).
2. **Rotational Latency:** The time required for the targeted disk sector to rotate under the head (typically $\frac{1}{2 \times \text{RPM}}$).
3. **Transfer Time:** The time taken to read or write the actual data bits.

### Q14: Compare SSTF, SCAN, and C-SCAN disk scheduling.
- **SSTF (Shortest Seek Time First):** Selects the request closest to the current head position. While it minimizes immediate seek time, it can cause **starvation** for requests far from the current head if new close requests continuously arrive.
- **SCAN (Elevator Algorithm):** The disk arm moves in one direction servicing requests until it hits the end cylinder, then reverses direction. This prevents starvation while maintaining good throughput.
- **C-SCAN (Circular SCAN):** Moves in one direction servicing requests until the end, then immediately returns to the beginning of the disk without servicing requests on the return trip. This provides a more **uniform waiting time** across all cylinders.

---

## Part 6: Inter-Process Communication (IPC)

### Q15: Compare UNIX Anonymous Pipes vs. POSIX Shared Memory.
| Feature | Anonymous Pipes (`pipe()`) | POSIX Shared Memory (`shm_open`, `mmap`) |
|---|---|---|
| **Mechanism** | Kernel-buffered unidirectional stream. | Direct mapping of physical memory pages into process address spaces. |
| **Data Copying** | Two copies: user buffer $\to$ kernel buffer $\to$ user buffer. | **Zero copies:** processes read/write directly to shared RAM. |
| **Speed** | Moderate (governed by pipe buffer size and syscalls). | Maximum possible IPC throughput. |
| **Process Relation**| Restricted to related processes (parent-child) via `fork()`. | Any unrelated processes with appropriate permissions and object name. |
| **Project Usage** | Piping test cases and output between server and judge sandbox. | Publishing live telemetry from server to the terminal monitor dashboard. |
