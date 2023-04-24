/*
 * TODO consolidate with elf_file.h. The following things need to be resolved
 * 1. elf_file.h rely on arctx
 * 2. elf_file.h only handles relocable file.
 *    while this file only handles executable file
 */
#pragma once

#include <assert.h>
#include "elf.h"
#include "util.h"
#include "str.h"
#include "vec.h"
#include "segment.h"

#define ENTRY 0x50000000
#define ALIGN_BYTES 16

struct elf_writer {
  Elf32_Ehdr ehdr;
  struct str textbuf;
  struct vec phdrtab; // TODO make use of this

  struct vec segtab; // contains a list of 'struct segment'

  uint32_t next_file_off;
  uint32_t next_va;
};

static void elf_writer_init_ehdr(struct elf_writer* writer) {
  Elf32_Ehdr* ehdr = &writer->ehdr;
  memset(ehdr, 0, sizeof(*ehdr));
  ehdr->e_ident[0] = 0x7f;
  ehdr->e_ident[1] = 'E';
  ehdr->e_ident[2] = 'L';
  ehdr->e_ident[3] = 'F';
  ehdr->e_ident[4] = 1; // ELFCLASS32
  ehdr->e_ident[5] = 1; // little endian
  ehdr->e_ident[6] = 1; // file version == EV_CURRENT
  ehdr->e_ident[7] = 0; // OSABI
  ehdr->e_ident[8] = 0; // ABI version

  ehdr->e_type = ET_EXEC;
  ehdr->e_machine = EM_386;
  ehdr->e_version = 1;
  ehdr->e_entry = ENTRY;
  ehdr->e_phoff = 0;
  ehdr->e_shoff = 0; // TODO need change
  ehdr->e_flags = 0;
  ehdr->e_ehsize = sizeof(Elf32_Ehdr);
  assert(sizeof(Elf32_Ehdr) == 52); // this should be a static assert

  ehdr->e_phentsize = sizeof(Elf32_Phdr);
  assert(sizeof(Elf32_Phdr) == 32);
  ehdr->e_phnum = 0;
  ehdr->e_shentsize = sizeof(Elf32_Shdr);
  assert(sizeof(Elf32_Shdr) == 40);
  ehdr->e_shnum = 0; // TODO need update
  ehdr->e_shstrndx = 0; // TODO need update
}

static struct elf_writer elf_writer_create() {
  struct elf_writer writer;
  elf_writer_init_ehdr(&writer);
  writer.phdrtab = vec_create(sizeof(Elf32_Phdr));
  writer.segtab = vec_create(sizeof(struct segment));

  writer.next_file_off = sizeof(writer.ehdr);
  writer.next_va = writer.ehdr.e_entry;
  return writer;
}

static void elf_write_set_text_manually(struct elf_writer* writer, struct str* textstr) {
  writer->textbuf = str_move(textstr);
}

static void elf_writer_write(struct elf_writer* writer, const char* path) {
  struct segment textseg = segment_create(
    writer->next_file_off,
    writer->next_va,
    &writer->textbuf);

  writer->next_file_off = make_align(writer->next_file_off + textseg.phdr.p_filesz, ALIGN_BYTES);
  writer->next_va = make_align(writer->next_va + textseg.phdr.p_memsz, ALIGN_BYTES);

  vec_append(&writer->segtab, &textseg);

  // write out Ehdr
  Elf32_Ehdr* ehdr = &writer->ehdr;
  ehdr->e_phoff = writer->next_file_off;
  ehdr->e_phnum = writer->segtab.len;

  FILE* fp = fopen(path, "wb");
  assert(fp);
  fwrite(ehdr, sizeof(Elf32_Ehdr), 1, fp);

  // write segment content
  VEC_FOREACH(&writer->segtab, struct segment, segptr) {
    fseek(fp, segptr->phdr.p_offset, SEEK_SET);
    assert(segptr->file_cont.len == segptr->phdr.p_filesz);
    fwrite(segptr->file_cont.buf, 1, segptr->phdr.p_filesz, fp);
  }

  // write segment table
  fseek(fp, ehdr->e_phoff, SEEK_SET);
  VEC_FOREACH(&writer->segtab, struct segment, segptr2) {
    fwrite(&segptr2->phdr, 1, sizeof(Elf32_Phdr), fp);
  }
  fclose(fp);
  printf("Done writing elf file to %s\n", path);
}

static void elf_writer_free(struct elf_writer* writer) {
  vec_free(&writer->phdrtab);
  VEC_FOREACH(&writer->segtab, struct segment, segptr) {
    segment_free(segptr);
  }
  vec_free(&writer->segtab);
  str_free(&writer->textbuf);
}

// I can image the following APIs that can help creating an executable ELF.
// But they are not necessarily to be the best:
// TODO: enter_ph
// TODO: leave_ph
// TODO: add_section (add this to the current ph)
// TODO: use a in memory buffer for the entire file for now
// maybe we can skip it.
