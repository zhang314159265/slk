#include <stdio.h>
#include "lkctx.h"
#include "elf_reader.h"
#include "scom/vec.h"
#include "scom/util.h"
#include "scom/check.h"
#include "sar/arctx.h"

int main(int argc, char **argv) {
  struct lkctx ctx = lkctx_create();
  #if 0
  struct vec bufs_to_free = vec_create(sizeof(void*));
  #endif
  for (int i = 1; i < argc; ++i) {
    const char* path = argv[i];
    printf("Handle path %s\n", path);
    if (endswith(path, ".o")) {
      // TODO: memory leak for the buffer
      lkctx_add_reader(&ctx, elf_reader_create(path));
    } else if (endswith(path, ".a")) {
      // TODO: call arctx_free
      struct arctx _ctx = arctx_create(path);

      VEC_FOREACH(&_ctx.elf_mem_list, struct elf_member, mem) {
        lkctx_add_reader(&ctx, elf_reader_create_from_buffer(_ctx.buf + mem->off, mem->size));
      }
    } else {
      FAIL("Can not handle file %s\n", path);
    }
  }
  lkctx_link(&ctx);
  lkctx_free(&ctx);
  #if 0
  VEC_FOREACH(&bufs_to_free, void *, item_ptr) {
    free(*item_ptr);
  }
  vec_free(&bufs_to_free);
  #endif
  printf("Bye\n");
  return 0;
}
