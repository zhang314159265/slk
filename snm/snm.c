/*
 * XXX this tool may look more like readelf rather than snm.
 */
#include <stdio.h>
#include <stdlib.h>
#include "scom/elf_reader.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: snm obj_file\n");
    exit(1);
  }

	struct elf_reader reader = elfr_create(argv[1]);
	elfr_dump_shtab(&reader);
	elfr_dump_symtab(&reader);
	elfr_free(&reader);

	printf("bye\n");
	return 0;
}
