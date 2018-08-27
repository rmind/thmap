# Concurrent trie-hash map

[![Build Status](https://travis-ci.org/rmind/thmap.svg?branch=master)](https://travis-ci.org/rmind/thmap)

Concurrent trie-hash map library -- a general purpose hash map, combining
the elements of hashing and radix trie.  Highlights:
- Very competitive performance, with logarithmic time complexity on average.
- Lookups are lock-free and inserts/deletes are using fine-grained locking.
- Incremental growth of the data structure (no large resizing/rehashing).
- Optional support for use in a shared memory, e.g. memory-mapped file.

The implementation is written in C11 and distributed under the 2-clause
BSD license.

NOTE: Delete operations (the key/data destruction) must be synchronised
with the readers using garbage collection.  You can use the Epoch-based
Reclamation (EBR) library provided [HERE](https://github.com/rmind/libqsbr).

References:

- [P. Bagwell, 2001, Ideal Hash Trees, EPFL Technical Report
](http://lampwww.epfl.ch/papers/idealhashtrees.pdf)

- [P. L. Lehman and S. B. Yao.
Efficient locking for concurrent operations on B-trees
ACM TODS, 6(4):650–670, 1981.
](https://www.csd.uoc.gr/~hy460/pdf/p650-lehman.pdf)

- [W. Litwin, 1981, Trie Hashing. Proceedings of the 1981 ACM SIGMOD, p. 19-29
](https://dl.acm.org/citation.cfm?id=582322)

## API

* `thmap_t *thmap_create(uintptr_t baseptr, const thmap_ops_t *ops, unsigned flags)`
  * Construct a new trie-hash map.  The optional `ops` parameter can
  used to set the custom allocate/free operations (see the description
  of `thmap_ops_t` below).  In such case, the `baseptr` is the base (start)
  address of the address space mapping (it must be word-aligned).  If `ops`
  is set to `NULL`, then _malloc(3)_ and _free(3)_ will be used as the
  default operations and `baseptr` should be
  set to zero.  Currently, the supported `flags` are:
    * `THMAP_NOCOPY`: the keys on insert will not be copied and the given
    pointers to them will be expected to be valid and the values constant
    until the key is deleted; by default, the put operation will make a
    copy of the key.

* `void thmap_destroy(thmap_t *hmap)`
  * Destroy the map, freeing the memory it uses.

* `void *thmap_get(thmap_t *hmap, const void *key, size_t len)`
  * Lookup the key (of a given length) and return the value associated with it.
  Return `NULL` if the key is not found (see the caveats section).

* `void *thmap_put(thmap_t *hmap, const void *key, size_t len, void *val)`
  * Insert the key with an arbitrary value.  If the key is already present,
  return the already existing associated value without changing it.
  Otherwise, on a successful insert, return the given value.  Just compare
  the result against `val` to test whether the insert was successful.

* `void *thmap_del(thmap_t *hmap, const void *key, size_t len)`
  * Remove the given key.  If the key was present, return the associated
  value; otherwise return `NULL`.

The `thmap_ops_t` structure has the following members:
* `uintptr_t (*alloc)(size_t len)`
  * Function to allocate the memory.  Must return an address to the
  memory area of the size `len`.  The address must be relative to the
  base address specified during map creation and must be word-aligned.
* `void (*free)(uintptr_t addr, size_t len)`
  * Function to release the memory.  Must take a previously allocated
  address (relative to the base) and release the memory area.  The `len`
  is guaranteed to match the original allocation length.

## Notes

Internally, offsets from the base pointer are used to organise the access
to the data structure.  This allows user to store the data structure in the
shared memory, using the allocation/free functions.  The keys will also be
copied using the custom functions; if `THMAP_NOCOPY` is set, then the keys
must belong to the same shared memory object.

The implementation was extensively tested on a 24-core x86 machine,
see [the stress test](src/t_stress.c) for the details on the technique.

## Caveats

The implementation uses pointer tagging, where two least significant bits
are reserved for the internal use.  This requires the base offset and node
allocations to provide at least 32-bit alignment.

## Example

Simple case backed by _malloc(3)_, which could be used in multi-threaded
environment:
```c
thmap_t *kvmap;
struct obj *obj;

kvmap = thmap_create(0, NULL);
assert(kvmap != NULL);
...
obj = obj_create();
thmap_put(kvmap, "test", sizeof("test") - 1, obj);
...
obj = thmap_get(kvmap, "test", sizeof("test") - 1);
...
thmap_destroy(kvmap);
```

## Packages

Just build the package, install it and link the library using the
`-lthmap` flag.
* RPM (tested on RHEL/CentOS 7): `cd pkg && make rpm`
* DEB (tested on Debian 9): `cd pkg && make deb`
