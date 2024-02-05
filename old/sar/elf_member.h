/*
 * Data structure to represent an elf file embedded inside an ar file.
 */
#pragma once

#include "../../sar/sym_group.h"
#include "scom/elf_reader.h"

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

#if 1
struct arctx;
// parse the elf file and process the symbol lists
// TODO: may need update the interface of this function
static void elfmem_parse_elf(char* ctx_buf, struct elf_member* elfmem) {
  char* elfbuf = ctx_buf + elfmem->off;
  int elfsiz = elfmem->size;
  struct elf_reader reader = elfr_create_from_buffer(elfbuf, elfsiz);

  elfmem->provide_syms = elfr_get_global_defined_syms(&reader);
  elfmem->need_syms = elfr_get_undefined_syms(&reader);

  #if 0
  elf_reader_list_syms(&reader);

  printf("global defined names in elf:\n");
  VEC_FOREACH(&elfmem->provide_syms, char *, provide_name_ptr) {
    printf(" - %s\n", *provide_name_ptr);
  }

  printf("Undefined names in elf:\n");
  VEC_FOREACH(&elfmem->need_syms, char *, need_name_ptr) {
    printf(" - %s\n", *need_name_ptr);
  }
  #endif
}


static struct elf_member elfmem_create(char* ctx_buf, const char* name, int off, int size) {
  struct elf_member mem;
  mem.name = name;
  mem.off = off;
  mem.size = size;
  mem.sgidx = -1;

  mem.need_syms = vec_create(sizeof(char*));
  mem.provide_syms = vec_create(sizeof(char*));

  elfmem_parse_elf(ctx_buf, &mem);
  return mem;
}

#endif

void elfmem_free(struct elf_member* mem) {
  free((void*) mem->name);
  // vec_free_str(&mem->need_syms);
  // vec_free_str(&mem->provide_syms);
  vec_free(&mem->need_syms);
  vec_free(&mem->provide_syms);
}

// verify the symbols in the symbol lookup table and in elf SYMTAB section match.
void elfmem_verify(struct elf_member* elfmem, struct vec sglist) {
  #if 0
  elfmem_dump(elfmem, &ctx->sglist);
  #endif
  struct vec names_in_index;
  if (elfmem->sgidx >= 0) {
    names_in_index = ((struct sym_group*) vec_get_item(&sglist, elfmem->sgidx))->names;
  } else {
    printf("\033[33mNo symbol found in the index file for %s\033[0m\n", elfmem->name);
    names_in_index = vec_create(sizeof(char*));
  }

  // verify that the symbols in the sym_group matches the
  // defined global symbols in the elf file.
  assert(names_in_index.len == elfmem->provide_syms.len);
  for (int i = 0; i < names_in_index.len; ++i) {
    char* lhs_name = *(char**) vec_get_item(&names_in_index, i);
    char* rhs_name = *(char**) vec_get_item(&elfmem->provide_syms, i);
    assert(strcmp(lhs_name, rhs_name) == 0);
  }
  #if 0
  printf("Pass verification for %s\n", elfmem->name);
  #endif
}



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
