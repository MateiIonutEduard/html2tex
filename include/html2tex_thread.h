#ifndef HTML2TEX_THREAD_H
#define HTML2TEX_THREAD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <synchapi.h>

typedef HANDLE thread_t;
typedef DWORD thread_return_t;
#ifdef _WIN64
#define THREAD_RETURN_TYPE DWORD
#else
#define THREAD_RETURN_TYPE DWORD WINAPI
#endif
#define THREAD_FUNC WINAPI

typedef CRITICAL_SECTION mutex_t;
typedef CONDITION_VARIABLE cond_t;

#include <intrin.h>
#define atomic_load(ptr) InterlockedOr((LONG volatile*)(ptr), 0)
#define atomic_store(ptr, val) InterlockedExchange((LONG volatile*)(ptr), (LONG)(val))
#define atomic_fetch_add(ptr, val) InterlockedExchangeAdd((LONG volatile*)(ptr), (LONG)(val))
#define atomic_fetch_sub(ptr, val) InterlockedExchangeAdd((LONG volatile*)(ptr), -(LONG)(val))

static inline int get_cpu_count(void) {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

#else
#include <unistd.h>
#include <stdatomic.h>
#include <threads.h>

typedef thrd_t thread_t;
typedef int thread_return_t;
#define THREAD_RETURN_TYPE int
#define THREAD_FUNC

typedef mtx_t mutex_t;
typedef cnd_t cond_t;

#define atomic_load(ptr) atomic_load_explicit(ptr, memory_order_acquire)
#define atomic_store(ptr, val) atomic_store_explicit(ptr, val, memory_order_release)
#define atomic_fetch_add(ptr, val) atomic_fetch_add_explicit(ptr, val, memory_order_acq_rel)
#define atomic_fetch_sub(ptr, val) atomic_fetch_sub_explicit(ptr, val, memory_order_acq_rel)

static inline int get_cpu_count(void) {
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    return nprocs > 0 ? (int)nprocs : 4;
}
#endif

int thread_create(thread_t* thread,
    THREAD_RETURN_TYPE(THREAD_FUNC* func)(void*), void* arg);
int thread_join(thread_t thread);
void thread_exit(thread_return_t ret);

int mutex_init(mutex_t* mutex);
int mutex_lock(mutex_t* mutex);

int mutex_unlock(mutex_t* mutex);
void mutex_destroy(mutex_t* mutex);

int cond_init(cond_t* cond);
int cond_wait(cond_t* cond, mutex_t* mutex);

int cond_signal(cond_t* cond);
int cond_broadcast(cond_t* cond);

void cond_destroy(cond_t* cond);
int cond_timedwait(cond_t* cond, mutex_t* mutex, unsigned int timeout_ms);

#endif