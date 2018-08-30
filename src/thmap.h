/*
 * Copyright (c) 2018 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Use is subject to license terms, as specified in the LICENSE file.
 */

#ifndef _THMAP_H_
#define _THMAP_H_

__BEGIN_DECLS

struct thmap;
typedef struct thmap thmap_t;

#define	THMAP_NOCOPY	0x01

typedef struct {
	uintptr_t	(*alloc)(size_t);
	void		(*free)(uintptr_t, size_t);
} thmap_ops_t;

thmap_t *	thmap_create(uintptr_t, const thmap_ops_t *, unsigned);
void		thmap_destroy(thmap_t *);

void *		thmap_get(thmap_t *, const void *, size_t);
void *		thmap_put(thmap_t *, const void *, size_t, void *);
void *		thmap_del(thmap_t *, const void *, size_t);

void *		thmap_stage_gc(thmap_t *);
void		thmap_gc(thmap_t *, void *);

__END_DECLS

#endif
