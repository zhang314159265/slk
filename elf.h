/*
 * Copied from: https://github.com/zhang314159265/sas/blob/master/elf.h, which follows
 * the copy in glibc: https://github.com/bminor/glibc/blob/master/elf/elf.h
 * TODO: avoid duplicate this file across repos
 */
#pragma once

/* 
 * core elf data structures.
 * Follow glibc elf/elf.h for definitions.
 */

#include <stdint.h>

typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;

typedef uint16_t Elf32_Section;

#define EI_NIDENT 16

#define ET_REL 1 /* relocatable file */
#define ET_EXEC 2 /* executable file */

#define EM_386 3

#define SHT_NULL 0
#define SHT_PROGBITS 1 /* program data */
#define SHT_SYMTAB 2 /* symbol table */
#define SHT_STRTAB 3 /* string table */
#define SHT_NOBITS 8 /* program space with no data (.bss) */
#define SHT_REL 9 /* relocation entries, no addends */

/* legal values for sh_flags (section flags). */
#define SHF_WRITE (1 << 0) /* writable */
#define SHF_ALLOC (1 << 1) /* occupies memory during execution */
#define SHF_EXECINSTR (1 << 2) /* executable */
#define SHF_INFO_LINK (1 << 6) /* sh_info contains SHT index */


#define SHN_UNDEF 0 /* undefined section */
#define SHN_ABS 0xfff1 /* Associated symbol is absolute */

#define STB_LOCAL 0 /* local symbol */
#define STB_GLOBAL 1 /* global symbol */
#define STB_WEAK 2 /* weak symbol */

#define STT_NOTYPE 0 /* symbol type is unspecified */
#define STT_OBJECT 1 /* symbol is a data object */
#define STT_FUNC 2 /* symbol is a code object */
#define STT_SECTION 3 /* symbol associated with a section */
#define STT_FILE 4 /* symbol's name is file name */
#define STT_TLS 6 /* thread local data object */
#define STT_LOOS 10 /* start of os-specific */
#define STT_GNU_IFUNC 10 /* symbol is indirect code object */
#define STT_HIOS 12 /* end of os-specific */

#define R_386_32 1 /* direct 32 bit */
#define R_386_PC32 2 /* PC relative 32 bit */

typedef struct {
  unsigned char e_ident[EI_NIDENT]; /* magic number and other info */
  Elf32_Half e_type; /* object file type */
  Elf32_Half e_machine; /* architecture */
  Elf32_Word e_version; /* object file version */
  Elf32_Addr e_entry; /* entry point virtual address */
  Elf32_Off e_phoff; /* program header table file offset */
  Elf32_Off e_shoff; /* section header table file offset */
  Elf32_Word e_flags; /* processor specific flags */
  Elf32_Half e_ehsize; /* ElF header size in bytes */
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx; /* section header string table index */
} Elf32_Ehdr;

typedef struct {
  Elf32_Word sh_name; /* section table (string tbl index) */
  Elf32_Word sh_type; /* section type */
  Elf32_Word sh_flags; /* section flags */
  Elf32_Addr sh_addr; /* section virtual address at execution */
  Elf32_Off sh_offset; /* section file offset */
  Elf32_Word sh_size; /* section size in bytes */
  /*
   * link to another section.
   * E.g. .symtab link to .strtab
   *
   * for SHT_REL, sh_link points to the symtab section, sh_info point to
   * the section whose content need to be relocated (e.g. a .text section).
   *
   * For SHT_SYMTAB, sh_link points to the strtab section. The sh_info interpretation
   * is a bit ambiguous according to the spec.
   * - in one place, it's defined to point to the first non local symbol index
   * - in another place, it's defined to be one larger than the index of the
   *   last local symbol.
   * To make the 2 equivalent, all local symbols should reside at the beginning
   * of the symtab. That seems to be true since the spec says: in each symbol table,
   * all symbols with STB_LOCAL bingins precede the weak and global symbols.
   */
  Elf32_Word sh_link;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign; /* section alignment */
  Elf32_Word sh_entsize; /* entry size if section holds table */
} Elf32_Shdr;

#define ELF32_ST_BIND(val) (((unsigned char)(val)) >> 4)
#define ELF32_ST_TYPE(val) (((unsigned char)(val)) & 0xF)
#define ELF32_ST_INFO(bind, type) ((((bind) & 0xf) << 4) | ((type) & 0xf))

#define STB_LOCAL 0 /* local symbol */
#define STB_GLOBAL 1 /* global symbol */
#define STB_WEAK 2 /* weak symbol */

#define STT_NOTYPE 0 /* symbol type is unspecified */
#define STT_OBJECT 1 /* data */
#define STT_FUNC 2 /* code */
#define STT_SECTION 3 /* section name */
#define STT_FILE 4 /* symbol's name is file name */

/* special section indices. Used in Elf32_Sym */
#define SHN_UNDEF 0 /* undefined section */
#define SHN_ABS 0xfff1 /* symbol is absolute */

typedef struct {
  Elf32_Word st_name; /* symbol name (string tbl index) */
  Elf32_Addr st_value; /* symbol value */
  Elf32_Word st_size; /* symbol size */
  // high 4 bits for bind and low  4 bits for type
  unsigned char st_info; /* symbol type and binding */
  unsigned char st_other; /* symbol visibility */
  Elf32_Section st_shndx; /* section index */
} Elf32_Sym;

typedef struct {
  Elf32_Addr r_offset; /* address */
  /*
   * Relocation type and symbol index.
   * High 24 bits represents symtab index; low 8 bits
   * represents relocation type.
   */
  Elf32_Word r_info;
} Elf32_Rel;

typedef struct {
  Elf32_Word p_type; /* segment type */
  Elf32_Off p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Addr p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
} Elf32_Phdr;
