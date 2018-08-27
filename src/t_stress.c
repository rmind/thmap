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

#define	COLKEY_MAGIC_VAL	0x8000100000080001

static uint64_t			collision_keys[2];

/*
 * WARNING: This must match the definitions in the thmap.c implementation.
 */
#define	ROOT_BITS	(6)
#define	ROOT_SIZE	(1 << ROOT_BITS)
#define	ROOT_MASK	(ROOT_SIZE - 1)

static void
prepare_collisions(void)
{
	unsigned b, c = UINT_MAX;

	collision_keys[0] = COLKEY_MAGIC_VAL;
	collision_keys[1] = collision_keys[0];

	/*
	 * Find two colliding keys (just brute-force as the range is small).
	 */
	b = murmurhash3(&collision_keys[0], sizeof(uint64_t), 0) & ROOT_MASK;
	while (b != c) {
		collision_keys[1]++;
		c = murmurhash3(&collision_keys[1], sizeof(uint64_t), 0) & ROOT_MASK;
	}
	assert(collision_keys[0] != collision_keys[1]);
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
fuzz_collision(void *arg)
{
	const unsigned id = (uintptr_t)arg;
	unsigned n = 1 * 1000 * 1000;

	pthread_barrier_wait(&barrier);
	while (n--) {
		uint64_t key = collision_keys[fast_random() & 0x1];
		void *val;

		switch (fast_random() % 4) {
		case 0:
		case 1:
			val = thmap_get(map, &key, sizeof(key));
			assert(!val || (uintptr_t)val == (uintptr_t)key);
			break;
		case 2:
			thmap_put(map, &key, sizeof(key),
			    (void *)(uintptr_t)key);
			break;
		case 3:
			thmap_del(map, &key, sizeof(key));
			break;
		}
	}
	pthread_barrier_wait(&barrier);

	/* The primary thread performs the clean-up. */
	if (id == 0) {
		thmap_del(map, &collision_keys[0], sizeof(uint64_t));
		thmap_del(map, &collision_keys[1], sizeof(uint64_t));
	}
	pthread_exit(NULL);
	return NULL;
}

static void *
fuzz_multi(void *arg, uint64_t range_mask)
{
	const unsigned id = (uintptr_t)arg;
	unsigned n = 1 * 1000 * 1000;

	pthread_barrier_wait(&barrier);
	while (n--) {
		uint64_t key = fast_random() & range_mask;
		void *val;

		switch (fast_random() % 3) {
		case 0:
			val = thmap_get(map, &key, sizeof(key));
			assert(!val || (uintptr_t)val == (uintptr_t)key);
			break;
		case 1:
			thmap_put(map, &key, sizeof(key),
			    (void *)(uintptr_t)key);
			break;
		case 2:
			thmap_del(map, &key, sizeof(key));
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
	run_test(fuzz_collision);
	run_test(fuzz_multi_128);
	run_test(fuzz_multi_512);
	puts("ok");
	return 0;
}
