#pragma once

#include "vec.h"
#include "elf.h"

struct segment {
  Elf32_Phdr phdr;
  struct str file_cont;
};

// TODO: handle the bss where file_size < mem_size
struct segment segment_create(uint32_t file_off, uint32_t va, struct str* buf) {
  struct segment seg;
  Elf32_Phdr* phdr = &seg.phdr;

  phdr->p_type = PT_LOAD; // TODO don't hardcode
  phdr->p_offset = file_off;
  phdr->p_vaddr = va;
  phdr->p_paddr = phdr->p_vaddr;
  phdr->p_filesz = buf->len;
  phdr->p_memsz = buf->len;
  phdr->p_flags = PF_X | PF_R; // TODO don't hardcode
  phdr->p_align = 16; // TODO don't hardcode

  seg.file_cont = str_move(buf);
  return seg;
}

void segment_free(struct segment* seg) {
  str_free(&seg->file_cont);
}
