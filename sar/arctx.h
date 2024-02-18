#pragma once

#include "elf_member.h"

#define SIGNATURE "!<arch>\n"
#define SIGNATURE_LEN 8

#define FILE_HEADER_LEN 60

#ifndef dprintf
#define dprintf(...)
#endif

struct file_header {
  char file_identifier[16];
  char mod_ts[12]; // file modification timestamp
  char owner_id[6];
  char group_id[6];
  char file_mod[8];
  char file_size[10]; // file size in bytes
  char signature[2];
} __attribute__((packed));

static_assert(sizeof(struct file_header) == FILE_HEADER_LEN);

#define FILE_IDENTIFIER_LEN sizeof(((struct file_header*) 0)->file_identifier)

struct arctx {
  int file_size;
  char* buf; // to simply the implementation we load the whole file into buf

  struct vec elf_mem_list; // a list of struct elf_member each represents an embedded elf file.

  // long_name_buf point somewhere in buf. It's used to resolve file names
  // longer than some limit.
  char *long_name_buf;
  int long_name_buflen;

  struct vec sglist; // a list of symbol groups. Use for the symbol lookup table.

  // symbol to the index of member defining it.
  // A typical workflow is, given a symbol to be resolved, we find the elf member
  // defining it. And then we need recursively resolve all the unresolved symbols needed
  // by that elf member.
  struct dict symname2memidx;
	struct dict symname2weak;  // weakness information used to handle redefined symbols
};

static void arctx_parse(struct arctx* ctx);

/*
 * Guarantee to read len bytes from file to buf.
 */
static void _read_file(FILE* fp, char* buf, int len) {
  int status = fread(buf, 1, len, fp);
  assert(status == len);
}

static struct arctx arctx_create(const char* path) {
  struct arctx ctx;
  memset(&ctx, 0, sizeof(ctx));
  struct stat ar_st;
  int status = stat(path, &ar_st);
  assert(status == 0);

  ctx.file_size = ar_st.st_size;
  FILE* fp = fopen(path, "rb");
  assert(fp);

  ctx.buf = malloc(ar_st.st_size);
  _read_file(fp, ctx.buf, ctx.file_size);
  fclose(fp);

  assert(strncmp(ctx.buf, SIGNATURE, SIGNATURE_LEN) == 0);

  ctx.elf_mem_list = vec_create(sizeof(struct elf_member));
  ctx.sglist = vec_create(sizeof(struct sym_group));
  arctx_parse(&ctx);
  return ctx;
}

static int _is_elf_member(struct arctx* ctx, int off, int size) {
  return size >= 4 /* #bytes of the magic number */
    && ctx->buf[off] == 0x7f
    && ctx->buf[off + 1] == 'E'
    && ctx->buf[off + 2] == 'L'
    && ctx->buf[off + 3] == 'F';
}

// parse a long name like
// '/123      '
//
// Return NULL if fail to parse the long name. Otherwise, caller should free the
// memory for the name.
static char *arctx_resolve_long_member_name(struct arctx* ctx, char* file_identifier) {
  assert(file_identifier[0] == '/' && isdigit(file_identifier[1]));

  if (!ctx->long_name_buf) {
    return NULL;
  }
  int off = atoi(file_identifier + 1);
  if (off >= ctx->long_name_buflen) {
    return NULL;
  }
 
  int idx = off;
  // '/' is used to mark the end of a filename
  for (; idx < ctx->long_name_buflen && ctx->long_name_buf[idx] != '/'; ++idx) {
  }
  // different file names are separated by one or more '\n'
  if (idx == ctx->long_name_buflen || idx == ctx->long_name_buflen - 1 || ctx->long_name_buf[idx + 1] != '\n') {
    return NULL;
  }
  int len = idx - off;
  char *name = malloc(len + 1);
  memcpy(name, ctx->long_name_buf + off, len);
  name[len] = '\0';
  return name;
}

/*
 * Return NULL if not able to fully resolve the name for the member.
 * Otherwise return the resolved name. Caller is responsible to free the memory.
 */
static char* arctx_resolve_member_name(struct arctx* ctx, char *file_identifier) {
  if (file_identifier[0] == '/' && isdigit(file_identifier[1])) {
    // long name
    return arctx_resolve_long_member_name(ctx, file_identifier);
  }

  int idx = FILE_IDENTIFIER_LEN - 1;

  // find the rightmost first non space
  while (idx >= 0 && file_identifier[idx] == ' ') {
    --idx;
  }
  if (idx < 0 || file_identifier[idx] != '/' || idx == 0) {
    return NULL;
  }

  // find potential more '/'
  int idx2 = idx - 1;
  while (idx2 >= 0 && file_identifier[idx2] != '/') {
    --idx2;
  }
  if (idx2 >= 0) {
    return NULL; // multi slash
  }

  // successfully resolve the name, make a copy
  char *name = malloc(idx + 1);
  memcpy(name, file_identifier, idx);
  name[idx] = '\0';
  return name;
}

/*
 * All the numbers in the sym_lookup_table member are 4 byte big endian numbers.
 *
 * The payload of the sym_lookup_table member starts with the number of entiries.
 * A location (offset of the header for the corresponding member defining the symbol) follows for each entry.
 * Then it follows with the '\0' terminated symbol name for each entry.
 *
 * It's common that location list contains the same location multiple times.
 * This happens when there are multiple symbols for the same member.
 */
static void arctx_parse_sym_lookup_table(struct arctx* ctx, const char* payload, int size) {
  assert(size >= 4); // at least enough space for number of entries
  int off = 0;
  int nentry = *(uint32_t*) (payload + off);
  // the number of entry is a 32 bit big endian number. Thus we need
  // convert it to little endian.
  nentry = endian_swap(nentry);

  off += 4;
  dprintf("Symbol lookup table contains %d entries\n", nentry);
  assert(nentry >= 0);

  int loclist_off = off;
  off += 4 * nentry;
  assert(off <= size);

  // iterate thru the name list
  for (int i = 0; i < nentry; ++i) {
    int j = off;
    while (j < size && payload[j]) {
      ++j;
    }
    assert(j < size);
    const char *name = payload + off;
    off = j + 1;

    // parse the corresponding location
    int loc = endian_swap(*(int32_t*) (payload + loclist_off + i * 4));
    sglist_add_entry(&ctx->sglist, loc, name);
  }

  // there may be padding bytes after the last name. Assume there is at most 1 padding
  // byte and the byte should be '\0'. Revise if it's not true.
  if (off < size) {
    assert(off % 2); // padding only happens if off is odd
    assert(payload[off] == '\0');
    ++off;
  }

  CHECK(off == size, "Fail to parse symbol lookup table off %d v.s. size %d", off, size);
  dprintf("Found %d symbol groups in the symbol lookup table\n", ctx->sglist.len);
}

// TODO: we should use a hash table
static void arctx_assign_sym_group_to_elf_member(struct arctx* ctx) {
  assert(ctx->sglist.len > 0);
  assert(ctx->elf_mem_list.len > 0);
  // it's possible that some elf file does not have a symbol group
  assert(ctx->sglist.len <= ctx->elf_mem_list.len);

  VEC_FOREACH_I(&ctx->sglist, struct sym_group, sg, i) {
    struct elf_member* found_elf = NULL;
    VEC_FOREACH_I(&ctx->elf_mem_list, struct elf_member, elf, j) {
      if (sg->header_off == elf->payload_off - FILE_HEADER_LEN) {
        // found
        elf->sgidx = i;
        found_elf = elf;
        break;
      }
    }
    assert(found_elf);
  }
}

/*
 * Here are the rules followed in this function to handle duplicate symbols
 * 1. if both are strong symbols, report an error unless the symbol is
 *    in a small predefined list (e.g. if it's "__x86.get_pc_thunk.xx")
 * 2. if one symbol is string while another is weak, pick the strong one
 * 3. if both symbols are weak, pick the first one.
 */
static void arctx_build_symname2memidx(struct arctx* ctx) {
  ctx->symname2memidx = dict_create_str_int(); 
	ctx->symname2weak = dict_create_str_int();
  VEC_FOREACH_I(&ctx->elf_mem_list, struct elf_member, memptr, i) {
    VEC_FOREACH_I(&memptr->provide_syms, char*, pstr, j) {
			int new_weak = *(bool*) vec_get_item(&memptr->weaks, j);
      // add the mapping between *pstr and i to symname2memidx
      struct dict_entry* pentry = dict_find(&ctx->symname2memidx, *pstr);
      if (pentry) {
				int old_weak = (int) dict_find_nomiss(&ctx->symname2weak, *pstr);

        /*
         * Symbols like __x86.get_pc_thunk.ax use tricks to store %eip to
         * general purpose registers like %eax.
         * Example assembly code looks like:
         *  00000000 <__x86.get_pc_thunk.ax>:
         *     0:  8b 04 24               mov    (%esp),%eax
         *     3:  c3                     ret    
         * They may get defined in multiple .o files. E.g., in libc.a,
         * __x86.get_pc_thunk.ax is defined in both init-first.o and
         * libc-start.o as global symbols (show as 'T' in nm command output).
         *
         * We don't count this as an error (otherwise a linker can not
         * succeed to link with libc.a..). Picking any copy of the symbol should
         * work assuming different .o file implement it identically.
         * We pick the first one found.
				 *
				 * In libc.a the following symbols are also defined as strong in multiple
				 * objective files: __memcpy_chk, __mempcpy_chk, __memmove_chk, __libc_do_syscall
				 *
				 * Also if both symbols are weak, pick the first one.
				 */
				if (old_weak && new_weak) {
					// ignore the new entry
				} else if (!old_weak && !new_weak) {
          if (startswith(pentry->key, "__x86.get_pc_thunk.")
					    || strcmp(pentry->key, "__memcpy_chk") == 0
					    || strcmp(pentry->key, "__mempcpy_chk") == 0
					    || strcmp(pentry->key, "__memmove_chk") == 0
					    || strcmp(pentry->key, "__libc_do_syscall") == 0
					) {
            // ignore the new entry
          } else {
            struct elf_member* prevmemptr = vec_get_item(&ctx->elf_mem_list, (int) pentry->val);
            FAIL("\033[31mSymbol %s defined in both %s and %s\033[0m", (char*) pentry->key, prevmemptr->name, memptr->name);
          }
			  } else {
					if (new_weak) {
						assert(!old_weak);
						// keep the old one since it's string
					} else {
						assert(old_weak);
						// use the new one since it's string
						pentry->val = (void*) i;
					}
				}
      } else {
        dict_put(&ctx->symname2memidx, (void*) strdup(*pstr), (void*) i);
				dict_put(&ctx->symname2weak, (void*) strdup(*pstr), (void*) new_weak);
      }
    }
  }
}

static void arctx_verify_all_members(struct arctx* ctx) {
  int idx = 0;
  int tot = ctx->elf_mem_list.len;
  VEC_FOREACH(&ctx->elf_mem_list, struct elf_member, elfmem) {
    dprintf("Verifying %d/%d member...\n", idx++, tot);
    elfmem_verify(elfmem, ctx->sglist);
  }
  dprintf("Pass verifying all members in the .a file\n");
}

/*
 * Parse the archive file.
 */
static void arctx_parse(struct arctx* ctx) {
  assert(ctx->file_size >= SIGNATURE_LEN + FILE_HEADER_LEN);
  int off = SIGNATURE_LEN;

  int num_unresolved_member = 0;
  while (off < ctx->file_size) {
    if (off % 2) {
      // padding a newline
      assert(ctx->buf[off] == '\n');
      ++off;
    }
    // there is enough space for a file header
    assert(off + FILE_HEADER_LEN <= ctx->file_size);
    struct file_header* hdr = (struct file_header*) (ctx->buf + off);

    // check signature
		CHECK(hdr->signature[0] == 0x60, "Invalid signature[0]: 0x%x", hdr->signature[0]);
		CHECK(hdr->signature[1] == 0x0a, "Invalid signature[1]: 0x%x", hdr->signature[1]);
    int memsize = atoi(hdr->file_size);
    int hdr_off = off;
    int payload_off = off + FILE_HEADER_LEN;
    off += FILE_HEADER_LEN + memsize; // skip the header and payload

    // symbol lookup table
    if (memcmp(hdr->file_identifier, "/   ", 4) == 0) {
      arctx_parse_sym_lookup_table(ctx, ctx->buf + payload_off, memsize);
      continue;
    }

    // should check the whole 16 bytes?
    if (memcmp(hdr->file_identifier, "//  ", 4) == 0) {
      // long name list. Usually it's right after the sym file
      ctx->long_name_buf = ctx->buf + payload_off;
      ctx->long_name_buflen = memsize;
      continue;
    }

    int is_elf = _is_elf_member(ctx, payload_off, memsize);
    char *name = arctx_resolve_member_name(ctx, hdr->file_identifier);
    if (name && is_elf) {
      struct elf_member elfmem = elfmem_create(ctx->buf, name, payload_off, memsize);
      vec_append(&ctx->elf_mem_list, &elfmem);
      continue;
    }
    if (name) {
      free(name);
    }

    printf("== Unresolved File %d\n", ++num_unresolved_member);
    printf("    Name '%.16s'\n", hdr->file_identifier);
    printf("    Size %d bytes\n", memsize);
    printf("    is elf: %d\n", is_elf);
    printf("    payload off: 0x%x\n", payload_off);
  }
	CHECK(num_unresolved_member == 0, "Fail parsing the archive since %d members can not be resolved", num_unresolved_member);
  assert(off == ctx->file_size);
  dprintf("Number of elf file %d\n", ctx->elf_mem_list.len);
  if (ctx->sglist.len > 0) {
    arctx_assign_sym_group_to_elf_member(ctx);
  }
  arctx_verify_all_members(ctx);
  arctx_build_symname2memidx(ctx);
}

static void arctx_free(struct arctx* ctx) {
  if (ctx->buf) {
    free(ctx->buf);
  }
  
  VEC_FOREACH(&ctx->elf_mem_list, struct elf_member, elf) {
    elfmem_free(elf);
  }
  vec_free(&ctx->elf_mem_list);

  VEC_FOREACH(&ctx->sglist, struct sym_group, sg) {
    sym_group_free(sg);
  }
  vec_free(&ctx->sglist);
  dict_free(&ctx->symname2memidx);
	dict_free(&ctx->symname2weak);
}

static void arctx_dump(struct arctx* ctx) {
  printf("== BEGIN DUMP ar METADATA ==\n");
  printf("The ar file cotnains %d elf members\n", ctx->elf_mem_list.len);
  VEC_FOREACH(&ctx->elf_mem_list, struct elf_member, elf) {
    elfmem_dump(elf, &ctx->sglist);
  }
  printf("== END DUMP ar METADATA ==\n");
}

// return nULL if not found
static struct elf_member* arctx_get_elfmem_by_name(struct arctx* ctx, const char* mem_name) {
  assert(mem_name);
  // TODO: use hash table
  struct elf_member* found_elf = NULL;
  VEC_FOREACH(&ctx->elf_mem_list, struct elf_member, elf) {
    if (strcmp(mem_name, elf->name) == 0) {
      found_elf = elf;
      break;
    }
  }
  return found_elf;
}

static void arctx_extract_member(struct arctx* ctx, const char* mem_name, const char *outpath) {
  printf("Trying to extracting %s out of the ar file\n", mem_name);
  assert(mem_name);
  struct elf_member* found_elf = arctx_get_elfmem_by_name(ctx, mem_name);
  assert(found_elf);
  char path[1024]; // assume enough

	if (!outpath) {
	  sprintf(path, "%s", mem_name);
	} else {
		struct stat statobj;
		stat(outpath, &statobj);
		if (S_ISDIR(statobj.st_mode)) {
			// directory
			sprintf(path, "%s/%s", outpath, mem_name);
		} else {
			sprintf(path, "%s", outpath);
		}
	}
  FILE* fp = fopen(path, "w");
  assert(fp);
  int status = fwrite(ctx->buf + found_elf->payload_off, 1, found_elf->size, fp);
  assert(status == found_elf->size);
  fclose(fp);
  printf("Successfully extract %s to %s\n", mem_name, path);
}

#include "../old/sar/arctx.h"
