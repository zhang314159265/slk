/*
 * Data structure to represent an elf file embedded inside an ar file.
 */
#pragma once

#include "sym_group.h"

struct elf_member {
  const char* name;
  int off; // off is the payload offset
  int size;

  // index into the sglist in arctx. -1 if not found. These are symbols
  // found in the symbol lookup table inside the ar file.
  int sgidx;

  struct vec need_syms; // str list of symbols needed by this elf file. str itself is not owned by this vec.
  struct vec provide_syms; // str list of symbols provided/defined by this elf file
};

struct arctx;
struct elf_member elfmem_create(struct arctx* ctx, const char* name, int off, int size);
void elfmem_free(struct elf_member* mem);
void elfmem_verify(struct elf_member* elfmem, struct arctx* ctx);

static void elfmem_dump(struct elf_member* elf, struct vec* sglist) {
  assert(sglist);
  printf("Dump elfmem\n");
  printf("= %s off %d, size %d =\n", elf->name, elf->off, elf->size);
  if (elf->sgidx >= 0) {
    assert(elf->sgidx < sglist->len);
    struct sym_group* sg = vec_get_item(sglist, elf->sgidx);
    printf("Symbols in the lookup table:\n");
    VEC_FOREACH(&sg->names, const char*, name) {
      printf("  %s\n", *name);
    }
  } else {
    printf("No symbols found in the lookup table\n");
  }

  printf("The elf file provide the following symbols:\n");
  VEC_FOREACH(&elf->provide_syms, const char*, provide_name_ptr) {
    printf("  %s\n", *provide_name_ptr);
  }
  printf("The elf file needs the following symbols:\n");
  VEC_FOREACH(&elf->need_syms, const char*, need_name_ptr) {
    printf("  %s\n", *need_name_ptr);
  }
}
