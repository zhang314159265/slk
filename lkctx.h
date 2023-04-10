#pragma once

#include "vec.h"
#include "elf_reader.h"
#include "elf_writer.h"

struct lkctx {
  /*
   * The list of elf_reader's.
   * An archive file has been expanded to individual elf files already.
   */
  struct vec readers;
  struct elf_writer writer;
};

static struct lkctx lkctx_create() {
  struct lkctx ctx;
  ctx.readers = vec_create(sizeof(struct elf_reader));
  ctx.writer = elf_writer_create();
  return ctx;
}

static void lkctx_free(struct lkctx* ctx) {
  vec_free(&ctx->readers);
  elf_writer_free(&ctx->writer);
}

static void lkctx_add_reader(struct lkctx* ctx, struct elf_reader reader) {
  vec_append(&ctx->readers, &reader);
}

static void lkctx_link(struct lkctx* ctx) {
  assert(ctx->readers.len > 0 && "No input ELF found");
  printf("Got %d ELF objects\n", ctx->readers.len);
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
    elf_reader_list_syms(rdptr);
  }
  assert(false && "link ni");
}
