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

struct elf_writer {
  Elf32_Ehdr ehdr;
  struct str textbuf;
  struct vec phdrtab; // TODO make use of this
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
  ehdr->e_entry = 0; // TODO need change
  ehdr->e_phoff = 0; // TODO need change
  ehdr->e_shoff = 0; // TODO need change
  ehdr->e_flags = 0;
  ehdr->e_ehsize = sizeof(Elf32_Ehdr);
  assert(sizeof(Elf32_Ehdr) == 52); // this should be a static assert

  ehdr->e_phentsize = sizeof(Elf32_Phdr);
  assert(sizeof(Elf32_Phdr) == 32);
  ehdr->e_phnum = 0; // TODO need update
  ehdr->e_shentsize = sizeof(Elf32_Shdr);
  assert(sizeof(Elf32_Shdr) == 40);
  ehdr->e_shnum = 0; // TODO need update
  ehdr->e_shstrndx = 0; // TODO need update
}

static struct elf_writer elf_writer_create() {
  struct elf_writer writer;
  elf_writer_init_ehdr(&writer);
  writer.phdrtab = vec_create(sizeof(Elf32_Phdr));
  return writer;
}

static void elf_write_set_text_manually(struct elf_writer* writer, struct str* textstr) {
  writer->textbuf = str_move(textstr);
}

static void elf_writer_write(struct elf_writer* writer, const char* path) {
  assert(false && "elf_writer_write ni");
}

static void elf_writer_free(struct elf_writer* writer) {
  vec_free(&writer->phdrtab);
  str_free(&writer->textbuf);
}

// I can image the following APIs that can help creating an executable ELF.
// But they are not necessarily to be the best:
// TODO: enter_ph
// TODO: leave_ph
// TODO: add_section (add this to the current ph)
// TODO: use a in memory buffer for the entire file for now
// maybe we can skip it.
