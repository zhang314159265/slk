/*
 * Data structure to represent an elf file embedded inside an ar file.
 */
#pragma once

#include "sym_group.h"

struct elf_member {
  const char* name;
  int off; // off is the payload offset
  int size;

  // index into the sglist in arctx. -1 if not found.
  int sgidx;
};

struct elf_member elfmem_create(const char* name, int off, int size) {
  struct elf_member mem;
  mem.name = name;
  mem.off = off;
  mem.size = size;
  mem.sgidx = -1;
  return mem;
}

void elfmem_dump(struct elf_member* elf, struct vec* sglist) {
  assert(sglist);
  printf("= %s off %d, size %d =\n", elf->name, elf->off, elf->size);
  if (elf->sgidx >= 0) {
    assert(elf->sgidx < sglist->len);
    struct sym_group* sg = vec_get_item(sglist, elf->sgidx);
    VEC_FOREACH(&sg->names, const char*, name) {
      printf("  %s\n", *name);
    }
  }
}

void elfmem_free(struct elf_member* mem) {
  free((void*) mem->name);
}
