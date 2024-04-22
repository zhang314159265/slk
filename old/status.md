A linker implemented in C.

# NOTE
- Run `gcc -v` to see the detailed command called by the driver in each stage (preprocess/compile/assemble/linking)
- ld '-lLIBNAME' search for libLIBNAME.so before libLIBNAME.a . Incorrectly using libLIBNAME.a if libLIBNAME.so exists may cause a segfault.
- `ld --verbose` will show the default linker script; `ld -melf_i386 --verbase` to show the linker script for 32bit linking.
- `__libc_start_main` caled by `_start` will call main. It's defined in libc. But `readelf -s libc.so | grep __libc_start_main` may shows up nothing since the symbol name with version string is too long and get truncated. Add `-W` option to readelf solve the problem.
  - It's weird that `nm libc.so` reports no symbols: "nm: /lib/i386-linux-gnu/libc.so.6: no symbols" , but 'readelf -s' works.
- A segfault executable will have 139 (128 + SIGSEGV which is 11) as the exit code. But an executable calling exit(139) will not be recognized by make or gdb as segfault. Not sure the mechanism of segfault detection yet.

# Reference
- [collect2](https://gcc.gnu.org/onlinedocs/gccint/Collect2.html): collect2 is a wrapper to ld.
- [GCC Driver](http://web.cse.ohio-state.edu/~reeves.92/CSE2421au12/SlidesDay51.pdf): gcc is a driver to call preprocessor, compiler, assembler, and linker when needed.
- [ld document](https://sourceware.org/binutils/docs-2.39/ld.html)
- [crt0 - wikipedia][https://en.wikipedia.org/wiki/Crt0]
- [Createing a C Library - osdev](https://wiki.osdev.org/Creating_a_C_Library): referred from crt0 wikipedia page. Talks about crt0, crti, crtn, crtbegin, crtend.
- [ar doc](https://sourceware.org/binutils/docs-2.39/binutils.htm): the doc is very simply and basically only goes thru the command line options.
- [ar - wikipedia](https://en.wikipedia.org/wiki/Ar_(Unix)): explains the format of an ar file.
- [Working with submodules](https://github.blog/2016-02-01-working-with-submodules/)
  - need call 'git submodule update --remote' to fetch new commits for submodule from server.

# Low Prio
- why nm shows no symbols for libc.so but `readelf -s` works

# Plan
- make slk work for sum.o and slibc
- refactor sas and slk to avoid duplicate code
- support got/plt and dynamic linking

# Scratch

- cleanup `runslk_slibc` in old/Makefile 

- get rid of `runld_slibc` since this should be in slibc repo.

- TODO: test slk with an archive file rather than all .o file

- NEXT: cleanup slk.c and move some part of Makefile from old to top level directory ++++++++++
  - add some unit test for sum.

- TODO: move all stuff in slk to old/. And increnemtnally cleanup stuff in old/ to refactor the project.

- NOTE: critical to clean up `elf_file, elf_reader, elf_writer` under old/

- REFACTORING.......
	- `elf_writer` <= replaced by slk but some test still use the old version. Fully drop the old code for `elf_writer` after cleaning up the tests.
  - clean up the dependencies on sar....
  - `elf_file` <=== not used by slk
	```
	  comment in elf_writer.h that's relevant to elf_file
/*
 * TODO consolidate with elf_file.h. The following things need to be resolved
 * 1. elf_file.h rely on arctx
 * 2. elf_file.h only handles relocable file.
 *    while this file only handles executable file
 */
 ```


- check dynamic linking

- DONE: printf.o
  - note that the switch is implemented with a jump table in .rodata section.
    Every entry of the jump table need to be relocated by .text section.
    The number of entries of the jump table is calculated by the literal values
    of all the cases in the switch statement.

- TODO: handle dynamic libraries and pic code (got/plt)

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

- check how linker works when calling 'gcc' to build the sum executable.
  - how does ld create the executable based on sum.o and libraries

` [https://blogs.oracle.com/linux/post/hello-from-a-libc-free-world-part-1] `, referred from crt0 wikipedia page. Looks reasonable.

- check branch WIP-manually-flatten-static-lib under slk
