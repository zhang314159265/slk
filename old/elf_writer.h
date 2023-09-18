#pragma once

#if 0

#include <assert.h>
#include "scom/elf.h"
#include "scom/util.h"
#include "scom/str.h"
#include "scom/vec.h"
#include "segment.h"

#define ENTRY 0x50000000
#define ALIGN_BYTES 16

struct elf_writer {
  Elf32_Ehdr ehdr;
  struct str textbuf;
  // struct vec phdrtab; // TODO make use of this. segtab already serves the purpose

  struct str shstrtab; // contains section names
  uint16_t shn_text;

  struct vec shtab; // contains a list of 'Elf32_Shdr'
  struct vec segtab; // contains a list of 'struct segment'

  uint32_t next_file_off;
  uint32_t next_va;
};

static struct elf_writer elf_writer_create() {
  struct elf_writer writer;
  elf_writer_init_ehdr(&writer);
  // writer.phdrtab = vec_create(sizeof(Elf32_Phdr));
  writer.segtab = vec_create(sizeof(struct segment));
  writer.shtab = vec_create(sizeof(Elf32_Shdr));

  writer.next_file_off = sizeof(writer.ehdr);
  writer.next_va = writer.ehdr.e_entry;

  writer.shstrtab = str_create(0);
  str_append(&writer.shstrtab, 0); 

  elf_writer_add_section(&writer, NULL, 0, 0, 0, 0, 0, 0);
  writer.ehdr.e_shstrndx = elf_writer_add_section(&writer, ".shstrtab", SHT_STRTAB, 0, 0, 0, 1, 0);
  writer.shn_text = elf_writer_add_section(&writer, ".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 0, 0, 1, 0);
  return writer;
}

// TODO deprecate this
static void elf_writer_set_text_manually(struct elf_writer* writer, struct str* textstr) {
  writer->textbuf = str_move(textstr);
}

struct lkctx;
void elf_writer_write_with_ctx(struct elf_writer* writer, const char* path, struct lkctx* ctx);

// TODO deprecate this
static void elf_writer_write(struct elf_writer* writer, const char* path) {
  // THIS ALIGNMENT is THE KEY TO make the generated ELF file work!
  writer->next_file_off = make_align(writer->next_file_off, 4096);
  struct segment textseg = segment_create(
    writer->next_file_off,
    writer->next_va,
    &writer->textbuf,
    writer->textbuf.len,
    ".text");

  // Having a .text section header is good but not necessary to make the ELF
  // file work.
  Elf32_Shdr* sh_text = vec_get_item(&writer->shtab, writer->shn_text);
  // set this address is critical to let 'readelf -l' figure out what sections
  // belong to a segment.
  sh_text->sh_addr = textseg.phdr.p_vaddr;
  sh_text->sh_size = textseg.phdr.p_filesz;
  sh_text->sh_offset = writer->next_file_off;

  writer->next_file_off = make_align(writer->next_file_off + textseg.phdr.p_filesz, ALIGN_BYTES);
  writer->next_va = make_align(writer->next_va + textseg.phdr.p_memsz, ALIGN_BYTES);

  vec_append(&writer->segtab, &textseg);

  // write out .shstrtab
  Elf32_Shdr* sh_shstrtab = vec_get_item(&writer->shtab, writer->ehdr.e_shstrndx);
  sh_shstrtab->sh_size = writer->shstrtab.len;
  elf_writer_place_section(writer, sh_shstrtab); // will move writer->next_file_off

  // write out Ehdr
  // align next_file_off
  writer->next_file_off = make_align(writer->next_file_off, ALIGN_BYTES);
  Elf32_Ehdr* ehdr = &writer->ehdr;
  ehdr->e_phoff = writer->next_file_off;
  ehdr->e_phnum = writer->segtab.len;

  writer->next_file_off = make_align(writer->next_file_off + sizeof(Elf32_Phdr) * ehdr->e_phnum, ALIGN_BYTES);
  ehdr->e_shoff = writer->next_file_off;
  ehdr->e_shnum = writer->shtab.len;

  FILE* fp = fopen(path, "wb");
  assert(fp);
  fwrite(ehdr, sizeof(Elf32_Ehdr), 1, fp);

  // write segment content
  VEC_FOREACH(&writer->segtab, struct segment, segptr) {
    fseek(fp, segptr->phdr.p_offset, SEEK_SET);
    assert(segptr->file_cont.len == segptr->phdr.p_filesz);
    fwrite(segptr->file_cont.buf, 1, segptr->phdr.p_filesz, fp);
  }

  // write .shstrtab
  elf_writer_write_section(fp, sh_shstrtab, writer->shstrtab.buf);

  // write segment table
  fseek(fp, ehdr->e_phoff, SEEK_SET);
  VEC_FOREACH(&writer->segtab, struct segment, segptr2) {
    fwrite(&segptr2->phdr, 1, sizeof(Elf32_Phdr), fp);
  }

  // write section table. An elf without a NULL section will trigger warning
  // by running 'readelf -h'
  VEC_FOREACH(&writer->shtab, Elf32_Shdr, shdr) {
    fwrite(shdr, 1, sizeof(Elf32_Shdr), fp);
  }

  fclose(fp);
  printf("Done writing elf file to %s\n", path);
}

#endif
