#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>
#include "sym_group.h"
#include "util.h"
#include "vec.h"
#include "elf_member.h"

#define SIGNATURE "!<arch>\n"
#define SIGNATURE_LEN 8
#define FILE_HEADER_LEN 60

struct arctx {
  int file_size;
  FILE* fp;
  char* buf; // to simply the implementation we load the whole file into buf
  struct vec elf_mem_list;

  char *long_name_buf; // point somewhere in buf
  int long_name_buflen;
  struct vec sglist; // a list of symbol groups
};

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

/*
 * Guarantee to read len bytes fomr file to buf.
 */
void read_file(FILE* fp, char* buf, int len) {
  int status = fread(buf, 1, len, fp);
  assert(status == len);
}

struct arctx ctx_create(const char* path) {
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
  read_file(ctx.fp, ctx.buf, ctx.file_size);

  assert(strncmp(ctx.buf, SIGNATURE, 8) == 0);
  assert(sizeof(struct file_header) == FILE_HEADER_LEN); // TODO use static_assert

  ctx.elf_mem_list = vec_create(sizeof(struct elf_member));
  ctx.sglist = vec_create(sizeof(struct sym_group));
  return ctx;
}

int is_elf_member(struct arctx* ctx, int off, int size) {
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
char *ar_resolve_long_name(struct arctx* ctx, char* file_identifier) {
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
 * Return NULL if no able to fully resolve the name for the member.
 * Otherwise return the resolve name. Caller is responsible to free the memory.
 */
char* ar_resolve_name(struct arctx* ctx, char *file_identifier) {
  if (file_identifier[0] == '/' && isdigit(file_identifier[1])) {
    // long name
    return ar_resolve_long_name(ctx, file_identifier);
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

uint32_t endian_swap(uint32_t val) {
  return ((val >> 24) & 0xff)
         | ((val >> 8) & 0xff00)
         | ((val << 8) & 0xff0000)
         | ((val << 24) & 0xff000000);
}

void parse_sym_lookup_table(struct arctx* ctx, const char* payload, int size) {
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

  assert(off == size);
  free(loclist);
  free(namelist);

  printf("Found %d symbol groups\n", ctx->sglist.len);
}

// TODO: we should use a hash table
void assign_sym_group_to_elf_file(struct arctx* ctx) {
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

void parse_ar_file(struct arctx* ctx) {
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
      parse_sym_lookup_table(ctx, ctx->buf + payload_off, memsize);
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
    char *name = ar_resolve_name(ctx, hdr->file_identifier);
    if (name && is_elf) {
      struct elf_member elfmem = elfmem_create(name, payload_off, memsize);
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
    assign_sym_group_to_elf_file(ctx);
  }
  printf("PASS PARSING!\n");
}

void ctx_dump(struct arctx* ctx) {
  printf("== BEGIN DUMP ar METADATA ==\n");
  printf("The ar file cotnains %d elf members\n", ctx->elf_mem_list.len);
  VEC_FOREACH(&ctx->elf_mem_list, struct elf_member, elf) {
    elfmem_dump(elf, &ctx->sglist);
  }
  printf("== END DUMP ar METADATA ==\n");
}

void ctx_free(struct arctx* ctx) {
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
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: sar <path>\n");
    exit(1);
  }
  const char* ar_path = argv[1];
  struct arctx ctx = ctx_create(ar_path);
  parse_ar_file(&ctx);
  ctx_dump(&ctx);
  ctx_free(&ctx);
  printf("bye\n");
  return 0;
}