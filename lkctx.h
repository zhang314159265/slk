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

/*
 * Relocate a single SHT_REL section.
 */
static void lkctx_relocate_section(struct lkctx* ctx, struct elf_reader* rdptr, Elf32_Shdr* shdr_rel) {
	assert(shdr_rel->sh_type == SHT_REL);
	char* name = rdptr->shstrtab + shdr_rel->sh_name;
	printf("Handle relocation section with name %s\n", name);
	assert(shdr_rel->sh_entsize == sizeof(Elf32_Rel));
	assert(shdr_rel->sh_size >= 0 && shdr_rel->sh_size % sizeof(Elf32_Rel) == 0);
	Elf32_Shdr* shdr_symtab = elf_reader_get_sh(rdptr, shdr_rel->sh_link);
	// call it text but it may be a data section as well
	Elf32_Shdr* shdr_text = elf_reader_get_sh(rdptr, shdr_rel->sh_info);

	Elf32_Rel* entries = (Elf32_Rel*) (rdptr->buf + shdr_rel->sh_offset);
	int nent = shdr_rel->sh_size / sizeof(Elf32_Rel);

	Elf32_Sym* syms = (Elf32_Sym*) (rdptr->buf + shdr_symtab->sh_offset);
	assert(shdr_symtab->sh_size % sizeof(Elf32_Sym) == 0);
	int nsym = shdr_symtab->sh_size / sizeof(Elf32_Sym);

	for (int i = 0; i < nent; ++i) {
		Elf32_Rel* cur_ent = entries + i;
		Elf32_Addr r_offset = cur_ent->r_offset;
		uint32_t sym_idx = ((cur_ent->r_info >> 8) & 0x00FFFFFF);
		int rel_type = (cur_ent->r_info & 0xFF);
		assert(sym_idx >= 0 && sym_idx < nsym);
		Elf32_Sym* cur_sym = syms + sym_idx;
		printf("- r_offset 0x%x, sym_idx %d, rel_type %d\n", r_offset, sym_idx, rel_type);
		elf_reader_dump_symbol(rdptr, cur_sym);
	}
}

/*
 * Relocate an elf file.
 */
static void lkctx_relocate_elf(struct lkctx* ctx, struct elf_reader* rdptr) {
	for (int i = 0; i < rdptr->nsection; ++i) {
    Elf32_Shdr* shdr = elf_reader_get_sh(rdptr, i);

		// don't support SHT_RELA yet.
		if (shdr->sh_type == SHT_REL) {
			lkctx_relocate_section(ctx, rdptr, shdr);
		}
	}
}

static void lkctx_relocate(struct lkctx* ctx) {
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
    elf_reader_list_sht(rdptr);

		lkctx_relocate_elf(ctx, rdptr);
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
