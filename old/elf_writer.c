#include "elf_writer.h"
#include "lkctx.h"

// TODO: free the buffer in each segment
void elf_writer_create_segment(struct elf_writer* writer, struct lkctx* ctx, const char* secname) {
  // THIS ALIGNMENT is THE KEY TO make the generated ELF file work!
  writer->next_file_off = make_align(writer->next_file_off, 4096);
  writer->next_va = make_align(writer->next_va, 4096);

  // merge the secname from each reader into a final buffer
  struct str segbuf = str_create(8);
  int seglen = 0;
  bool isbss = (strcmp(secname, ".bss") == 0);
  VEC_FOREACH(&ctx->readers, struct elf_reader, rdptr) {
    Elf32_Shdr *shdr = elfr_get_shdr_by_name(rdptr, secname);
    if (!shdr) {
      continue;
    }
    int padded_len = make_align(segbuf.len, shdr->sh_addralign) - segbuf.len;
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

  assert(segbuf.len == 0 || segbuf.len == seglen);

  struct segment seg = segment_create(writer->next_file_off, writer->next_va, &segbuf, seglen, secname);
  vec_append(&writer->segtab, &seg);
  writer->next_file_off = make_align(writer->next_file_off + seg.phdr.p_filesz, ALIGN_BYTES);
  writer->next_va = make_align(writer->next_va + seg.phdr.p_memsz, ALIGN_BYTES);
}

void elf_writer_write_with_ctx(struct elf_writer* writer, const char* path, struct lkctx* ctx) {
  // set the entry correctly
  writer->ehdr.e_entry = (uint32_t) dict_find_nomiss(&ctx->sym_name_to_abs_addr, "_start");
  
	// follow elf_writer_write to write the elf file using the information
	// in lkctx.
  elf_writer_create_segment(writer, ctx, ".text");
  // TODO: .data and .bss can be merged
  elf_writer_create_segment(writer, ctx, ".data");
  elf_writer_create_segment(writer, ctx, ".bss");
  elf_writer_create_segment(writer, ctx, ".rodata");

  assert(writer->segtab.len == 4);

  // place the .shstrtab
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

  // write the file content
  FILE* fp = fopen(path, "wb");
  assert(fp);
  fwrite(ehdr, sizeof(Elf32_Ehdr), 1, fp);

  // write segment content
  VEC_FOREACH(&writer->segtab, struct segment, segptr) {
    fseek(fp, segptr->phdr.p_offset, SEEK_SET);
    if (segptr->file_cont.len != segptr->phdr.p_filesz) {
      printf("buf len %d, file size %d\n", segptr->file_cont.len, segptr->phdr.p_filesz);
    }
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
