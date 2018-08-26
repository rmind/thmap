/*
 * murmurhash3 -- from the original code:
 *
 * "MurmurHash3 was written by Austin Appleby, and is placed in the public
 * domain. The author hereby disclaims copyright to this source code."
 *
 * References:
 *	https://github.com/aappleby/smhasher/
 */

#include <inttypes.h>

#include "utils.h"

uint32_t
murmurhash3(const void *key, size_t len, uint32_t seed)
{
	const uint8_t *data = key;
	const size_t orig_len = len;
	uint32_t h = seed;

	if (__predict_true(((uintptr_t)key & 3) == 0)) {
		while (len >= sizeof(uint32_t)) {
			uint32_t k = *(const uint32_t *)(const void *)data;

			k *= 0xcc9e2d51;
			k = (k << 15) | (k >> 17);
			k *= 0x1b873593;

			h ^= k;
			h = (h << 13) | (h >> 19);
			h = h * 5 + 0xe6546b64;

			data += sizeof(uint32_t);
			len -= sizeof(uint32_t);
		}
	} else {
		while (len >= sizeof(uint32_t)) {
			uint32_t k;

			k  = data[0];
			k |= data[1] << 8;
			k |= data[2] << 16;
			k |= data[3] << 24;

			k *= 0xcc9e2d51;
			k = (k << 15) | (k >> 17);
			k *= 0x1b873593;

			h ^= k;
			h = (h << 13) | (h >> 19);
			h = h * 5 + 0xe6546b64;

			data += sizeof(uint32_t);
			len -= sizeof(uint32_t);
		}
	}

	/*
	 * Handle the last few bytes of the input array.
	 */
	uint32_t k = 0;

	switch (len) {
	case 3:
		k ^= data[2] << 16;
		/* FALLTHROUGH */
	case 2:
		k ^= data[1] << 8;
		/* FALLTHROUGH */
	case 1:
		k ^= data[0];
		k *= 0xcc9e2d51;
		k = (k << 15) | (k >> 17);
		k *= 0x1b873593;
		h ^= k;
	}

	/*
	 * Finalisation mix: force all bits of a hash block to avalanche.
	 */
	h ^= orig_len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}
