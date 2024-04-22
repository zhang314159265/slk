#include <stdlib.h>

#include "scom/util.h"
#include "scom/vec.h"
#include "scom/check.h"
#include "scom/elf_reader.h"

#include "sar/arctx.h"
#include "lkctx.h"

int main(int argc, char **argv) {
	struct lkctx ctx = lkctx_create();
	for (int i = 0; i < argc; ++i) {
		const char *path = argv[i];
		printf("Handle path %s\n", path);
		if (endswith(path, ".o")) {
			lkctx_add_reader(&ctx, elfr_create(path));
		} else if (endswith(path, ".a")) {
			// TODO: add an API to lkctx to add an archive.
			// Freeing the lkctx should free arctx's for all the added archives.
			struct arctx _ctx = arctx_create(path); // TODO: the memory allocated in the arctx is leaked so far.	

			VEC_FOREACH(&_ctx.elf_mem_list, struct elf_member, mem) {
				lkctx_add_reader(&ctx, elfr_create_from_buffer(_ctx.buf + mem->payload_off, mem->size, false));
			}
		} else {
			FAIL("Can not handle file %s\n", path);
		}
	}
	lkctx_link(&ctx);
	lkctx_free(&ctx);
	printf("Bye\n");
	return 0;
}
