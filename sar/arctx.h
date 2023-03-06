#pragma once

#include "dict.h"
#include <sys/stat.h>

#define SIGNATURE "!<arch>\n"
#define SIGNATURE_LEN 8
#define FILE_HEADER_LEN 60

struct file_header {
  char file_identifier[16];
  char mod_ts[12]; // file modification timestamp
  char owner_id[6];
  char group_id[6];
  char file_mod[8];
  char file_size[10]; // file size in bytes
  char signature[2];
} __attribute__((packed));

#define FILE_IDENTIFIER_LEN sizeof(((struct file_header*) 0)->file_identifier)

struct arctx {
  int file_size;
  FILE* fp;
  char* buf; // to simply the implementation we load the whole file into buf
  struct vec elf_mem_list;

  char *long_name_buf; // point somewhere in buf
  int long_name_buflen;
  struct vec sglist; // a list of symbol groups

  // symbol to the index of member defining it.
  struct dict symname2memidx;
};

/*
 * Guarantee to read len bytes fomr file to buf.
 */
static void _read_file(FILE* fp, char* buf, int len) {
  int status = fread(buf, 1, len, fp);
  assert(status == len);
}

static void arctx_parse_sym_lookup_table(struct arctx* ctx, const char* payload, int size) {
  int off = 0;
  assert(off + 4 <= size);
  int nentry = *(uint32_t*) (payload + off);
  // the number of entry is a 32 bit big endian number. Thus we need
  // convert it to little endian.
  nentry = endian_swap(nentry);

  off += 4;
  printf("Symbol lookup table contains %d entries\n", nentry);
  assert(nentry >= 0);
  uint32_t *loclist = malloc(nentry * sizeof(uint32_t));
  // namelist[i] does not own data. It points somewhere in payload buffer
  const char** namelist = malloc(nentry * sizeof(char*));

  // parse the loc list
  for (int i = 0; i < nentry; ++i) {
    assert(off + 4 <= size);
    loclist[i] = endian_swap(
      *(uint32_t*) (payload + off)
    );
    off += 4;
  }

  // parse the name list
  for (int i = 0; i < nentry; ++i) {
    int j = off;
    while (j < size && payload[j]) {
      ++j;
    }
    assert(j < size);
    namelist[i] = payload + off;
    off = j + 1;
  }

  for (int i = 0; i < nentry; ++i) {
    sglist_add_entry(&ctx->sglist, loclist[i], namelist[i]);
  }

  // there may be padding bytes after the last name. Assume there is at most 1 padding
  // byte and the byte should be '\0'. Revise if it's not true.
  if (off < size) {
    assert(off % 2); // padding only happens if off is odd
    assert(payload[off] == '\0');
    ++off;
  }

  CHECK(off == size, "Fail to parse symbol lookup table off %d v.s. size %d", off, size);
  free(loclist);
  free(namelist);

  printf("Found %d symbol groups\n", ctx->sglist.len);
}

static int is_elf_member(struct arctx* ctx, int off, int size) {
  return size >= 4 /* magic number */
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
  // TODO: do more validation
  int off = atoi(file_identifier + 1);
  if (off >= ctx->long_name_buflen) {
    return NULL;
  }
 
  int idx = off;
  // find the next '/'
  for (; idx < ctx->long_name_buflen && ctx->long_name_buf[idx] != '/'; ++idx) {
  }
  // should follow a '\n'
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

static void arctx_dump(struct arctx* ctx) {
  printf("== BEGIN DUMP ar METADATA ==\n");
  printf("The ar file cotnains %d elf members\n", ctx->elf_mem_list.len);
  VEC_FOREACH(&ctx->elf_mem_list, struct elf_member, elf) {
    elfmem_dump(elf, &ctx->sglist);
  }
  printf("== END DUMP ar METADATA ==\n");
}

static void arctx_free(struct arctx* ctx) {
  fclose(ctx->fp);
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

static void arctx_extract_member(struct arctx* ctx, const char* mem_name) {
  printf("Trying to extracting %s out of the ar file\n", mem_name);
  assert(mem_name);
  struct elf_member* found_elf = arctx_get_elfmem_by_name(ctx, mem_name);
  assert(found_elf);
  char path[1024]; // assume enough
  sprintf(path, "../artifact/%s", mem_name); // hardcode path for now
  FILE* fp = fopen(path, "w");
  assert(fp);
  int status = fwrite(ctx->buf + found_elf->off, 1, found_elf->size, fp);
  assert(status == found_elf->size);
  fclose(fp);
  printf("Successfully extract %s\n", mem_name);
}

static void arctx_verify_member_by_name(struct arctx* ctx, const char* mem_name) {
  printf("Verify member %s\n", mem_name);
  struct elf_member* elfmem = arctx_get_elfmem_by_name(ctx, mem_name);
  elfmem_verify(elfmem, ctx);
}

static void arctx_verify_all_members(struct arctx* ctx) {
  int idx = 0;
  int tot = ctx->elf_mem_list.len;
  VEC_FOREACH(&ctx->elf_mem_list, struct elf_member, elfmem) {
    #if 0
    printf("Verifying %d/%d member...\n", idx++, tot);
    #endif
    elfmem_verify(elfmem, ctx);
  }
  printf("Pass verifying all members in the .a file\n");
}

// TODO: we should use a hash table
static void arctx_assign_sym_group_to_elf_file(struct arctx* ctx) {
  assert(ctx->sglist.len > 0);
  assert(ctx->elf_mem_list.len > 0);
  // it's possible that some elf file does not have a symbol group
  assert(ctx->sglist.len <= ctx->elf_mem_list.len);

  VEC_FOREACH_I(&ctx->sglist, struct sym_group, sg, i) {
    struct elf_member* found_elf = NULL;
    VEC_FOREACH_I(&ctx->elf_mem_list, struct elf_member, elf, j) {
      if (sg->header_off == elf->off - FILE_HEADER_LEN) {
        // found
        elf->sgidx = i;
        found_elf = elf;
        break;
      }
    }
    assert(found_elf);
  }
}

static void arctx_build_symname2memidx(struct arctx* ctx) {
  ctx->symname2memidx = dict_create(); 
  VEC_FOREACH_I(&ctx->elf_mem_list, struct elf_member, memptr, i) {
    VEC_FOREACH(&memptr->provide_syms, char*, pstr) {
      // add the mapping between *pstr and i to symname2memidx
      struct dict_entry* pentry = dict_lookup(&ctx->symname2memidx, *pstr);
      if (pentry->key) {
        struct elf_member* prevmemptr = vec_get_item(&ctx->elf_mem_list, pentry->val);
        // the previous definition wins. Should that be fine?
        //
        // Too many entrys like __x86.get_pc_thunk.ax, ignore them.
        if (!startswith(pentry->key, "__x86.get_pc_thunk")) {
          printf("\033[33mSymbol %s redefined, %s > %s\033[0m\n", pentry->key, prevmemptr->name, memptr->name);
        }
        continue;
      }
      dict_put_to_entry(&ctx->symname2memidx, pentry, *pstr, i);
    }
  }
}

/*
 * parse the ar file.
 */
static void arctx_parse(struct arctx* ctx) {
  assert(ctx->file_size >= SIGNATURE_LEN + FILE_HEADER_LEN);
  int off = SIGNATURE_LEN;

  int idx = 0;
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
    assert(hdr->signature[0] == 0x60); // TODO use CHECK macro as in sas
    assert(hdr->signature[1] == 0x0a);
    int memsize = atoi(hdr->file_size); // TODO check for error case
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

    int is_elf = is_elf_member(ctx, payload_off, memsize);
    char *name = arctx_resolve_member_name(ctx, hdr->file_identifier);
    if (name && is_elf) {
      struct elf_member elfmem = elfmem_create(ctx, name, payload_off, memsize);
      vec_append(&ctx->elf_mem_list, &elfmem);
      continue;
    }
    if (name) {
      free(name);
    }

    printf("== Unresolved File %d\n", ++idx);
    printf("    Name '%.16s'\n", hdr->file_identifier);
    printf("    Size %d bytes\n", memsize);
    printf("    is elf: %d\n", is_elf);
    printf("    payload off: 0x%x\n", payload_off);
  }
  assert(off == ctx->file_size);
  printf("Number of elf file %d\n", ctx->elf_mem_list.len);
  if (ctx->sglist.len > 0) {
    arctx_assign_sym_group_to_elf_file(ctx);
  }
  printf("PASS PARSING!\n");
  arctx_verify_all_members(ctx);
  arctx_build_symname2memidx(ctx);
}

static struct arctx arctx_create(const char* path) {
  struct arctx ctx;
  memset(&ctx, 0, sizeof(ctx));
  struct stat ar_st;
  int status = stat(path, &ar_st);
  assert(status == 0);
  printf("File %s size %ld\n", path, ar_st.st_size);

  ctx.file_size = ar_st.st_size;
  ctx.fp = fopen(path, "rb");
  assert(ctx.fp);

  ctx.buf = malloc(ar_st.st_size);
  _read_file(ctx.fp, ctx.buf, ctx.file_size);

  assert(strncmp(ctx.buf, SIGNATURE, 8) == 0);
  assert(sizeof(struct file_header) == FILE_HEADER_LEN); // TODO use static_assert

  ctx.elf_mem_list = vec_create(sizeof(struct elf_member));
  ctx.sglist = vec_create(sizeof(struct sym_group));
  arctx_parse(&ctx);
  return ctx;
}

/*
 * target_syms will be mutated.
 */
static void arctx_resolve_symbols(struct arctx* ctx, struct vec* target_syms, struct vec* predefined_syms) {
  assert(target_syms->len > 0);
  printf("Need resolve the following %d symbols:\n", target_syms->len);
  VEC_FOREACH(target_syms, char*, itemptr) {
    printf(" - %s\n", *itemptr);
  }

  struct dict added_file = dict_create(); // value is not used

  // resolve symbols in stack order
  while (target_syms->len > 0) {
    char* sym = *(char**) vec_pop_item(target_syms);
    // linear scan is fine since predefined_syms usually is quite a small list.
    if (vec_str_find(predefined_syms, sym)) {
      free(sym);
      continue;
    }
    struct dict_entry* entry = dict_lookup(&ctx->symname2memidx, sym);
    if (!entry->key) {
      FAIL("Symbol not found %s\n", sym);
    }
    int elfmem_idx = entry->val;
    struct elf_member* elfmem = vec_get_item(&ctx->elf_mem_list, elfmem_idx);

    if (dict_put(&added_file, elfmem->name, 0) > 0) {
      printf("Add file %s\n", elfmem->name);
      // add the needed symbols
      VEC_FOREACH(&elfmem->need_syms, char*, pstr) {
        char *newsym = strdup(*pstr);
        vec_append(target_syms, &newsym);
        #if 0
        printf(" - need %s\n", newsym);
        #endif
      }
    }
    free(sym);
  }

  const char* path = "/tmp/dep_obj_list";
  printf("Write %d dependencies to %s\n", added_file.size, path);
  FILE* fp = fopen(path, "w");
  DICT_FOREACH(&added_file, entry_ptr) {
    fprintf(fp, "%s\n", entry_ptr->key);
  }
  fclose(fp);
  
  dict_free(&added_file);
}
