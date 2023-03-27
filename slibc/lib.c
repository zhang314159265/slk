#include "stdarg.h"

#define PH 0
int syscall(int no, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5);

asm(R"ASM(
.global syscall
syscall:
  push %ebx
  push %ecx
  push %edx
  push %esi
  push %edi
  push %ebp

  mov 28(%esp), %eax
  mov 32(%esp), %ebx
  mov 36(%esp), %ecx
  mov 40(%esp), %edx
  mov 44(%esp), %esi
  mov 48(%esp), %edi
  mov 52(%esp), %ebp
  int $0x80

  pop %ebp
  pop %edi
  pop %esi
  pop %edx
  pop %ecx
  pop %ebx
  ret
)ASM");

#define SYSCALL1(no, arg) syscall(no, arg, PH, PH, PH, PH, PH)
#define SYSCALL3(no, arg0, arg1, arg2) syscall(no, arg0, arg1, arg2, PH, PH, PH)

#define SYS_exit 1
#define SYS_write 4

void dead_loop() {
  while (1) {
  }
}

int strlen(const char* fmt) {
  int len = 0;
  while (*fmt++) {
    ++len;
  }
  return len;
}

char printf_buf[256]; // TODO: hopefully this is enough
int printf_buflen = 0;

void myexit(int code) {
  SYSCALL1(SYS_exit, code);
}

void putchar_fn(char ch) {
  if (printf_buflen >= sizeof(printf_buf)) {
    // overflow
    myexit(-1);
  }
  printf_buf[printf_buflen++] = ch;
}

typedef void putchar_fn_t(char ch);
int vprintf_int(const char*fmt, va_list va, putchar_fn_t* putchar_fn);
int printf(const char* fmt, ...) {
  printf_buflen = 0;
  va_list va;
  va_start(va, fmt);
  vprintf_int(fmt, va, putchar_fn);
  SYSCALL3(SYS_write, 1, (int) printf_buf, printf_buflen);
  return printf_buflen;
}

int main(int argc, char**argv);
void __libc_start_main(void) {
  // TODO setup argc, argv
  int ret = main(0, 0);
  SYSCALL1(SYS_exit, ret);
}
