#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "sym_group.h"
#include "util.h"
#include "vec.h"
#include "elf_member.h"
#include "elf_reader.h"
#include "arctx.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: sar <path> [-v] [<member file>]\n");
    exit(1);
  }
  int argidx = 1;
  const char* ar_path = argv[argidx++];

  struct vec target_syms = vec_create_nomalloc(sizeof(char*));
  if (argidx + 1 < argc && strcmp(argv[argidx], "-s") == 0) {
    target_syms = vec_create_from_csv(argv[argidx + 1]);
    argidx += 2;
  }

  const char* mem_name = NULL;
  if (argidx < argc) {
    mem_name = argv[argidx++];
  }
  struct arctx ctx = arctx_create(ar_path);

  if (target_syms.len > 0) {
    arctx_resolve_symbols(&ctx, &target_syms);  
  } else if (!mem_name) {
    // without a member name, we just dump the ar metadata
    arctx_dump(&ctx);
  } else {
    // with a member name, we extract it to artifact/ directory
    arctx_extract_member(&ctx, mem_name);
  }
  arctx_free(&ctx);
  vec_free_str(&target_syms);
  printf("bye\n");
  return 0;
}
