/*
 * Copyright (c) 2017 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Use is subject to license terms, as specified in the LICENSE file.
 */

#ifndef	_UTILS_H_
#define	_UTILS_H_

#include <stdatomic.h>
#include <stddef.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>

/*
 * A regular assert (debug/diagnostic only).
 */

#if defined(DEBUG)
#define	ASSERT		assert
#else
#define	ASSERT(x)
#endif

/*
 * Branch prediction macros.
 */

#ifndef __predict_true
#define	__predict_true(x)	__builtin_expect((x) != 0, 1)
#define	__predict_false(x)	__builtin_expect((x) != 0, 0)
#endif

/*
 * Minimum, maximum and rounding macros.
 */

#ifndef MIN
#define	MIN(x, y)	((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define	MAX(x, y)	((x) > (y) ? (x) : (y))
#endif

#ifndef roundup2
#define	roundup2(x,m)	((((x) - 1) | ((m) - 1)) + 1)
#endif

/*
 * Atomic operations and memory barriers.  If C11 API is not available,
 * then wrap the GCC builtin routines.
 *
 * Note: This atomic_compare_exchange_weak does not do the C11 thing of
 * filling *(expected) with the actual value, because we don't need
 * that here.
 */
#ifndef atomic_compare_exchange_weak
#define	atomic_compare_exchange_weak(ptr, expected, desired) \
    __sync_bool_compare_and_swap(ptr, *(expected), desired)
#endif
#ifndef atomic_compare_exchange_weak_explicit
#define	atomic_compare_exchange_weak_explicit(ptr, expected, desired, s, f) \
    atomic_compare_exchange_weak(ptr, expected, desired)
#endif

#ifndef atomic_exchange
static inline void *
atomic_exchange(volatile void *ptr, void *newval)
{
	void * volatile *ptrp = (void * volatile *)ptr;
	void *oldval;
again:
	oldval = *ptrp;
	if (!__sync_bool_compare_and_swap(ptrp, oldval, newval)) {
		goto again;
	}
	return oldval;
}
#define atomic_exchange_explicit(ptr, newval, o) atomic_exchange(ptr, newval)
#endif

#ifndef atomic_thread_fence
#define	memory_order_relaxed	__ATOMIC_RELAXED
#define	memory_order_acquire	__ATOMIC_ACQUIRE
#define	memory_order_consume	__ATOMIC_CONSUME
#define	memory_order_release	__ATOMIC_RELEASE
#define	memory_order_seq_cst	__ATOMIC_SEQ_CST
#define	atomic_thread_fence(m)	__atomic_thread_fence(m)
#endif
#ifndef atomic_store_explicit
#define	atomic_store_explicit	__atomic_store_n
#endif
#ifndef atomic_load_explicit
#define	atomic_load_explicit	__atomic_load_n
#endif

/*
 * Conciser convenience wrappers.
 */

#define	atomic_load_relaxed(p)	atomic_load_explicit(p, memory_order_relaxed)
#define	atomic_load_acquire(p)	atomic_load_explicit(p, memory_order_acquire)
#define	atomic_load_consume(p)	atomic_load_explicit(p, memory_order_consume)

#define	atomic_store_release(p,v) \
    atomic_store_explicit(p, v, memory_order_release)
#define	atomic_store_relaxed(p,v) \
    atomic_store_explicit(p, v, memory_order_relaxed)

/*
 * Exponential back-off for the spinning paths.
 */
#define	SPINLOCK_BACKOFF_MIN	4
#define	SPINLOCK_BACKOFF_MAX	128
#if defined(__x86_64__) || defined(__i386__)
#define SPINLOCK_BACKOFF_HOOK	__asm volatile("pause" ::: "memory")
#else
#define SPINLOCK_BACKOFF_HOOK
#endif
#define	SPINLOCK_BACKOFF(count)					\
do {								\
	int __i;						\
	for (__i = (count); __i != 0; __i--) {			\
		SPINLOCK_BACKOFF_HOOK;				\
	}							\
	if ((count) < SPINLOCK_BACKOFF_MAX)			\
		(count) += (count);				\
} while (/* CONSTCOND */ 0);

/*
 * Cache line size - a reasonable upper bound.
 */
#define	CACHE_LINE_SIZE		64

/*
 * Hash functions.
 */
uint32_t	murmurhash3(const void *, size_t, uint32_t);

#endif
