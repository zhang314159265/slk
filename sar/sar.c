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

	if (objfile_to_extract) {
		arctx_extract_member(&ctx, objfile_to_extract, outpath);
	} else {
		arctx_dump(&ctx);
	}

	arctx_free(&ctx);
	printf("bye\n");
	return 0;
}
