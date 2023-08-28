/*
 * TODO: should this tool be a counterpart to readelf rather than nm?
 */
#include <stdio.h>
#include <stdlib.h>
#include "elf_reader.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: snm obj_file\n");
    exit(1);
  }
  struct elf_reader reader = elf_reader_create(argv[1]);
  elf_reader_list_sht(&reader);
  elf_reader_list_syms(&reader);
  printf("bye\n");
  return 0;
}
