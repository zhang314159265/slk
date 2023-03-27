/*
 * Copied from sos
 */

#ifndef STDINT_H
#define STDINT_H

#ifndef __cplusplus
typedef int bool;
#define true 1
#define false 0
#endif

// #include <assert.h>

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int int16_t;
typedef unsigned short int uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

#if 0
static_assert(sizeof(int8_t) == 1);
static_assert(sizeof(uint8_t) == 1, "Check size of uint8_t");
static_assert(sizeof(int16_t) == 2);
static_assert(sizeof(uint16_t) == 2);
static_assert(sizeof(int32_t) == 4);
static_assert(sizeof(uint32_t) == 4);
static_assert(sizeof(int64_t) == 8);
static_assert(sizeof(uint64_t) == 8);
#endif

#ifndef NULL
#define NULL ((void*) 0)
#endif

#endif
