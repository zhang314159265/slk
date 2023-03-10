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
- check how ar file works: understand the format and how linker handles it
- why nm shows no symbols for libc.so but `readelf -s` works

# Quest
- be able to use sar to parse libc, libgcc and `libgcc_eh`
  - currently it fails to parse libgcc

# Scratch

- enhancle sar to figure out what are the minimal set of .o files to fullfil the pass in list of symbols.
  - only consider GLOBAL UNDEFINED SYMBOLS as new symbols that need to be fullfilled for now
  - how to handle the case that multiple .o files defines the same symbol (some may be WEAK symbols)

- enhancle arctx:
  // TODO: for each elf_member store the list of symbols it needs and the list of symbols it provides.
  // TODO: add a dict to the ctx to map a symbol to the index of elf_member that defines it.

- TODO: move the verificatoin to be an essential step of ar file parsing and remove itst reciple from Makefile

- TODO: do 3 ways of flatten static lib in linking
  - fully flatten [DONE]
  - automatic flatten <== TODO HERE, find what's the min list of .o files.
  - manual flatten (may skip if automatic flatten shows there are too many items after flattening)

- TODO: create my own simple libc (to provide printf and `__libc_start_main`)

- TODO: after creating my own simple libc, create my own crt object files.

- TODO: fully understand `make runld` with `USE_STATIC=1`

- TODO:
  - create a example using ctor/dtor to try out crti/crtn/crtbegin/crtend

- TODO: create a example .s/.o that uses libgcc

- check dynamic linking

- check how linker works when calling 'gcc' to build the sum executable.
  - how does ld create the executable based on sum.o and libraries

` [https://blogs.oracle.com/linux/post/hello-from-a-libc-free-world-part-1] `, referred from crt0 wikipedia page. Looks reasonable.
