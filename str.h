/*
 * Copied from: https://github.com/zhang314159265/sas/blob/master/str.h
 * TODO: avoid duplicate this file across repos
 */

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "util.h"

// variable length string
struct str {
  int capacity;
	int len;
	char *buf;
};

static inline struct str str_create(int init_capa) {
	init_capa = max(init_capa, 16); // provide at least 16 capacity
	struct str str;
	str.capacity = init_capa;
	str.len = 0;
  if (str.capacity > 0) {
  	str.buf = (char*) malloc(str.capacity);
  } else {
    str.buf = NULL;
  }
	return str;
}

static inline void str_append(struct str* pstr, char ch) {
	assert(pstr->len <= pstr->capacity);
	if (pstr->len == pstr->capacity) {
		pstr->capacity <<= 1;
		pstr->buf = (char*) realloc(pstr->buf, pstr->capacity);
	}
	pstr->buf[pstr->len++] = ch;
}

static void str_append_i32(struct str* pstr, uint32_t val) {
  // append a 4 byte little endian value
  str_append(pstr, val & 0xff);
  str_append(pstr, (val >> 8) & 0xff);
  str_append(pstr, (val >> 16) & 0xff);
  str_append(pstr, (val >> 24) & 0xff);
}

static inline void str_nappend(struct str* pstr, int n, char ch) {
  for (int i = 0; i < n; ++i) {
    str_append(pstr, ch);
  }
}

static int str_lenconcat(struct str* str, const char* extra, int n) {
  int ret = str->len;
  for (int i = 0; i < n; ++i) {
    str_append(str, extra[i]);
  }
  return ret;
}

/*
 * Concat an cstr (with training '\0') and return the size before concating.
 */
static int str_concat(struct str* str, const char* extra) {
  return str_lenconcat(str, extra, strlen(extra) + 1);
}

static inline void str_hexdump(struct str* pstr) {
	printf("str dump:\n");
	for (int i = 0; i < pstr->len; ++i) {
		printf(" %02x", (unsigned char) pstr->buf[i]);
		if ((i + 1) % 16 == 0) {
			printf("\n");
		}
	}
	printf("\n");
}

// the caller has the responsibility to free the returned pointer
static char* str_to_hex(struct str* pstr) {
  char* buf;
  buf = malloc(pstr->len * 2 + 1);
  char *cur = buf;
  for (int i = 0; i < pstr->len; ++i) {
    sprintf(cur, "%02x", (unsigned char) pstr->buf[i]);
    cur += 2;
  }
  *cur = '\0';
  return buf;
}

static inline void str_free(struct str* pstr) {
  if (pstr->buf) {
	  free(pstr->buf);
    pstr->buf = NULL;
  }
}

// Relocate the dword relative displacement at off to point to symbol.
static inline void str_relocate_off_to_sym(struct str* pstr, int off, int symbol) {
  uint32_t next_instr_addr = (uint32_t) (pstr->buf + off + 4);
  *(uint32_t*) (pstr->buf + off)
    = symbol - next_instr_addr;
}

static struct str str_move(struct str* pstr) {
  struct str ret = *pstr;
  pstr->capacity = 0;
  pstr->len = 0;
  pstr->buf = NULL;
  return ret;
}

/*
 * Return 1 if hexdump of the str matches 'hex' 0 otherwise.
 */
static int str_check(struct str* pstr, const char* hex) {
  char* actual = str_to_hex(pstr);
  int r = strcasecmp(actual, hex);
  free(actual);
  return r == 0;
}

static void str_align(struct str* str, int align) {
  if (align <= 0) {
    return;
  }
  // pad with 0
  int len = str->len;
  int newlen = make_align(len, align);
  str_nappend(str, newlen - len, 0);
}
