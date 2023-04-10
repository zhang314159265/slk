/*
 * TODO consolidate with elf_file.h. The following things need to be resolved
 * 1. elf_file.h rely on arctx
 * 2. elf_file.h only handles relocable file.
 *    while this file only handles executable file
 */
#pragma once

#include "elf.h"

struct elf_writer {
  Elf32_Ehdr ehdr;
};

static struct elf_writer elf_writer_create() {
  struct elf_writer writer;
  return writer;
}

static void elf_writer_free(struct elf_writer* writer) {
}
