/*
 * Copied from: https://github.com/zhang314159265/sas/blob/master/dict.h
 * TODO: avoid duplicate this file across repos
 */
#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "util.h"
#include "check.h"

// TODO: key can not be 'const char*' and value can only be int so far.
// generalize if needed

#define DICT_FOREACH(dict_ptr, entry_ptr) \
  for (struct dict_entry* entry_ptr = dict_begin(dict_ptr); \
       entry_ptr != dict_end(dict_ptr); \
       entry_ptr = dict_next(dict_ptr, entry_ptr))

struct dict_entry {
  const char* key;
  int val;
};

struct dict {
  struct dict_entry* entries;
  int size;
  int capacity;
};

static struct dict_entry* _dict_find(struct dict_entry* entries, int capacity, const char* key);
static struct dict_entry* dict_end(struct dict* dict);

static struct dict dict_create() {
  struct dict dict;
  dict.size = 0;
  dict.capacity = 64;
  dict.entries = (struct dict_entry*) calloc(dict.capacity, sizeof(struct dict_entry));
  return dict;
}

static uint32_t hash(const char *s) {
  uint32_t h = 0;
  for (int i = 0; s[i]; ++i) {
    h = h * 23 + s[i];
  }
  return h;
}

static void dict_expand(struct dict* dict) {
  struct dict_entry* oldentries = dict->entries;
  int newcapacity = (dict->capacity << 1);
  struct dict_entry* newentries = (struct dict_entry*) calloc(newcapacity, sizeof(struct dict_entry));

  for (int i = 0; i < dict->capacity; ++i) {
    struct dict_entry* src_entry = oldentries + i;
    if (src_entry->key) {
      struct dict_entry* dst_entry = _dict_find(newentries, newcapacity, src_entry->key);
      assert(dst_entry->key == NULL);
      dst_entry->key = src_entry->key;
      dst_entry->val = src_entry->val;
    }
  }

  dict->capacity = newcapacity;
  dict->entries = newentries;
  free(oldentries);
}

/*
 * If key exists in dict, return the existing entry;
 * otherwise return the entry allocated for the new key.
 */
static struct dict_entry* _dict_find(struct dict_entry* entries, int capacity, const char* key) {
  int h = hash(key) % capacity;
  int cnt = 0;
  while (entries[h].key && strcmp(key, entries[h].key) != 0) {
    if (++cnt == capacity) {
      assert(false && "hash table full");
    }
    ++h;
    if (h == capacity) {
      h = 0;
    }
  }
  return &(entries[h]);
}

/*
 * Put the key/val pair to the provided entry
 */
static void dict_put_to_entry(struct dict* dict, struct dict_entry* entry, const char* key, int val) {
  assert(entry->key == NULL);
  entry->key = strdup(key);
  entry->val = val;
  ++dict->size;

  // expand at the end to avoid invaidate entry ptr
  if ((dict->size << 1) >= dict->capacity) {
    dict_expand(dict);
  }
}

/*
 * Return the number of entries created.
 */
static int dict_put(struct dict* dict, const char* key, int val) {
  if ((dict->size << 1) >= dict->capacity) {
    dict_expand(dict);
  }
  struct dict_entry* entry = _dict_find(dict->entries, dict->capacity, key);
  if (entry->key) { // update
    entry->val = val;
    return 0;
  } else { // insert
    dict_put_to_entry(dict, entry, key, val);
    return 1;
  }
}

static int dict_lookup_nomiss(struct dict* dict, const char *key) {
  struct dict_entry* entry = _dict_find(dict->entries, dict->capacity, key);
  assert(entry->key != NULL);
  return entry->val;
}

/*
 * Always return an non-Null dict_entry pointer. p_dict_entry->key will
 * be NULL if the key is not found.
 */
static struct dict_entry* dict_lookup(struct dict* dict, const char *key) {
  return _dict_find(dict->entries, dict->capacity, key);
}

static void dict_free(struct dict* dict) {
  for (int i = 0; i < dict->capacity; ++i) {
    if (dict->entries[i].key) {
      free((void*) dict->entries[i].key);
    }
  }
  free(dict->entries);
}

static struct dict_entry* _dict_skip_unused(struct dict* dict, struct dict_entry* cur) {
  while (cur != dict_end(dict) && !cur->key) {
    ++cur;
  }
  return cur;
}

/*
 * If the dict is not empty, return the pointer to the first entry, otherwise
 * return end.
 */
static struct dict_entry* dict_begin(struct dict* dict) {
  return _dict_skip_unused(dict, dict->entries);
}

static struct dict_entry* dict_next(struct dict* dict, struct dict_entry* cur) {
  assert(cur != dict_end(dict));
  return _dict_skip_unused(dict, cur + 1);
}

static struct dict_entry* dict_end(struct dict* dict) {
  return dict->entries + dict->capacity;
}
