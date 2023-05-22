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

static uint32_t lkctx_decide_sections_va(struct lkctx* ctx, uint32_t next_va, const char* secname) {
  next_va = make_align(next_va, 4096);
  int idx = 0;
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
    Elf32_Shdr* shdr = elf_reader_get_sh_by_name(rdptr, secname);
    if (shdr) {
      next_va = make_align(next_va, shdr->sh_addralign);
      printf("Elf %d, section %s virtual address 0x%x size %d, algin %d\n", idx, secname, next_va, shdr->sh_size, shdr->sh_addralign);
      next_va += shdr->sh_size;
    }
    idx += 1;
  }
  return next_va;
}

static void lkctx_relocate(struct lkctx* ctx) {
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
    elf_reader_list_sht(rdptr);
  }
  assert(false && "relocate ni");
}

/*
 * I plan to implement linking in 2 passes
 * Pass1:
 * - decide the file_offset and va for each section
 * Pass2:
 * - do relocation
 */
static void lkctx_link(struct lkctx* ctx) {
  assert(ctx->readers.len > 0 && "No input ELF found");
  printf("Got %d ELF objects\n", ctx->readers.len);
  int idx = 0;
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
    printf("Elf %d\n", idx++);
    elf_reader_list_sht(rdptr);
    elf_reader_list_syms(rdptr);
  }

  uint32_t next_va = ENTRY;
  next_va = lkctx_decide_sections_va(ctx, next_va, ".text");
  next_va = lkctx_decide_sections_va(ctx, next_va, ".data");
  next_va = lkctx_decide_sections_va(ctx, next_va, ".bss");
  next_va = lkctx_decide_sections_va(ctx, next_va, ".rodata");

  lkctx_relocate(ctx);

  assert(false && "link ni");
}
