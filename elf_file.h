/*
 * Copied from: https://github.com/zhang314159265/sas/blob/master/elf_file.h
 * TODO: avoid duplicate this file across repos
 */
#pragma once

#include "elf.h"
#include "util.h"
#include "vec.h"
#include "str.h"
#include <assert.h>
#include <stdio.h>
#include "asctx.h"

/* data structures for reading and writing elf files */
struct elf_file {
  Elf32_Ehdr ehdr;
  struct vec shtab; // section header table. Contains Elf32_Shdr

  struct str shstrtab;
  struct str strtab;
  struct vec symtab;
  struct vec reltext;

  // the content of .text, .data will be from the asctx->sectab
  uint16_t shn_text;
  uint16_t shn_data;

  uint16_t shn_strtab;
  uint16_t shn_symtab;
  uint16_t shn_shstrtab;
  uint16_t shn_reltext;
};

/*
 * Fatal if there is no section with the name found.
 */
static int ef_get_shn_by_name(struct elf_file *ef, const char* name) {
  int idx = 0;
  VEC_FOREACH(&ef->shtab, Elf32_Shdr, shdr) {
    if (strcmp(ef->shstrtab.buf + shdr->sh_name, name) == 0) {
      return idx;
    }
    ++idx;
  }
  FAIL("section name not found %s", name);
}

/*
 * After parsing the whole assembly file, we know what are all the sections
 * mentioned in the file. Create the Elf32_Shdr in elf_file if it does not
 * exist yet. And then assign the section header number to the 'struct section'
 * in the asctx.
 *
 * The section header number in asctx will be used to create Elf32_Sym from labels.
 */
void ef_set_shn_in_ctx(struct elf_file* ef, struct asctx* ctx) {
  // TODO: this functions assumes all the section headers are preexist already.
  // we can do this for commonly known section names but may fail for customized
  // section names. Be able to create Elf32_Shdr on the fly.
  VEC_FOREACH(&ctx->sectab, struct section, sec) {
    sec->shn = ef_get_shn_by_name(ef, sec->name);
  }
}

/*
 * Add a section header before all it's fields can be decided (e.g. size/offset) so
 * that the section number can be available early enough.
 *
 * The section header need to be revised before writing the file.
 *
 * Return the index of the section header just added.
 */
int ef_add_section(struct elf_file* ef, const char* name, uint32_t type, uint32_t flags, uint32_t link, uint32_t info, uint32_t addralign, uint32_t entsize) {
  Elf32_Shdr newsh;
  memset(&newsh, 0, sizeof(Elf32_Shdr));

  int name_idx = 0;
  if (name) {
    name_idx = str_concat(&ef->shstrtab, name);
  }
  newsh.sh_name = name_idx;
  newsh.sh_type = type;
  newsh.sh_flags = flags;
  newsh.sh_link = link;
  newsh.sh_info = info;
  newsh.sh_addralign = addralign;
  newsh.sh_entsize = entsize;

  vec_append(&ef->shtab, &newsh);
  return ef->shtab.len - 1;
}

struct elf_file ef_create() {
  struct elf_file ef;
  Elf32_Ehdr* ehdr = &ef.ehdr;
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

  ehdr->e_type = ET_REL;
  ehdr->e_machine = EM_386;
  ehdr->e_version = 1;
  ehdr->e_entry = 0;
  ehdr->e_phoff = 0;
  ehdr->e_shoff = 0; // will be set to the correct value in ef_freeze
  ehdr->e_flags = 0;
  ehdr->e_ehsize = sizeof(Elf32_Ehdr);
  assert(sizeof(Elf32_Ehdr) == 52); // this should be a static assert

  ehdr->e_phentsize = 0; // no need to provide the real size since there are not program headers in relocatable file
  ehdr->e_phnum = 0;
  ehdr->e_shentsize = sizeof(Elf32_Shdr);
  assert(sizeof(Elf32_Shdr) == 40);
  ehdr->e_shnum = 0; // will be setup in ef_freeze
  ehdr->e_shstrndx = 0; // Setup later in this function

  ef.shtab = vec_create(sizeof(Elf32_Shdr));
  // add a null section
  ef_add_section(&ef, NULL, 0, 0, 0, 0, 0, 0);

  ef.shstrtab = str_create(0);
  // add a 0 at the beginning so index 0 represents an empty string
  // (this happen for the null section 0)
  str_append(&ef.shstrtab, 0);
  ef.strtab = str_create(0);
  str_append(&ef.strtab, 0);
  ef.symtab = vec_create(sizeof(Elf32_Sym));
  Elf32_Sym null_sym;
  memset(&null_sym, 0, sizeof(Elf32_Sym));
  vec_append(&ef.symtab, &null_sym);

  ef.reltext = vec_create(sizeof(Elf32_Rel));

  ef.shn_text = ef_add_section(&ef, ".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 0, 0, 1, 0);
  ef.shn_data = ef_add_section(&ef, ".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, 0, 0, 1, 0);
  ef.shn_shstrtab = ef_add_section(&ef, ".shstrtab", SHT_STRTAB, 0, 0, 0, 1, 0);
  ehdr->e_shstrndx = ef.shn_shstrtab;
  ef.shn_strtab = ef_add_section(&ef, ".strtab", SHT_STRTAB, 0, 0, 0, 1, 0);
  // the sh_info field will be set in ef_freeze once we know all the Elf32_Sym entries
  ef.shn_symtab = ef_add_section(&ef, ".symtab", SHT_SYMTAB, 0, ef.shn_strtab, 0, 4, sizeof(Elf32_Sym));
  ef.shn_reltext = ef_add_section(&ef, ".rel.text", SHT_REL, SHF_INFO_LINK, ef.shn_symtab, ef.shn_text, 4, sizeof(Elf32_Rel));
  return ef;
}

/*
 * Value represents offset to the section for function symbols.
 *
 * Return the index of the newly added Elf32_Sym
 */
int ef_add_symbol(struct elf_file* ef, const char *name, int shn, int value, int size, int bind, int type) {
  int name_idx = str_concat(&ef->strtab, name);

  Elf32_Sym sym;
  sym.st_name = name_idx;
  sym.st_value = value;
  sym.st_size = size;
  sym.st_info = (((bind) << 4) | type);
  sym.st_other = 0; // default visibility
  sym.st_shndx = shn;
  vec_append(&ef->symtab, &sym);
  return ef->symtab.len - 1;
}

/*
 * Try to place the section at specified offset and return the new offset
 * after the placement. Alignment is handled properly.
 *
 * The sh_size field should be already setup before calling this function.
 */
static int ef_place_section(struct elf_file* ef, Elf32_Shdr* sh, int off) {
  int align = sh->sh_addralign;
  assert(align > 0);
  off = make_align(off, align);
  sh->sh_offset = off;
  assert(sh->sh_size >= 0);
  return off + sh->sh_size;
}

/*
 * Finalize the content of section header and then elf header (e.g. shoff)
 * by 'virtually' write the elf file.
 */
static void ef_freeze(struct elf_file* ef, struct asctx* ctx) {
  // virtually place the Ehdr
  int next_off = sizeof(Elf32_Ehdr);

  // freeze all sections in asctx. This will include .text, .data
  VEC_FOREACH(&ctx->sectab, struct section, sec) {
    assert(sec->shn > 0);
    Elf32_Shdr* shdr = vec_get_item(&ef->shtab, sec->shn);
    shdr->sh_size = sec->cont.len;
    shdr->sh_addralign = sec->align;
    next_off = ef_place_section(ef, shdr, next_off);
  }

  // strtab
  Elf32_Shdr* strtab_shdr = vec_get_item(&ef->shtab, ef->shn_strtab);
  strtab_shdr->sh_size = ef->strtab.len;
  next_off = ef_place_section(ef, strtab_shdr, next_off);

  // symtab
  Elf32_Shdr* symtab_shdr = vec_get_item(&ef->shtab, ef->shn_symtab);
  symtab_shdr->sh_size = ef->symtab.len * ef->symtab.itemsize;
  next_off = ef_place_section(ef, symtab_shdr, next_off);
  {
    int i;
    for (i = 0; i < ef->symtab.len; ++i) {
      Elf32_Sym *sym = vec_get_item(&ef->symtab, i);
      if (ELF32_ST_BIND(sym->st_info) != STB_LOCAL) {
        break;
      }
    }
    symtab_shdr->sh_info = i;
    for (; i < ef->symtab.len; ++i) {
      Elf32_Sym *sym = vec_get_item(&ef->symtab, i);
      CHECK(ELF32_ST_BIND(sym->st_info) != STB_LOCAL, "Local symbol should be placed before global/weak symbols");
    }
  }

  // shstrtab
  Elf32_Shdr* shstrtab_shdr = vec_get_item(&ef->shtab, ef->shn_shstrtab);
  shstrtab_shdr->sh_size = ef->shstrtab.len;
  next_off = ef_place_section(ef, shstrtab_shdr, next_off);

  // .rel.text
  Elf32_Shdr* reltext_shdr = vec_get_item(&ef->shtab, ef->shn_reltext);
  reltext_shdr->sh_size = ef->reltext.len * ef->reltext.itemsize;
  next_off = ef_place_section(ef, reltext_shdr, next_off);

  // now we know where the section header table should go
  next_off = make_align(next_off, 16); // force 16 bytes alignment for the section header table
  ef->ehdr.e_shoff = next_off;
  ef->ehdr.e_shnum = ef->shtab.len;
}

static void ef_write_section(FILE* fp, Elf32_Shdr* sh, void *data) {
  fseek(fp, sh->sh_offset, SEEK_SET);
  fwrite(data, 1, sh->sh_size, fp);
}

static void ef_write_shtab(struct elf_file* ef, FILE* fp) {
  fseek(fp, ef->ehdr.e_shoff, SEEK_SET);
  fwrite(ef->shtab.data, sizeof(Elf32_Shdr), ef->shtab.len, fp);
}

static void ef_add_symbol_from_label_metadata(struct elf_file* ef, const char* name, struct label_metadata* md) {
  int off = md->off;
  assert(md->section->shn > 0);
  int shn = md->section->shn;
  int size = md->size;
  int bind = md->bind;
  int type = md->type;
  ef_add_symbol(ef, name, shn, off, size, bind, type);
}

/*
 * Perform 2 passes to ut local symbols before global/weak symbols
 */
static void ef_add_symbols_from_labels(struct elf_file* ef, struct asctx* ctx) {
  asctx_dump_labels(ctx);
  DICT_FOREACH(&ctx->label2idx, entry) {
    assert(entry->key);
    const char* name = entry->key;
    struct label_metadata* md = vec_get_item(&ctx->labelmd_list, entry->val);
    if (md->bind == STB_LOCAL) {
      ef_add_symbol_from_label_metadata(ef, name, md);
    }
  }
  DICT_FOREACH(&ctx->label2idx, entry) {
    assert(entry->key);
    const char* name = entry->key;
    struct label_metadata* md = vec_get_item(&ctx->labelmd_list, entry->val);
    if (md->bind != STB_LOCAL) {
      ef_add_symbol_from_label_metadata(ef, name, md);
    }
  }
}

/*
 * Write the elf_file to path.
 *
 * Take 2 passes:
 * 1. decide each section's offset. (call freeze)
 * 2. write to file.
 *
 * An alternative solution is to do in one pass but do necessary back patching
 * in the end.
 */
static void ef_write(struct elf_file* ef, struct asctx* ctx, const char* path) {
  ef_freeze(ef, ctx);
  assert(ef->ehdr.e_ehsize == sizeof(Elf32_Ehdr));
  assert(ef->ehdr.e_shoff > 0);
  assert(ef->ehdr.e_shnum > 0);
  assert(ef->ehdr.e_shstrndx > 0);

  FILE* fp = fopen(path, "wb");
  fwrite(&(ef->ehdr), sizeof(Elf32_Ehdr), 1, fp);

  // write sections
  VEC_FOREACH(&ctx->sectab, struct section, sec) {
    Elf32_Shdr* shdr = vec_get_item(&ef->shtab, sec->shn);
    ef_write_section(fp, shdr, sec->cont.buf);
  }

  // strtab
  Elf32_Shdr* strtab_shdr = vec_get_item(&ef->shtab, ef->shn_strtab);
  ef_write_section(fp, strtab_shdr, ef->strtab.buf);

  // symtab
  Elf32_Shdr* symtab_shdr = vec_get_item(&ef->shtab, ef->shn_symtab);
  ef_write_section(fp, symtab_shdr, ef->symtab.data);

  // shstrtab
  Elf32_Shdr* shstrtab_shdr = vec_get_item(&ef->shtab, ef->shn_shstrtab);
  ef_write_section(fp, shstrtab_shdr, ef->shstrtab.buf);

  // reltext
  Elf32_Shdr* reltext_shdr = vec_get_item(&ef->shtab, ef->shn_reltext);
  ef_write_section(fp, reltext_shdr, ef->reltext.data);

  ef_write_shtab(ef, fp);
  fclose(fp);
}

void ef_dump_syms(struct elf_file* ef) {
  printf("The file contains %d symbols:\n", ef->symtab.len);
  VEC_FOREACH(&ef->symtab, Elf32_Sym, esymptr) {
    const char* ename = ef->strtab.buf + esymptr->st_name;
    printf(" - name '%s'\n", ename);
  }
}

/*
 * If the symbol is not found, create an undefined symbol.
 *
 * Return the symbol index.
 */
int ef_get_or_create_symidx(struct elf_file* ef, const char* name) {
  // TODO: imporve the perf by avoiding linear scan
  int symidx = -1;
  int i = 0;
  VEC_FOREACH(&ef->symtab, Elf32_Sym, esymptr) {
    const char* ename = ef->strtab.buf + esymptr->st_name;
    if (strcmp(name, ename) == 0) {
      symidx = i;
      break;
    }
    ++i;
  }
  if (symidx < 0) {
    symidx = ef_add_symbol(ef, name, SHN_UNDEF, 0, 0, STB_GLOBAL, STT_NOTYPE);
  }
  return symidx;
}

/*
 * TODO: for relocation that can be resolved right away (e.g. entries for jcc)
 * don't create a Elf32_Rel entry in the ELF file.
 */
static void ef_handle_relocs(struct elf_file* ef, struct asctx* ctx) {
  asctx_dump_relocs(ctx);
  VEC_FOREACH(&ctx->rel_list, struct as_rel_s, relptr) {
    Elf32_Rel elf_rel;
    const char* symname = lenstrdup(relptr->sym, relptr->symlen);
    int elf_rel_sym = ef_get_or_create_symidx(ef, symname);
    free((void*) symname);
    int elf_rel_reltype = relptr->rel_type;

    // TODO: have a macro to handle this
    elf_rel.r_info = ((elf_rel_sym << 8) | elf_rel_reltype);
    elf_rel.r_offset = relptr->offset;
    vec_append(&ef->reltext, &elf_rel);

    // assign addend to the double-word that will be patched in future
    // TODO: here we assum we are relocating the text section
    *(uint32_t*) (ctx->textsec->cont.buf + elf_rel.r_offset) = relptr->addend;
  }
}

void ef_free(struct elf_file* ef) {
  vec_free(&ef->shtab);
  str_free(&ef->shstrtab);
  str_free(&ef->strtab);
  vec_free(&ef->symtab);
  vec_free(&ef->reltext);
}
