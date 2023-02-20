#pragma once

#include "dict.h"

struct arctx {
  int file_size;
  FILE* fp;
  char* buf; // to simply the implementation we load the whole file into buf
  struct vec elf_mem_list;

  char *long_name_buf; // point somewhere in buf
  int long_name_buflen;
  struct vec sglist; // a list of symbol groups
};

/*
 * target_syms will be mutated.
 */
static void arctx_resolve_symbols(struct arctx* ctx, struct vec* target_syms) {
  assert(target_syms->len > 0);
  printf("Need resolve the following %d symbols:\n", target_syms->len);
  VEC_FOREACH(target_syms, char*, itemptr) {
    printf(" - %s\n", *itemptr);
  }

  // resolve symbols in stack order
  assert(false && "arctx_resolve_symbols ni");
  assert(target_syms->len == 0); // successfully resolve all symbols
}
