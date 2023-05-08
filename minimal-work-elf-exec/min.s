.globl _start
_start:
  mov $1, %eax # syscall number
  mov $3, %ebx
  int $0x80
