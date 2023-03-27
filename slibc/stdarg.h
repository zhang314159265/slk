/*
 * Copied from sos.
 */
#ifndef STDARG_H
#define STDARG_H

#include "stdlib.h"

// looks like varargs.h is replaced by stdarg.h: https://stackoverflow.com/questions/24950362/gcc-no-longer-implements-varargs-h

#define va_list void*

// need revise for x64
#define STACK_ENTRY_SIZE 4

// assumes anchor can be placed inside a single stack entry.
#define va_start(va, anchor) va = (void*) &anchor + STACK_ENTRY_SIZE

// This macro is implemented with statement-expression (https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html)
#define va_arg(va, type) ({ type ret; ret = *((type*) va); va += ROUND_UP(sizeof(type), STACK_ENTRY_SIZE); ret; })

#endif
