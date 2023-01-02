Command to disassemble Scrt1.o with relocation info: `objdump -dr Scrt1.o`

# Annotated disassembly of Scrt1.o

- NOTE: this disassemly used GOT and PLT, it's probably easier to try an example using static linking first.

```
   0:   f3 0f 1e fb             endbr32

   # mark end of backtrace frames
   4:   31 ed                   xor    %ebp,%ebp

   # %esi now stores argc
   6:   5e                      pop    %esi

   # %ecx now stores argv
   7:   89 e1                   mov    %esp,%ecx

   # make %esp 16 bytes align
   9:   83 e4 f0                and    $0xfffffff0,%esp

   # push a garbage so we push 8 works in total and maintain 16 bytes alignment
   c:   50                      push   %eax

   # stack end
   d:   54                      push   %esp

   # rtld_fini
   e:   52                      push   %edx

   f:   e8 18 00 00 00          call   2c <_start+0x2c>
  14:   81 c3 02 00 00 00       add    $0x2,%ebx
                        16: R_386_GOTPC _GLOBAL_OFFSET_TABLE_

  # fini
  1a:   6a 00                   push   $0x0
  # init
  1c:   6a 00                   push   $0x0

  # argv
  1e:   51                      push   %ecx

  # argc
  1f:   56                      push   %esi
  20:   ff b3 00 00 00 00       push   0x0(%ebx)
                        22: R_386_GOT32 main
  26:   e8 fc ff ff ff          call   27 <_start+0x27>
                        27: R_386_PLT32 __libc_start_main
  2b:   f4                      hlt

  # a tiny helper function to return eip in %ebx
  2c:   8b 1c 24                mov    (%esp),%ebx
  2f:   c3                      ret
```

# Reference
- [Linux x86 Program Start Up](http://dbp-consulting.com/tutorials/debugging/linuxProgramStartup.html): referred from crt0 wikipedia page. It explains pretty well the work of `_start` especially the setup of stack for `__libc_start_main`
