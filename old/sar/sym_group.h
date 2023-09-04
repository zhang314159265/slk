#pragma once

#include "scom/vec.h"
#include "scom/util.h"

// represent a group of symbols in the ar symbol lookup table that have the
// same header offset.
struct sym_group {
  int header_off;
  struct vec names; // does not own the memory for each name
};

static struct sym_group sym_group_create(int header_off) {
  struct sym_group sg;
  sg.header_off = header_off;
  sg.names = vec_create(sizeof(char*));
  return sg;
}

static void sym_group_free(struct sym_group* sg) {
  vec_free(&sg->names);
}

// add an entry to sym_group list.
// XXX: use a linear search so far. Should use a hash table.
static void sglist_add_entry(struct vec* sglist, int header_off, const char* sname) {
  struct sym_group* sg = NULL;
  VEC_FOREACH(sglist, struct sym_group, iter_sg) {
    if (iter_sg->header_off == header_off) {
      sg = iter_sg;
      break;
    }
  }
  if (!sg) {
    struct sym_group newgroup = sym_group_create(header_off);
    vec_append(sglist, &newgroup);
    sg = vec_get_item(sglist, sglist->len - 1);
  }
  assert(sg);
  assert(sg->header_off == header_off);
  vec_append(&sg->names, &sname);
}
