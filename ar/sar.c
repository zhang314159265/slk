#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>

#define SIGNATURE "!<arch>\n"
#define SIGNATURE_LEN 8
#define FILE_HEADER_LEN 60

struct sar_ctx {
  int file_size;
  FILE* fp;
  char* buf; // to simply the implementation we load the whole file into buf
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

/*
 * Guarantee to read len bytes fomr file to buf.
 */
void read_file(FILE* fp, char* buf, int len) {
  int status = fread(buf, 1, len, fp);
  assert(status == len);
}

struct sar_ctx ctx_create(const char* path) {
  struct sar_ctx ctx;
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
  return ctx;
}

void parse_ar_file(struct sar_ctx* ctx) {
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
    // printf("signatue 0x%x 0x%x\n", hdr->signature[0], hdr->signature[1]);
    assert(hdr->signature[0] == 0x60); // TODO use CHECK macro as in sas
    assert(hdr->signature[1] == 0x0a);

    printf("== File %d\n", idx);
    printf("   Name '%.16s'\n", hdr->file_identifier);
    int memsize = atoi(hdr->file_size); // TODO check for error case
    printf("   Size %d bytes\n", memsize);
    ++idx;

    // skip the header
    off += FILE_HEADER_LEN;
    // skip the content
    off += memsize;
  }
  assert(off == ctx->file_size);
  printf("PASS PARSING!\n");
}

void ctx_free(struct sar_ctx* ctx) {
  fclose(ctx->fp);
  if (ctx->buf) {
    free(ctx->buf);
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: sar <path>\n");
    exit(1);
  }
  const char* ar_path = argv[1];
  struct sar_ctx ctx = ctx_create(ar_path);
  parse_ar_file(&ctx);
  ctx_free(&ctx);
  printf("bye\n");
  return 0;
}
