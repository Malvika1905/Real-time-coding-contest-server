#ifndef POSIX_COMPAT_H
#define POSIX_COMPAT_H

#if !defined(_WIN32)
// Native Linux / POSIX environment
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sched.h>
#else
// Windows Win32 API emulation layer for POSIX threads and semaphores
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600 // Enable Vista+ APIs: SRWLOCK, CONDITION_VARIABLE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>

// ---------------------------------------------------------------------------
// pthread_t emulation using Win32 HANDLE
// ---------------------------------------------------------------------------
struct Win32ThreadArg {
    void* (*func)(void*);
    void* arg;
};

inline DWORD WINAPI Win32ThreadProc(LPVOID lpParam) {
    auto* ta = static_cast<Win32ThreadArg*>(lpParam);
    ta->func(ta->arg);
    delete ta;
    return 0;
}

typedef HANDLE pthread_t;

inline int pthread_create(pthread_t* thread, const void* /*attr*/, void* (*start_routine)(void*), void* arg) {
    auto* ta = new Win32ThreadArg{start_routine, arg};
    *thread = CreateThread(nullptr, 0, Win32ThreadProc, ta, 0, nullptr);
    return (*thread != nullptr) ? 0 : -1;
}

inline int pthread_join(pthread_t thread, void** /*retval*/) {
    if (thread != nullptr) {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// pthread_mutex_t emulation using Win32 CRITICAL_SECTION
// ---------------------------------------------------------------------------
typedef CRITICAL_SECTION pthread_mutex_t;

inline int pthread_mutex_init(pthread_mutex_t* mutex, const void* /*attr*/) {
    InitializeCriticalSection(mutex);
    return 0;
}

inline int pthread_mutex_lock(pthread_mutex_t* mutex) {
    EnterCriticalSection(mutex);
    return 0;
}

inline int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    LeaveCriticalSection(mutex);
    return 0;
}

inline int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    DeleteCriticalSection(mutex);
    return 0;
}

// ---------------------------------------------------------------------------
// pthread_rwlock_t emulation using Win32 SRWLOCK
// ---------------------------------------------------------------------------
struct pthread_rwlock_t {
    SRWLOCK srw;
    DWORD exclusive_owner;
};

inline int pthread_rwlock_init(pthread_rwlock_t* rwlock, const void* /*attr*/) {
    InitializeSRWLock(&rwlock->srw);
    rwlock->exclusive_owner = 0;
    return 0;
}

inline int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock) {
    AcquireSRWLockShared(&rwlock->srw);
    return 0;
}

inline int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock) {
    AcquireSRWLockExclusive(&rwlock->srw);
    rwlock->exclusive_owner = GetCurrentThreadId();
    return 0;
}

inline int pthread_rwlock_unlock(pthread_rwlock_t* rwlock) {
    if (rwlock->exclusive_owner == GetCurrentThreadId()) {
        rwlock->exclusive_owner = 0;
        ReleaseSRWLockExclusive(&rwlock->srw);
    } else {
        ReleaseSRWLockShared(&rwlock->srw);
    }
    return 0;
}

inline int pthread_rwlock_destroy(pthread_rwlock_t* /*rwlock*/) {
    return 0;
}

// ---------------------------------------------------------------------------
// pthread_cond_t emulation using Win32 CONDITION_VARIABLE
// ---------------------------------------------------------------------------
typedef CONDITION_VARIABLE pthread_cond_t;

inline int pthread_cond_init(pthread_cond_t* cond, const void* /*attr*/) {
    InitializeConditionVariable(cond);
    return 0;
}

inline int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    SleepConditionVariableCS(cond, mutex, INFINITE);
    return 0;
}

inline int pthread_cond_signal(pthread_cond_t* cond) {
    WakeConditionVariable(cond);
    return 0;
}

inline int pthread_cond_broadcast(pthread_cond_t* cond) {
    WakeAllConditionVariable(cond);
    return 0;
}

inline int pthread_cond_destroy(pthread_cond_t* /*cond*/) {
    return 0;
}

// ---------------------------------------------------------------------------
// sem_t counting semaphore emulation using Win32 Semaphore
// ---------------------------------------------------------------------------
typedef HANDLE sem_t;

inline int sem_init(sem_t* sem, int /*pshared*/, unsigned int value) {
    *sem = CreateSemaphore(nullptr, static_cast<LONG>(value), 32, nullptr);
    return (*sem != nullptr) ? 0 : -1;
}

inline int sem_wait(sem_t* sem) {
    DWORD res = WaitForSingleObject(*sem, INFINITE);
    return (res == WAIT_OBJECT_0) ? 0 : -1;
}

inline int sem_post(sem_t* sem) {
    BOOL res = ReleaseSemaphore(*sem, 1, nullptr);
    return res ? 0 : -1;
}

inline int sem_getvalue(sem_t* sem, int* sval) {
    LONG prev_count = 0;
    if (ReleaseSemaphore(*sem, 0, &prev_count)) {
        *sval = static_cast<int>(prev_count);
        return 0;
    }
    *sval = 1;
    return 0;
}

inline int sem_destroy(sem_t* sem) {
    if (*sem != nullptr) {
        CloseHandle(*sem);
        *sem = nullptr;
    }
    return 0;
}

inline void posix_sleep_ms(int ms) {
    Sleep(static_cast<DWORD>(ms));
}

#endif // _WIN32
#endif // POSIX_COMPAT_H
