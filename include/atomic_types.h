#ifndef ATOMIC_TYPES_H
#define ATOMIC_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#ifdef _WIN64
typedef int64_t atomic_bool;
typedef int64_t atomic_size_t;
#else
typedef int32_t atomic_bool;
typedef int32_t atomic_size_t;
#endif
#define ATOMIC_BOOL_INIT 0
#define ATOMIC_SIZE_INIT 0
#else
#include <stdatomic.h>
typedef atomic_bool atomic_bool;
typedef atomic_size_t atomic_size_t;
#define ATOMIC_BOOL_INIT false
#define ATOMIC_SIZE_INIT 0
#endif

#endif