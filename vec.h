/*
 * Copied from: https://github.com/zhang314159265/sas/blob/master/vec.h
 * TODO: avoid duplicate this file across repos
 */
#pragma once

#include "check.h"

#define VEC_FOREACH_I(vec_ptr, item_type, item_ptr, i) \
  item_type* item_ptr = NULL; \
  for (int i = 0; i < (vec_ptr)->len && (item_ptr = vec_get_item(vec_ptr, i)); ++i)

// TODO: how to handle the case that the body use a variable having the same
// name as the indicator variable?
#define VEC_FOREACH(vec_ptr, item_type, item_ptr) VEC_FOREACH_I(vec_ptr, item_type, item_ptr, __i)

struct vec {
  int itemsize;
  int capacity; // in number of item
  int len; // in number of item
  void *data;
};

static inline struct vec vec_create(int itemsize) {
  struct vec vec;
  vec.itemsize = itemsize;
  vec.capacity = 16; // init capacity
  vec.len = 0;
  vec.data = malloc(vec.capacity * itemsize);
  return vec;
}

/*
 * Don't malloc so we don't need to free anything
 */
static inline struct vec vec_create_nomalloc(int itemsize) {
  struct vec vec;
  vec.itemsize = itemsize;
  vec.capacity = 0;
  vec.len = 0;
  vec.data = NULL;
  return vec;
}

static inline void vec_append(struct vec* vec, void *itemptr) {
  if (vec->len == vec->capacity) {
    vec->capacity <<= 1;
    vec->data = realloc(vec->data, vec->capacity * vec->itemsize);
  }
  memcpy(vec->data + vec->len * vec->itemsize, itemptr, vec->itemsize);
  ++vec->len;
}

static inline void* vec_get_item(struct vec* vec, int idx) {
  CHECK(idx >= 0 && idx < vec->len, "vec index out of range: index %d, size %d", idx, vec->len);
  return vec->data + idx * vec->itemsize;
}

static inline void vec_free(struct vec* vec) {
  assert(vec->data);
  free(vec->data);
  vec->data = NULL;
}
