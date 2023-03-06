/*
 * Copied from: https://github.com/zhang314159265/sas/blob/master/util.h
 * TODO: avoid duplicate this file across repos
 */
#pragma once

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define bool int
#define true 1
#define false 0

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

static int gcd(int a, int b) {
  while (b) {
    int r = a % b;
    a = b;
    b = r;
  }
  return a;
}

static int lcd(int a, int b) {
  return a * b / gcd(a, b);
}

static int make_align(int val, int align) {
  assert(align > 0);
  return (val + align - 1) / align * align;
}

static int is_int8(int32_t val) {
  return val >= -128 && val <= 127; 
}

static int hex2int(char ch) {
  ch = tolower(ch);
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  } else if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  } else {
    return -1;
  }
}

static char* lenstrdup(const char* src, int len) {
  char* dst = (char*) malloc(len + 1);
  assert(dst != NULL);
  memcpy(dst, src, len);
  dst[len] = '\0';
  return dst;
}

#include "check.h"

static int lenstrtoi(const char* s, int len) {
  const char* scpy = lenstrdup(s, len);
  char* end = NULL;
  int ret = strtol(scpy, &end, 10);
  CHECK(*end == '\0', "Invalid integer string: %s", scpy);
  free((void*) scpy);
  return ret;
}

static uint32_t endian_swap(uint32_t val) {
  return ((val >> 24) & 0xff)
         | ((val >> 8) & 0xff00)
         | ((val << 8) & 0xff0000)
         | ((val << 24) & 0xff000000);
}


