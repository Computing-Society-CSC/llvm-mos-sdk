#include <stdio.h>

#define PUT_CHAR 0xfff1

__attribute__((always_inline, weak)) int
__from_ascii(char c, void *ctx, int (*write)(char c, void *ctx)) {
  if (__builtin_expect(c == '\n', 0))
    if (write('\r', ctx) == EOF)
      return EOF;
  return write(c, ctx);
}

__attribute__((always_inline)) inline void __putchar(char c) {
    volatile char *address = (volatile char *)PUT_CHAR;
    *address = c;
}

// no implementation
int __getchar(void) { return 0; }