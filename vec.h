/*
 * Copied from: https://github.com/zhang314159265/sas/blob/master/vec.h
 * TODO: avoid duplicate this file across repos
 */
#pragma once

#include "check.h"
#include "util.h"

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

static inline void vec_append(struct vec* vec, void *itemptr);

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

// spaces preceding items with be ignored. But spaces inside or after items will be
// kept.
static inline struct vec vec_create_from_csv(const char* csv) {
  struct vec list = vec_create(sizeof(char*));

  const char *l = csv, *r;
  while (*l) {
    while (isspace(*l)) {
      ++l;
    }
    if (!*l) {
      break;
    }
    r = l;
    while (*r && *r != ',') {
      ++r;
    }
    if (r - l > 0) {
      char* item = lenstrdup(l, r - l);
      vec_append(&list, &item);
    }
    l = r;
    if (*l == ',') {
      ++l;
    }
  }
  return list;
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
  if (vec->data) {
    free(vec->data);
  }
  vec->data = NULL;
}

static inline void vec_free_str(struct vec* vec) {
  // free every item string first
  VEC_FOREACH(vec, char*, item_ptr) {
    free(*item_ptr);
  }
  vec_free(vec);
}
