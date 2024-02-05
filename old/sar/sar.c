#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "sym_group.h"
#include "scom/util.h"
#include "scom/vec.h"
#include "elf_member.h"
#include "scom/elf_reader.h"
#include "arctx.h"

#if 1

#include "elf_member.h"
#include "arctx.h"
#include "scom/elf_reader.h"

#endif

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: sar <path> [-v] [<member file>]\n");
    exit(1);
  }
  int argidx = 1;
  const char* ar_path = argv[argidx++];

  struct vec target_syms = vec_create(sizeof(char*));
  if (argidx + 1 < argc && strcmp(argv[argidx], "-s") == 0) {
    target_syms = vec_create_from_csv(argv[argidx + 1]);
    argidx += 2;
  }

  // -d represent a list of predefined symbols
  struct vec predefined_syms = vec_create(sizeof(char*));
  if (argidx + 1 < argc && strcmp(argv[argidx], "-d") == 0) {
    predefined_syms = vec_create_from_csv(argv[argidx + 1]);
    argidx += 2;
  }

  const char* mem_name = NULL;
  if (argidx < argc) {
    mem_name = argv[argidx++];
  }
  struct arctx ctx = arctx_create(ar_path);

  if (target_syms.len > 0) {
    char *sym;
    // defined by the linker. Verify by 'ld --verbose'
    sym = strdup("__init_array_start"); vec_append(&predefined_syms, &sym);
    sym = strdup("__init_array_end"); vec_append(&predefined_syms, &sym);
    sym = strdup("__fini_array_start"); vec_append(&predefined_syms, &sym);
    sym = strdup("__fini_array_end"); vec_append(&predefined_syms, &sym);
    sym = strdup("__preinit_array_start"); vec_append(&predefined_syms, &sym);
    sym = strdup("__preinit_array_end"); vec_append(&predefined_syms, &sym);
    sym = strdup("_end"); vec_append(&predefined_syms, &sym);
    // don't know how these symbols are defined yet
    sym = strdup("_GLOBAL_OFFSET_TABLE_"); vec_append(&predefined_syms, &sym);
    sym = strdup("__ehdr_start"); vec_append(&predefined_syms, &sym);
    sym = strdup("_DYNAMIC"); vec_append(&predefined_syms, &sym);

    // must pass defined_syms into arctx_resolve_symbols since they should be
    // used to cover some recursively dependent symbols.
    arctx_resolve_symbols(&ctx, &target_syms, &predefined_syms);
  } else if (!mem_name) {
    // without a member name, we just dump the ar metadata
    arctx_dump(&ctx);
  } else {
    // with a member name, we extract it to artifact/ directory
    arctx_extract_member(&ctx, mem_name);
  }
  arctx_free(&ctx);
  vec_free(&target_syms);
  vec_free(&predefined_syms);
  printf("bye\n");
  return 0;
}
