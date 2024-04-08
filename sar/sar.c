#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define DEBUG 0
#if DEBUG
#define dprintf(...) fprintf(stderr, __VA_ARGS__)
#else
// do nothing
#define dprintf(...)
#endif

#include "arctx.h"

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: sar <path.a> [<objfile_to_extract>]\n");
		exit(1);
	}

  int argidx = 1;
  const char* ar_path = argv[argidx++];

  struct vec target_syms = vec_create(sizeof(char*));
  if (argidx + 1 < argc && strcmp(argv[argidx], "-s") == 0) {
    target_syms = vec_create_from_csv(argv[argidx + 1]);
    argidx += 2;
  }

	// -d represents a list of predefined symbols.
	// Typically we can use -d to specify the symbols that should be
	// resolved by dependent libraries. This allow us to resolve
	// symbols for a SINGLE library without consiering the dependencies.
  struct vec predefined_syms = vec_create(sizeof(char*));
  if (argidx + 1 < argc && strcmp(argv[argidx], "-d") == 0) {
    predefined_syms = vec_create_from_csv(argv[argidx + 1]);
    argidx += 2;
  }

	const char* objfile_to_extract = NULL;
	if (argidx < argc) {
		objfile_to_extract = argv[argidx++];
	}

	const char* outpath = NULL;
	// -o outpath
	if (argidx + 1 < argc && strcmp(argv[argidx], "-o") == 0) {
		outpath = argv[argidx + 1];
		argidx += 2;
	}

	assert(argidx == argc && "Unrecognized arguments found");

	struct arctx ctx = arctx_create(ar_path);

	if (target_syms.len > 0) {
		// define linker defined symbols
		char *sym;
		sym = strdup("_GLOBAL_OFFSET_TABLE_"); vec_append(&predefined_syms, &sym);
		sym = strdup("__init_array_start"); vec_append(&predefined_syms, &sym);
		sym = strdup("__init_array_end"); vec_append(&predefined_syms, &sym);
		sym = strdup("__fini_array_start"); vec_append(&predefined_syms, &sym);
		sym = strdup("__fini_array_end"); vec_append(&predefined_syms, &sym);
		sym = strdup("__preinit_array_start"); vec_append(&predefined_syms, &sym);
		sym = strdup("__preinit_array_end"); vec_append(&predefined_syms, &sym);
		sym = strdup("__ehdr_start"); vec_append(&predefined_syms, &sym);
		sym = strdup("_end"); vec_append(&predefined_syms, &sym);
		sym = strdup("_DYNAMIC"); vec_append(&predefined_syms, &sym);
		arctx_resolve_symbols(&ctx, &target_syms, &predefined_syms);
	} else if (objfile_to_extract) {
		arctx_extract_member(&ctx, objfile_to_extract, outpath);
	} else {
		arctx_dump(&ctx);
	}

	arctx_free(&ctx);
  vec_free(&target_syms);
  vec_free(&predefined_syms);
	printf("bye\n");
	return 0;
}
