#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
	assert(argidx == argc && "Unrecognized arguments found");

	struct arctx ctx = arctx_create(ar_path);

	if (objfile_to_extract) {
		arctx_extract_member(&ctx, objfile_to_extract);
	} else {
		arctx_dump(&ctx);
	}

	printf("bye\n");
	return 0;
}
