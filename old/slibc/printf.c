/*
 * Copied from sos.
 */
#include "stdarg.h"
#include "stdint.h"

typedef void putchar_fn_t(char ch);

// 64 bit print does not work yet. They depends on symbols like:
// `__udivdi3' and `__umoddi3'
// void printnum_unsigned(uint64_t num, int base, putchar_fn_t* putchar_fn) {
void printnum_unsigned(uint32_t num, int base, putchar_fn_t* putchar_fn) {
  static char digits[] = "0123456789abcdef";
  // handle single digit
  if (num >= 0 && num < base) {
    putchar_fn(digits[num]);
    return;
  }
  // handle multi digit
  printnum_unsigned(num / base, base, putchar_fn);
  putchar_fn(digits[num % base]);
}

void printnum(int num, int base, putchar_fn_t* putchar_fn) {
   // handle negative number
  if (num < 0) {
    putchar_fn('-');
    printnum_unsigned(-num, base, putchar_fn);
  } else {
    printnum_unsigned(num, base, putchar_fn);
  }
}

// internal implementation of vprintf
int vprintf_int(const char*fmt, va_list va, putchar_fn_t* putchar_fn) {
  int percent_mode = 0;
  for (const char* cp = fmt; *cp; ++cp) {
    char ch = *cp;
    if (ch == '%') {
      if (percent_mode) {
        // in "%%" the first percent escape the second one and result in a literal '%'
        putchar_fn(ch);
      }
      percent_mode = !percent_mode;
      continue;
    }

    if (percent_mode) {
      // TODO support other formattings
      switch (ch) {
      case 'd': {
        int num = va_arg(va, int);
        printnum(num, 10, putchar_fn);
        percent_mode = 0;
        goto next;
      }
      case 'p':
        putchar_fn('0');
        putchar_fn('x');
        // fall thru
      case 'x': {
        unsigned int num = va_arg(va, unsigned int);
        printnum_unsigned(num, 16, putchar_fn);
        percent_mode = 0;
        goto next;
      }
      case 's': {
        const char *str = va_arg(va, const char*);
        while (*str) {
          putchar_fn(*str++);
        }
        percent_mode = 0;
        goto next;
      }
      case 'c': {
        char arg_ch = va_arg(va, char);
        putchar_fn(arg_ch);
        percent_mode = 0;
        goto next;
      }
      #if 0
      // 64 bit print does not work yet. They depends on symbols like:
      // `__udivdi3' and `__umoddi3'
      case 'l': {
        if (*(cp + 1) == 'l' && *(cp + 2) == 'x') {
          cp += 2; // not adding 3 since there will be a ++cp in the end of this iteration
          uint64_t num = va_arg(va, uint64_t);
          printnum_unsigned(num, 16, putchar_fn);
          percent_mode = 0;
          goto next;
        }
        break;
      }
      #endif
      default:
        break;
      }
    }

    putchar_fn(ch);
next:;
  }
  // should not be in percent mode here
  return 0; // TODO return value!
}
