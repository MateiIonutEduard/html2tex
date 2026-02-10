#include "html2tex_thread.h"
#include <stdlib.h>

#ifdef _WIN32

int thread_create(thread_t* thread, THREAD_RETURN_TYPE(*func)(void*), void* arg) {
    *thread = CreateThread(NULL, 0, func, arg, 0, NULL);
    return *thread != NULL ? 0 : -1;
}

int thread_join(thread_t thread) {
    return WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0 ? 0 : -1;
}

void thread_exit(thread_return_t ret) {
    ExitThread(ret);
}

int mutex_init(mutex_t* mutex) {
    InitializeCriticalSection(mutex);
    return 0;
}

int mutex_lock(mutex_t* mutex) {
    EnterCriticalSection(mutex);
    return 0;
}

int mutex_unlock(mutex_t* mutex) {
    LeaveCriticalSection(mutex);
    return 0;
}

void mutex_destroy(mutex_t* mutex) {
    DeleteCriticalSection(mutex);
}

int cond_init(cond_t* cond) {
    InitializeConditionVariable(cond);
    return 0;
}

int cond_wait(cond_t* cond, mutex_t* mutex) {
    return SleepConditionVariableCS(cond, mutex, INFINITE) ? 0 : -1;
}

int cond_timedwait(cond_t* cond, mutex_t* mutex, unsigned int timeout_ms) {
    return SleepConditionVariableCS(cond, mutex, timeout_ms) ? 0 : -1;
}

int cond_signal(cond_t* cond) {
    WakeConditionVariable(cond);
    return 0;
}

int cond_broadcast(cond_t* cond) {
    WakeAllConditionVariable(cond);
    return 0;
}

void cond_destroy(cond_t* cond) {
    (void)cond;
}

#else

int thread_create(thread_t* thread, THREAD_RETURN_TYPE(*func)(void*), void* arg) {
    return thrd_create(thread, func, arg);
}

int thread_join(thread_t thread) {
    return thrd_join(thread, NULL);
}

void thread_exit(thread_return_t ret) {
    thrd_exit(ret);
}

int mutex_init(mutex_t* mutex) {
    return mtx_init(mutex, mtx_plain);
}

int mutex_lock(mutex_t* mutex) {
    return mtx_lock(mutex);
}

int mutex_unlock(mutex_t* mutex) {
    return mtx_unlock(mutex);
}

void mutex_destroy(mutex_t* mutex) {
    mtx_destroy(mutex);
}

int cond_init(cond_t* cond) {
    return cnd_init(cond);
}

int cond_wait(cond_t* cond, mutex_t* mutex) {
    return cnd_wait(cond, mutex);
}

int cond_timedwait(cond_t* cond, mutex_t* mutex, unsigned int timeout_ms) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;

    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }

    return cnd_timedwait(cond, mutex, &ts);
}

int cond_signal(cond_t* cond) {
    return cnd_signal(cond);
}

int cond_broadcast(cond_t* cond) {
    return cnd_broadcast(cond);
}

void cond_destroy(cond_t* cond) {
    cnd_destroy(cond);
}

#endif