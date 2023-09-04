#pragma once

#include "scom/vec.h"
#include "scom/elf.h"

struct segment {
  Elf32_Phdr phdr;
  struct str file_cont;
};

// TODO: handle the bss where file_size < mem_size
static struct segment segment_create(uint32_t file_off, uint32_t va, struct str* buf, int seglen, const char* secname) {
  assert(file_off % 4096 == 0);
  assert(va % 4096 == 0);
  struct segment seg;
  Elf32_Phdr* phdr = &seg.phdr;

  bool isbss = (strcmp(secname, ".bss") == 0);

  phdr->p_type = PT_LOAD; // TODO don't hardcode
  phdr->p_offset = file_off;
  phdr->p_vaddr = va;
  phdr->p_paddr = phdr->p_vaddr;
  phdr->p_memsz = seglen;
  if (isbss) {
    phdr->p_filesz = 0;
  } else {
    phdr->p_filesz = seglen;
  }
  // phdr->p_flags = PF_X | PF_R; // TODO don't hardcode
  phdr->p_flags = PF_R;
  if (strcmp(secname, ".text") == 0) {
    phdr->p_flags |= PF_X;
  }
  if (strcmp(secname, ".data") == 0 || strcmp(secname, ".bss") == 0) {
    phdr->p_flags |= PF_W;
  }
  phdr->p_align = 16; // TODO don't hardcode

  seg.file_cont = str_move(buf);
  return seg;
}

static void segment_free(struct segment* seg) {
  str_free(&seg->file_cont);
}
