
- cleanup everything else in old/
- check branch WIP-manually-flatten-static-lib under slk


- TODO: do 3 ways of flatten static lib in linking
  - fully flatten [DONE]
  - automatic flatten. find what's the min list of .o files. [DONE]
  - manual flatten (may skip if automatic flatten shows there are too many items after flattening)

- make slk work with glibc for sum after cleaning up 

- DONE: printf.o
  - note that the switch is implemented with a jump table in .rodata section.
    Every entry of the jump table need to be relocated by .text section.
    The number of entries of the jump table is calculated by the literal values
    of all the cases in the switch statement.

- check dynamic linking
- TODO: handle dynamic libraries and pic code (got/plt)

- TODO: should I remove the code that manually figure out the min set of .o? <++ LOW-PRIO
- TODO: ld + min .o set result in a 755K a.out. Is this a general problem? We can improve linker to let the out a.out file much smaller.

- TODO: fully understand `make runld` with `USE_STATIC=1`

- TODO:
  - create a example using ctor/dtor to try out crti/crtn/crtbegin/crtend

- TODO: create a example .s/.o that uses libgcc

- check how linker works when calling 'gcc' to build the sum executable.
  - how does ld create the executable based on sum.o and libraries

` [https://blogs.oracle.com/linux/post/hello-from-a-libc-free-world-part-1] `, referred from crt0 wikipedia page. Looks reasonable.

