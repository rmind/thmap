/*
 * Copyright (c) 2017-2018 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Use is subject to license terms, as specified in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <err.h>

#include "thmap.h"
#include "utils.h"

static thmap_t *		map;
static pthread_barrier_t	barrier;
static unsigned			nworkers;

static uint64_t			c_keys[4];
static unsigned			thmap_alloc_count;

static uintptr_t
alloc_test_wrapper(size_t len)
{
	thmap_alloc_count++; // count the allocation
	return (uintptr_t)malloc(len);
}

static void
free_test_wrapper(uintptr_t addr, size_t len)
{
	free((void *)addr); (void)len;
}

static void
prepare_collisions(void)
{
	static const thmap_ops_t thmap_test_ops = {
		.alloc = alloc_test_wrapper,
		.free = free_test_wrapper
	};
	void *val, *keyval = (void *)(uintptr_t)0xdeadbeef;

	/*
	 * Pre-calculated collisions.  Note: the brute-force conditions
	 * on the murmurhash3() values:
	 *
	 *	((h0 >> 26) ^ 8) != ((h1 >> 26) ^ 8) || (h0 & 3) == (h1 & 3)
	 *	((h0 >> 26) ^ 8) != ((h1 >> 26) ^ 8) || (h0 & 3) != (h1 & 3)
	 *	h0 != h3
	 */
	c_keys[0] = 0x8000100000080001;
	c_keys[1] = 0x80001000000800fa;
	c_keys[2] = 0x80001000000800df;
	c_keys[3] = 0x800010012e04d085;

	/*
	 * Validate check root-level collision.
	 */
	map = thmap_create(0, &thmap_test_ops, THMAP_NOCOPY);
	thmap_alloc_count = 0;

	val = thmap_put(map, &c_keys[0], sizeof(uint64_t), keyval);
	assert(val && thmap_alloc_count == 2); // leaf + internode

	val = thmap_put(map, &c_keys[1], sizeof(uint64_t), keyval);
	assert(val && thmap_alloc_count == 3); // just leaf

	thmap_destroy(map);

	/*
	 * Validate check first-level (L0) collision.
	 */
	map = thmap_create(0, &thmap_test_ops, THMAP_NOCOPY);
	(void)thmap_put(map, &c_keys[0], sizeof(uint64_t), keyval);

	thmap_alloc_count = 0;
	val = thmap_put(map, &c_keys[2], sizeof(uint64_t), keyval);
	assert(val && thmap_alloc_count == 2); // leaf + internode
	thmap_destroy(map);

	/*
	 * Validate the full 32-bit collision.
	 */
	map = thmap_create(0, &thmap_test_ops, THMAP_NOCOPY);
	(void)thmap_put(map, &c_keys[0], sizeof(uint64_t), keyval);

	thmap_alloc_count = 0;
	val = thmap_put(map, &c_keys[3], sizeof(uint64_t), keyval);
	assert(val && thmap_alloc_count == 1 + 8); // leaf + 8 levels
	thmap_destroy(map);
}

/*
 * Simple xorshift; random() causes huge lock contention on Linux/glibc,
 * which would "hide" the possible race conditions.
 */
static unsigned long
fast_random(void)
{
	static __thread uint32_t fast_random_seed = 5381;
	uint32_t x = fast_random_seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	fast_random_seed = x;
	return x;
}

static void *
fuzz_collision(void *arg, unsigned range_mask)
{
	const unsigned id = (uintptr_t)arg;
	unsigned n = 1 * 1000 * 1000;

	pthread_barrier_wait(&barrier);
	while (n--) {
		uint64_t key = c_keys[fast_random() & range_mask];
		void *keyval = (void *)(uintptr_t)key;
		void *val;

		switch (fast_random() & 3) {
		case 0:
		case 1: // ~50% lookups
			val = thmap_get(map, &key, sizeof(key));
			assert(!val || val == keyval);
			break;
		case 2:
			val = thmap_put(map, &key, sizeof(key), keyval);
			assert(val == keyval);
			break;
		case 3:
			val = thmap_del(map, &key, sizeof(key));
			assert(!val || val == keyval);
			break;
		}
	}
	pthread_barrier_wait(&barrier);

	/* The primary thread performs the clean-up. */
	if (id == 0) {
		thmap_del(map, &c_keys[0], sizeof(uint64_t));
		thmap_del(map, &c_keys[1], sizeof(uint64_t));
		thmap_del(map, &c_keys[2], sizeof(uint64_t));
		thmap_del(map, &c_keys[3], sizeof(uint64_t));
	}
	pthread_exit(NULL);
	return NULL;
}

static void *
fuzz_root_collision(void *arg)
{
	/* Root-level collision: c_keys[0] vs c_keys[1]. */
	return fuzz_collision(arg, 0x1);
}

static void *
fuzz_l0_collision(void *arg)
{
	/* First-level collision: c_keys[0] vs c_keys[2]. */
	return fuzz_collision(arg, 0x2);
}

static void *
fuzz_multi_collision(void *arg)
{
	/* Root-level collision: c_keys vs c_keys. */
	return fuzz_collision(arg, 0x3);
}

static void *
fuzz_multi(void *arg, uint64_t range_mask)
{
	const unsigned id = (uintptr_t)arg;
	unsigned n = 1 * 1000 * 1000;

	pthread_barrier_wait(&barrier);
	while (n--) {
		uint64_t key = fast_random() & range_mask;
		void *keyval = (void *)(uintptr_t)key;
		void *val;

		switch (fast_random() & 3) {
		case 0:
		case 1: // ~50% lookups
			val = thmap_get(map, &key, sizeof(key));
			assert(!val || val == keyval);
			break;
		case 2:
			val = thmap_put(map, &key, sizeof(key), keyval);
			assert(val == keyval);
			break;
		case 3:
			val = thmap_del(map, &key, sizeof(key));
			assert(!val || val == keyval);
			break;
		}
	}
	pthread_barrier_wait(&barrier);

	if (id == 0) for (uint64_t key = 0; key <= range_mask; key++) {
		thmap_del(map, &key, sizeof(key));
	}
	pthread_exit(NULL);
	return NULL;
}

static void *
fuzz_multi_128(void *arg)
{
	/*
	 * Key range of 128 values to trigger contended
	 * expand/collapse cycles mostly within two levels.
	 */
	return fuzz_multi(arg, 0x1f);
}

static void *
fuzz_multi_512(void *arg)
{
	/*
	 * Key range of 512 ought to create multiple levels.
	 */
	return fuzz_multi(arg, 0x1ff);
}

static void
run_test(void *func(void *))
{
	pthread_t *thr;

	puts(".");
	map = thmap_create(0, NULL, 0);
	nworkers = sysconf(_SC_NPROCESSORS_CONF) + 1;

	thr = malloc(sizeof(pthread_t) * nworkers);
	pthread_barrier_init(&barrier, NULL, nworkers);

	for (unsigned i = 0; i < nworkers; i++) {
		if ((errno = pthread_create(&thr[i], NULL,
		    func, (void *)(uintptr_t)i)) != 0) {
			err(EXIT_FAILURE, "pthread_create");
		}
	}
	for (unsigned i = 0; i < nworkers; i++) {
		pthread_join(thr[i], NULL);
	}
	pthread_barrier_destroy(&barrier);
	thmap_destroy(map);
}

int
main(void)
{
	prepare_collisions();
	run_test(fuzz_root_collision);
	run_test(fuzz_l0_collision);
	run_test(fuzz_multi_collision);
	run_test(fuzz_multi_128);
	run_test(fuzz_multi_512);
	puts("ok");
	return 0;
}
