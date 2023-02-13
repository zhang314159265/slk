#pragma once

#include "util.h"
#include "elf.h"
#include "vec.h"
#include <sys/stat.h>

struct elf_reader {
  char *buf; // the buffer storing the whole file content
  int file_size;

  Elf32_Ehdr* ehdr; // point to buf
  Elf32_Shdr* sht; // section header table point to buf
  int nsection;
  Elf32_Shdr* sh_shstrtab; // section header for string table for section names
  char *shstrtab; // the buffer for section header string table.

  Elf32_Sym* symtab;
  int nsym;
  char *symstr;
};

/*
 * Load a segment from the elf file. Make sure the segment does not go out of range.
 */
void *elf_reader_load_segment(struct elf_reader* reader, int start, int size) {
  if (size == 0) {
    return NULL;
  }
  assert(size > 0);
  assert(start >= 0 && start + size <= reader->file_size);
  return reader->buf + start;
}

/*
 * Do basic verifications of the elf file
 */
static void elf_reader_verify(struct elf_reader* reader) {
  // verify the magic number
  Elf32_Ehdr* ehdr = reader->ehdr;
  assert(ehdr->e_ident[0] == 0x7F
    && ehdr->e_ident[1] == 'E'
    && ehdr->e_ident[2] == 'L'
    && ehdr->e_ident[3] == 'F');

  // verify the section header table does not go out of range
  int sht_start = ehdr->e_shoff;
  assert(ehdr->e_shnum >= 0);
  assert(sizeof(Elf32_Shdr) == ehdr->e_shentsize);
  int sht_size = ehdr->e_shnum * sizeof(Elf32_Shdr);
  reader->sht = elf_reader_load_segment(reader, sht_start, sht_size);
  reader->nsection = ehdr->e_shnum;
}

/*
 * Get the address of section header for the given index. Fatal if the index
 * is out of range.
 */
Elf32_Shdr* elf_reader_get_sh(struct elf_reader* reader, int shidx) {
  assert(shidx >= 0 && shidx < reader->nsection);
  return reader->sht + shidx;
}

/*
 * Load content of a section given its section header.
 */
void* elf_reader_load_section(struct elf_reader* reader, Elf32_Shdr* sh) {
  return elf_reader_load_segment(reader, sh->sh_offset, sh->sh_size);
}

struct elf_reader elf_reader_create_from_buffer(const char* buf, int size) {
  struct elf_reader reader = {0};
  reader.file_size = size;
  reader.buf = (char *) buf;
  // verify elf header
  assert(reader.file_size >= sizeof(Elf32_Ehdr));
  reader.ehdr = (void*) reader.buf;
  elf_reader_verify(&reader);

  // set sh_shstrtab
  reader.sh_shstrtab = elf_reader_get_sh(&reader, reader.ehdr->e_shstrndx);
  reader.shstrtab = elf_reader_load_section(&reader, reader.sh_shstrtab);

  for (int i = 0; i < reader.nsection; ++i) {
    Elf32_Shdr* shdr = elf_reader_get_sh(&reader, i);
    Elf32_Shdr* shdr_link = NULL;
    switch (shdr->sh_type) {
    case SHT_SYMTAB:
      assert(!reader.symtab && "Assume a single symtab for now");
      reader.symtab = elf_reader_load_segment(&reader, shdr->sh_offset, shdr->sh_size);
      assert(shdr->sh_size % sizeof(Elf32_Sym) == 0);
      reader.nsym = shdr->sh_size / sizeof(Elf32_Sym);
      shdr_link = elf_reader_get_sh(&reader, shdr->sh_link);
      assert(shdr_link->sh_type == SHT_STRTAB);
      reader.symstr = elf_reader_load_segment(&reader, shdr_link->sh_offset, shdr_link->sh_size);
      break;
    default:
      break;
    }
  }
  return reader;
}

struct elf_reader elf_reader_create(const char* path) {
  printf("Create elf reader for %s\n", path);

  struct stat elf_st;
  int status = stat(path, &elf_st);
  assert(status == 0);
  int file_size = elf_st.st_size;
  char *buf = malloc(file_size);
  FILE* fp = fopen(path, "rb");
  assert(fp);
  status = fread(buf, 1, file_size, fp);
  assert(status == file_size);
  fclose(fp);

  return elf_reader_create_from_buffer(buf, file_size);
}

const char* _sht_to_str(int sht) {
  switch (sht) {
  case SHT_NULL:
    return "SHT_NULL";
  case SHT_PROGBITS:
    return "SHT_PROGBIGS";
  case SHT_SYMTAB:
    return "SHT_SYMTAB";
  case SHT_STRTAB:
    return "SHT_STRTAB";
  case SHT_NOBITS:
    return "SHT_NOBITS";
  case SHT_REL:
    return "SHT_REL";
  default:
    FAIL("Unrecognized section header type %d", sht);
  }
}

const char *_symtype_to_str(int type) {
  switch (type) {
  case STT_NOTYPE:
    return "STT_NOTYPE";
  case STT_FILE:
    return "STT_FILE";
  case STT_SECTION:
    return "STT_SECTION";
  case STT_OBJECT:
    return "STT_OBJECT";
  case STT_FUNC:
    return "STT_FUNC";
  default:
    FAIL("Unrecognized symbol type %d", type);
  }
}

const char *_symbind_to_str(int bind) {
  switch (bind) {
  case STB_LOCAL:
    return "STB_LOCAL";
  case STB_GLOBAL:
    return "STB_GLOBAL";
  case STB_WEAK:
    return "STB_WEAK";
  default:
    FAIL("Unrecognized symbol bind %d", bind);
  }
}

const char *_shn_to_str(int shn, int nsec) {
  static char buf[256]; // XXX not safe..
  switch (shn) {
  case SHN_UNDEF:
    return "SHN_UNDEF";
  case SHN_ABS:
    return "SHN_ABS";
  default:
    CHECK(shn > 0 && shn < nsec, "Unrecognized section number %d", shn);
    snprintf(buf, sizeof(buf), "%d", shn);
    return buf;
  }
}

/*
 * List the section header table.
 */
void elf_reader_list_sht(struct elf_reader* reader) {
  printf("The file has %d sections\n", reader->nsection);
  for (int i = 0; i < reader->nsection; ++i) {
    Elf32_Shdr* shdr = elf_reader_get_sh(reader, i);
    char *name = reader->shstrtab + shdr->sh_name; // TODO check for out of range access
    printf("  %d: '%s' %s size %d link %d\n",
      i, name,
      _sht_to_str(shdr->sh_type),
      shdr->sh_size,
      shdr->sh_link
    );
  }
}

int elf_reader_is_defined_section(struct elf_reader* reader, int secno) {
  return secno > 0 && secno < reader->nsection;
}

/* return a vec of names and caller should free it */
struct vec elf_reader_get_global_defined_syms(struct elf_reader* reader) {
  struct vec names = vec_create(sizeof(char*));
  for (int i = 0; i < reader->nsym; ++i) {
    Elf32_Sym* sym = reader->symtab + i;
    // int type = ELF32_ST_TYPE(sym->st_info);
    int bind = ELF32_ST_BIND(sym->st_info);
    char* name = reader->symstr + sym->st_name;
    if (bind == STB_GLOBAL && elf_reader_is_defined_section(reader, sym->st_shndx)) {
      vec_append(&names, &name);
    }
  }

  return names;
}

void elf_reader_list_syms(struct elf_reader* reader) {
  printf("The file contains %d symbols\n", reader->nsym);
  for (int i = 0; i < reader->nsym; ++i) {
    Elf32_Sym* sym = reader->symtab + i;
    int type = ELF32_ST_TYPE(sym->st_info);
    int bind = ELF32_ST_BIND(sym->st_info);
    printf("  %d: name '%s' type %s bind %s shndx %s\n",
      i, reader->symstr + sym->st_name,
      _symtype_to_str(type),
      _symbind_to_str(bind),
      _shn_to_str(sym->st_shndx, reader->nsection)
    );
  }
}

void elf_reader_free(struct elf_reader* reader) {
  if (reader->buf) {
    free(reader->buf);
  }
}
