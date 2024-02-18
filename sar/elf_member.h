/*
 * Data structure to represent an elf file embedded inside an .a file.
 */
#pragma once

#include "scom/util.h"
#include "scom/vec.h"
#include "scom/elf_reader.h"

#include "sym_group.h"

#ifdef dprintf
#undef dprintf
#endif
#define dprintf(...) // fprintf(stderr, __VA_ARGS__)

struct elf_member {
  const char* name; // the name is like xxx.o
	// This is the payload offset. header_off == payload_off - FILE_HEADER_LEN
  int payload_off;
	// This is the payload size.
  int size;

  // index into the sglist in arctx. -1 if not found. These are symbols
  // found in the symbol lookup table inside the archive file.
  int sgidx;

  struct vec need_syms; // str list of symbols needed by this elf file. str itself is not owned by this vec.

  struct vec provide_syms; // str list of symbols provided/defined by this elf file
	struct vec weaks; // if any provided symbol is a weak symbol
};

// verify the symbols in the symbol lookup table and in elf SYMTAB section match.
void elfmem_verify(struct elf_member* elfmem, struct vec sglist) {
  struct vec names_in_index;
  if (elfmem->sgidx >= 0) {
    names_in_index = ((struct sym_group*) vec_get_item(&sglist, elfmem->sgidx))->names;
  } else {
    dprintf("\033[33mNo symbol found in the index file for %s\033[0m\n", elfmem->name);
    names_in_index = vec_create(sizeof(char*));
  }

  // verify that the symbols in the sym_group matches the
  // defined global symbols in the elf file.
	// Currently assume that there is always a reasonable symbol lookup table
	// in the archive.
  assert(names_in_index.len == elfmem->provide_syms.len);
  for (int i = 0; i < names_in_index.len; ++i) {
    char* lhs_name = *(char**) vec_get_item(&names_in_index, i);
    char* rhs_name = *(char**) vec_get_item(&elfmem->provide_syms, i);
    assert(strcmp(lhs_name, rhs_name) == 0);
  }
  dprintf("Pass verification for %s\n", elfmem->name);
}

// parse the elf file and process the symbol lists
static void elfmem_parse_elf(char* ctx_buf, struct elf_member* elfmem) {
  char *elfbuf = ctx_buf + elfmem->payload_off;
  int elfsiz = elfmem->size;

  struct elf_reader reader = elfr_create_from_buffer(elfbuf, elfsiz, false);
	elfr_get_global_defined_syms2(&reader, &elfmem->provide_syms, &elfmem->weaks);
  elfmem->need_syms = elfr_get_undefined_syms(&reader);
	elfr_free(&reader);
}

static struct elf_member elfmem_create(char* ctx_buf, const char* name, int payload_off, int size) {
  struct elf_member mem;
  mem.name = name;
  mem.payload_off = payload_off;
  mem.size = size;
  mem.sgidx = -1;

  mem.need_syms = vec_create(sizeof(char*));
  mem.provide_syms = vec_create(sizeof(char*));
	mem.weaks = vec_create(sizeof(bool));

  elfmem_parse_elf(ctx_buf, &mem);
  return mem;
}

void elfmem_free(struct elf_member* mem) {
  free((void*) mem->name);
  vec_free(&mem->need_syms);
  vec_free(&mem->provide_syms);
	vec_free(&mem->weaks);
}

static void elfmem_dump(struct elf_member* elf, struct vec* sglist) {
  assert(sglist);
  printf("\033[32melfmem %s payload off %d, size %d\033[0m\n", elf->name, elf->payload_off, elf->size);

	#if DUMP_SYM_LOOKUP_TABLE
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
	#endif

	#if DUMP_PROVIDED_SYMS
  printf("The elf file provide the following symbols:\n");
  VEC_FOREACH(&elf->provide_syms, const char*, provide_name_ptr) {
    printf("  %s\n", *provide_name_ptr);
  }
	#endif

	#if DUMP_NEEDED_SYMS
  printf("The elf file needs the following symbols:\n");
  VEC_FOREACH(&elf->need_syms, const char*, need_name_ptr) {
    printf("  %s\n", *need_name_ptr);
  }
	#endif
}
