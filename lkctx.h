#pragma once

#include "scom/vec.h"
#include "scom/dict.h"
#include "scom/elf_reader.h"
#include "scom/elf_writer.h"

struct lkctx {
  /*
   * The list of elf_reader's.
   * An archive file has been expanded to individual elf files already.
   */
  struct vec readers;
  struct elf_writer writer;
	struct dict sym_name_to_abs_va;  // only record global symbols for relocation
};

static struct lkctx lkctx_create() {
  struct lkctx ctx;
  ctx.readers = vec_create(sizeof(struct elf_reader));
  ctx.writer = elfw_create();
	ctx.sym_name_to_abs_va = dict_create_str_int();
  return ctx;
}

static void lkctx_free(struct lkctx* ctx) {
	// TODO: we should free each individual readers
  vec_free(&ctx->readers);
  elfw_free(&ctx->writer);
	dict_free(&ctx->sym_name_to_abs_va);
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
      printf("Elf %d, section %s virtual address 0x%x size %d, algin %d\n", idx, secname, next_va, shdr->sh_size, shdr->sh_addralign);

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

			printf("sym_name %s, sym_section_name %s, sym_abs_addr 0x%x\n", sym_name, sym_section_name, sym_abs_addr);
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
		sym_abs_addr = (uint32_t) dict_find_nomiss(&rdptr->section_name_to_abs_addr, (void*) sec_name);
	} else if (sym_type == STT_FUNC || sym_type == STT_NOTYPE || sym_type == STT_OBJECT) {
		char* sym_name = rdptr->symstr + sym->st_name;
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
	printf("Relocate section with name %s\n", name);
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
		printf("- r_offset 0x%x, sym_idx %d, rel_type %d\n", r_offset, sym_idx, rel_type);
		elfr_dump_symbol(rdptr, cur_sym);
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
  // !!!This alignment is the key to make the generated ELF file work!
  writer->next_file_off = make_align(writer->next_file_off, 4096);
  writer->next_va = make_align(writer->next_va, 4096);

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

  // XXX don't support a combined .text + .bss segment yet.
  assert(segbuf.len == 0 || segbuf.len == seglen);

	Elf32_Phdr phdr = _elfw_create_phdr(writer->next_file_off, writer->next_va, seglen, secname);
	vec_append(&writer->phdrtab, &phdr);
	vec_append(&writer->pbuftab, &segbuf);

  writer->next_file_off = make_align(writer->next_file_off + phdr.p_filesz, ALIGN_BYTES);
  writer->next_va = make_align(writer->next_va + phdr.p_memsz, ALIGN_BYTES);
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

  // place the .shstrtab
  Elf32_Shdr* sh_shstrtab = vec_get_item(&writer->shdrtab, writer->ehdr.e_shstrndx);
  sh_shstrtab->sh_size = writer->shstrtab.len;
  elfw_place_section_to_layout(writer, sh_shstrtab); // will move writer->next_file_off

  // decide the file offset for the program headers and section headers
  writer->next_file_off = make_align(writer->next_file_off, ALIGN_BYTES);
  Elf32_Ehdr* ehdr = &writer->ehdr;
  ehdr->e_phoff = writer->next_file_off;
  ehdr->e_phnum = writer->phdrtab.len;

  writer->next_file_off = make_align(writer->next_file_off + sizeof(Elf32_Phdr) * ehdr->e_phnum, ALIGN_BYTES);
  ehdr->e_shoff = writer->next_file_off;
  ehdr->e_shnum = writer->shdrtab.len;

  // next_file_off could be incremented for the section header table size.
  // But it's fine to skip since nobody read it afterwards

  // The layout of the output ELF file is completely decided. Start writing
  // the content of the file.
  FILE* fp = fopen(path, "wb");
  assert(fp);
  fwrite(ehdr, sizeof(Elf32_Ehdr), 1, fp);

  // write segment content
	for (int i = 0; i < writer->phdrtab.len; ++i) {
		Elf32_Phdr* phdr = vec_get_item(&writer->phdrtab, i);
		struct str* pbuf = vec_get_item(&writer->pbuftab, i);
    fseek(fp, phdr->p_offset, SEEK_SET);
    assert(pbuf->len == phdr->p_filesz);
    fwrite(pbuf->buf, 1, phdr->p_filesz, fp);
	}

  // write .shstrtab
  _elfw_write_section(fp, sh_shstrtab, writer->shstrtab.buf);

  // write segment table
  fseek(fp, ehdr->e_phoff, SEEK_SET);
  VEC_FOREACH(&writer->phdrtab, Elf32_Phdr, phdr) {
    fwrite(phdr, 1, sizeof(Elf32_Phdr), fp);
  }

  // write section table. An ELF without a NULL section will trigger warning
  // by running 'readelf -h'
  VEC_FOREACH(&writer->shdrtab, Elf32_Shdr, shdr) {
    fwrite(shdr, 1, sizeof(Elf32_Shdr), fp);
  }

  fclose(fp);
  printf("Done writing elf file to %s\n", path);
}

static void lkctx_link(struct lkctx* ctx) {
  assert(ctx->readers.len > 0 && "No input ELF found");
  printf("Got %d ELF objects\n", ctx->readers.len);
  int idx = 0;
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
    printf("Elf %d\n", idx++);
    elfr_dump_shtab(rdptr);
    elfr_dump_symtab(rdptr);
  }

  uint32_t next_va = EXECUTABLE_START_VA;
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".text");
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".data");
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".bss");
  next_va = lkctx_decide_sections_abs_va(ctx, next_va, ".rodata");

	// decide te absolute address of each global symbols
	lkctx_decide_symbol_abs_addr(ctx);

	// do relocation using the absolute address of section and global symbols
  lkctx_relocate(ctx);

	lkctx_write_elf(ctx, &ctx->writer, "a.out");
}
