A linker implemented in C.

# NOTE
- Run `gcc -v` to see the detailed command called by the driver in each stage (preprocess/compile/assemble/linking)
- ld '-lLIBNAME' search for libLIBNAME.so before libLIBNAME.a . Incorrectly using libLIBNAME.a if libLIBNAME.so exists may cause a segfault.
- `ld --verbose` will show the default linker script; `ld -melf_i386 --verbase` to show the linker script for 32bit linking.
- `__libc_start_main` caled by `_start` will call main. It's defined in libc. But `readelf -s libc.so | grep __libc_start_main` may shows up nothing since the symbol name with version string is too long and get truncated. Add `-W` option to readelf solve the problem.
  - It's weird that `nm libc.so` reports no symbols: "nm: /lib/i386-linux-gnu/libc.so.6: no symbols" , but 'readelf -s' works.

# Reference
- [collect2](https://gcc.gnu.org/onlinedocs/gccint/Collect2.html): collect2 is a wrapper to ld.
- [GCC Driver](http://web.cse.ohio-state.edu/~reeves.92/CSE2421au12/SlidesDay51.pdf): gcc is a driver to call preprocessor, compiler, assembler, and linker when needed.
- [ld document](https://sourceware.org/binutils/docs-2.39/ld.html)
- [crt0 - wikipedia][https://en.wikipedia.org/wiki/Crt0]
- [Createing a C Library - osdev](https://wiki.osdev.org/Creating_a_C_Library): referred from crt0 wikipedia page. Talks about crt0, crti, crtn, crtbegin, crtend.
- [ar doc](https://sourceware.org/binutils/docs-2.39/binutils.htm): the doc is very simply and basically only goes thru the command line options.
- [ar - wikipedia](https://en.wikipedia.org/wiki/Ar_(Unix)): explains the format of an ar file.

# Low Prio
- why nm shows no symbols for libc.so but `readelf -s` works

# Quest

# Scratch

- TODO: make slk work for sum.o and slibc
  - combine .text/.data/.tss from all elf readers
    - respect alignment
    - decide a starting address
  - do relocation
  - write out an executable elf file.
    - write a trivial executable elf file with hardcoded content. <+++++++ TODO HERE


- TODO: should I remove the code that manually figure out the min set of .o? <++ LOW-PRIO
- TODO: ld + min .o set result in a 755K a.out. Is this a general problem? We can improve linker to let the out a.out file much smaller.

- TODO: do 3 ways of flatten static lib in linking
  - fully flatten [DONE]
  - automatic flatten. find what's the min list of .o files. [DONE]
  - manual flatten (may skip if automatic flatten shows there are too many items after flattening)

- TODO: fully understand `make runld` with `USE_STATIC=1`

- TODO:
  - create a example using ctor/dtor to try out crti/crtn/crtbegin/crtend

- TODO: create a example .s/.o that uses libgcc

- check dynamic linking

- check how linker works when calling 'gcc' to build the sum executable.
  - how does ld create the executable based on sum.o and libraries

` [https://blogs.oracle.com/linux/post/hello-from-a-libc-free-world-part-1] `, referred from crt0 wikipedia page. Looks reasonable.
