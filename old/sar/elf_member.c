#include "elf_member.h"
#include "arctx.h"
#include "scom/elf_reader.h"

// parse the elf file and process the symbol lists
static void elfmem_parse_elf(struct arctx* ctx, struct elf_member* elfmem) {
  char* elfbuf = ctx->buf + elfmem->off;
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

struct elf_member elfmem_create(struct arctx* ctx, const char* name, int off, int size) {
  struct elf_member mem;
  mem.name = name;
  mem.off = off;
  mem.size = size;
  mem.sgidx = -1;

  mem.need_syms = vec_create(sizeof(char*));
  mem.provide_syms = vec_create(sizeof(char*));

  elfmem_parse_elf(ctx, &mem);
  return mem;
}

void elfmem_free(struct elf_member* mem) {
  free((void*) mem->name);
  // vec_free_str(&mem->need_syms);
  // vec_free_str(&mem->provide_syms);
  vec_free(&mem->need_syms);
  vec_free(&mem->provide_syms);
}

// verify the symbols in the symbol lookup table and in elf SYMTAB section match.
void elfmem_verify(struct elf_member* elfmem, struct arctx* ctx) {
  #if 0
  elfmem_dump(elfmem, &ctx->sglist);
  #endif
  struct vec names_in_index;
  if (elfmem->sgidx >= 0) {
    names_in_index = ((struct sym_group*) vec_get_item(&ctx->sglist, elfmem->sgidx))->names;
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
