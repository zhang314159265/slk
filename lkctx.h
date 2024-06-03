#pragma once

#include "scom/vec.h"
#include "scom/dict.h"
#include "scom/elf_reader.h"
#include "scom/elf_writer.h"
#include "sar/arctx.h"

#ifdef dprintf
#undef dprintf
#endif
#define dprintf(...) // fprintf(stderr, __VA_ARGS__)

struct lkctx {
  /*
   * The list of elf_reader's.
   * An archive file has been expanded to individual elf files already.
   */
  struct vec readers;
  struct elf_writer writer;
	struct dict sym_name_to_abs_va;  // only record global symbols for relocation

	struct vec arctx_list;  // track the list of arctx to free in the end
};

static struct lkctx lkctx_create() {
  struct lkctx ctx;
  ctx.readers = vec_create(sizeof(struct elf_reader));
  ctx.writer = elfw_create();
	ctx.sym_name_to_abs_va = dict_create_str_int();
	ctx.arctx_list = vec_create(sizeof(struct arctx));
  return ctx;
}

static void lkctx_free(struct lkctx* ctx) {
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
    elfr_free(rdptr);
  }
  vec_free(&ctx->readers);
  elfw_free(&ctx->writer);
	dict_free(&ctx->sym_name_to_abs_va);

	VEC_FOREACH(&ctx->arctx_list, struct arctx, _ctx) {
		arctx_free(_ctx);
	}

	vec_free(&ctx->arctx_list);
}

static void lkctx_add_reader(struct lkctx* ctx, struct elf_reader reader) {
  vec_append(&ctx->readers, &reader);
}

static uint32_t lkctx_decide_sections_abs_va(struct lkctx* ctx, uint32_t next_va, const char* secname) {
  next_va = make_align(next_va, 4096);
  int idx = 0;
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
    Elf32_Shdr* shdr = elfr_get_shdr_by_name(rdptr, secname);
    if (shdr) {
      next_va = make_align(next_va, shdr->sh_addralign);
      dprintf("Elf %d, section %s virtual address 0x%x size %d, algin %d\n", idx, secname, next_va, shdr->sh_size, shdr->sh_addralign);

			// record the absolution address for the section
			int status = dict_put(&rdptr->section_name_to_abs_addr, (void*) strdup(secname), (void*) next_va);
			assert(status == 1);

      next_va += shdr->sh_size;
    }
    idx += 1;
  }
  return next_va;
}

static void lkctx_decide_symbol_abs_addr_elf(struct lkctx* ctx, struct elf_reader* rdptr) {
	for (int i = 0; i < rdptr->symtab_size; ++i) {
		Elf32_Sym* sym = rdptr->symtab + i;
    int type = ELF32_ST_TYPE(sym->st_info);
    int bind = ELF32_ST_BIND(sym->st_info);
		if (bind == STB_GLOBAL && sym->st_shndx != SHN_UNDEF) {
			char* sym_name = rdptr->symstr + sym->st_name;
			Elf32_Shdr* sym_section = elfr_get_shdr(rdptr, sym->st_shndx);
			char* sym_section_name = rdptr->shstrtab + sym_section->sh_name;
			uint32_t section_abs_addr = (uint32_t) dict_find_nomiss(&rdptr->section_name_to_abs_addr, (void*) sym_section_name);
			uint32_t sym_abs_addr = section_abs_addr + sym->st_value;
			int status = dict_put(&ctx->sym_name_to_abs_va, (void*) strdup(sym_name), (void*) sym_abs_addr);
			assert(status == 1 && "duplicate definition of symbol found");

			dprintf("sym_name %s, sym_section_name %s, sym_abs_addr 0x%x\n", sym_name, sym_section_name, sym_abs_addr);
		}
	}
}

static void lkctx_decide_symbol_abs_addr(struct lkctx* ctx) {
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
		lkctx_decide_symbol_abs_addr_elf(ctx, rdptr);
	}
}

static void lkctx_relocate_one_entry(struct lkctx* ctx, struct elf_reader* rdptr, Elf32_Sym* sym, int rel_type, uint32_t r_offset, Elf32_Shdr* shdr_text) {
	uint32_t sym_abs_addr;
	uint32_t pc_abs_addr; // for relative relocation
	int sym_type = ELF32_ST_TYPE(sym->st_info);
	if (sym_type == STT_SECTION) {
		Elf32_Shdr* sec = elfr_get_shdr(rdptr, sym->st_shndx);
		char* sec_name = rdptr->shstrtab + sec->sh_name;
		if (!dict_find(&rdptr->section_name_to_abs_addr, (void *) sec_name)) {
			FAIL("Section name not found in section_name_to_abs_addr: %s", sec_name);
		}
		sym_abs_addr = (uint32_t) dict_find_nomiss(&rdptr->section_name_to_abs_addr, (void*) sec_name);
	} else if (sym_type == STT_FUNC || sym_type == STT_NOTYPE || sym_type == STT_OBJECT) {
		char* sym_name = rdptr->symstr + sym->st_name;
		if (!dict_find(&ctx->sym_name_to_abs_va, (void *) sym_name)) {
			FAIL("Symbol name not found in sym_name_to_abs_va: %s", sym_name);
		}
		sym_abs_addr = (uint32_t) dict_find_nomiss(&ctx->sym_name_to_abs_va, (void*) sym_name);
	} else {
		printf("unhandled sym_type %d\n", sym_type);
		assert(false && "unhandled symbol type");
	}

	uint32_t* patch_loc = (uint32_t*) (rdptr->buf + shdr_text->sh_offset + r_offset);
	if (rel_type == R_386_32) {
		*patch_loc = *patch_loc + sym_abs_addr;
	} else if (rel_type == R_386_PC32) {
		// We use the PC before the increment for the relocation computation.
		// For x86 relative jump, the instruction use the PC after increment
		// as the base for addition. The discrepancy is resolved by storing
		// -4 in 'patch_loc'.
		char* text_sec_name = rdptr->shstrtab + shdr_text->sh_name;
		if (!dict_find(&rdptr->section_name_to_abs_addr, (void*) text_sec_name)) {
			FAIL("Section name not found in section_name_to_abs_addr %s", text_sec_name);
		}
		uint32_t text_sec_abs_addr = (uint32_t) dict_find_nomiss(&rdptr->section_name_to_abs_addr, (void*) text_sec_name);
		pc_abs_addr = text_sec_abs_addr + r_offset;
		*patch_loc = *patch_loc + sym_abs_addr - pc_abs_addr;
	} else {
		assert(false && "unsupported relocation type");
	}
}

/*
 * Relocate a single SHT_REL section.
 */
static void lkctx_relocate_section(struct lkctx* ctx, struct elf_reader* rdptr, Elf32_Shdr* shdr_rel) {
	assert(shdr_rel->sh_type == SHT_REL);
	char* name = rdptr->shstrtab + shdr_rel->sh_name;
	dprintf("Relocate section with name %s\n", name);
	assert(shdr_rel->sh_entsize == sizeof(Elf32_Rel));
	assert(shdr_rel->sh_size >= 0 && shdr_rel->sh_size % sizeof(Elf32_Rel) == 0);
	Elf32_Shdr* shdr_symtab = elfr_get_shdr(rdptr, shdr_rel->sh_link);
	// call it text but it may be a data section as well
	Elf32_Shdr* shdr_text = elfr_get_shdr(rdptr, shdr_rel->sh_info);

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
		#if DEBUG
		dprintf("- r_offset 0x%x, sym_idx %d, rel_type %d\n", r_offset, sym_idx, rel_type);
		elfr_dump_symbol(rdptr, cur_sym);
		#endif
		lkctx_relocate_one_entry(ctx, rdptr, cur_sym, rel_type, r_offset, shdr_text);
	}
}

/*
 * Relocate an elf file.
 */
static void lkctx_relocate_elf(struct lkctx* ctx, struct elf_reader* rdptr) {
	for (int i = 0; i < rdptr->shtab_size; ++i) {
    Elf32_Shdr* shdr = elfr_get_shdr(rdptr, i);

		// don't support SHT_RELA yet.
		if (shdr->sh_type == SHT_REL) {
			lkctx_relocate_section(ctx, rdptr, shdr);
		}
	}
}

static void lkctx_relocate(struct lkctx* ctx) {
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
		lkctx_relocate_elf(ctx, rdptr);
  }
}

// Create the header and buffer for an ELF segment.
void lkctx_create_elf_segment(struct lkctx* ctx, struct elf_writer* writer, const char* secname) {
  // segbuf.len may be smaller than seglen for .bss section.
  struct str segbuf = str_create(8);
  int seglen = 0;
  bool isbss = (strcmp(secname, ".bss") == 0);

  // merge the seciton with name `secname` from each reader into a final buffer
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
    Elf32_Shdr *shdr = elfr_get_shdr_by_name(rdptr, secname);
    if (!shdr) {
      continue;
    }

    // use seglen rather than segbuf.len to compute padding.
    int padded_len = make_align(seglen, shdr->sh_addralign) - seglen;

    // add padding
    seglen += padded_len;
    if (!isbss) {
      str_nappend(&segbuf, padded_len, '\0');
    }
    // add section content
    int file_off = shdr->sh_offset;
    int size = shdr->sh_size;
    char* cptr = rdptr->buf + file_off;
    seglen += size;
    if (!isbss) {
      str_lenconcat(&segbuf, cptr, size);
    }
  }

	elfw_create_segment(writer, secname, &segbuf, seglen);
}

static void lkctx_write_elf(struct lkctx* ctx, struct elf_writer* writer, const char* path) {
  // set the entry correctly
  writer->ehdr.e_entry = (uint32_t) dict_find_nomiss(&ctx->sym_name_to_abs_va, "_start");

  // TODO: .data and .bss can be merged
  lkctx_create_elf_segment(ctx, writer, ".text");
  lkctx_create_elf_segment(ctx, writer, ".data");
  lkctx_create_elf_segment(ctx, writer, ".bss");
  lkctx_create_elf_segment(ctx, writer, ".rodata");

  assert(writer->phdrtab.len == 4);
  assert(writer->pbuftab.len == 4);

	elfw_write(writer, path);
}

static void lkctx_link(struct lkctx* ctx) {
  assert(ctx->readers.len > 0 && "No input ELF found");
	#if DEBUG
  printf("Got %d ELF objects\n", ctx->readers.len);
  int idx = 0;
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
    printf("Elf %d\n", idx++);
    elfr_dump_shtab(rdptr);
    elfr_dump_symtab(rdptr);
  }
	#endif

  uint32_t next_va = EXECUTABLE_START_VA;
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".text");
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".data");
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".bss");
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".rodata");
	// to support objective file compiled with -g option
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".debug_abbrev");
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".debug_str");
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".debug_info");
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".debug_line_str");
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".debug_line");

  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".eh_frame");

	// decide te absolute address of each global symbols
	lkctx_decide_symbol_abs_addr(ctx);

	// do relocation using the absolute address of section and global symbols
  lkctx_relocate(ctx);

	lkctx_write_elf(ctx, &ctx->writer, "a.out");
}
