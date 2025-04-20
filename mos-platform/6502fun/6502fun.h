#ifndef _6502fun_H_
#define _6502fun_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <via.h>
#include <screen.h>

#define PEEK(addr) (*(volatile uint8_t*)(addr))
#define POKE(addr, val) (*(volatile uint8_t*)(addr) = (val))

#define GET_RAND 0xfff0

__attribute__((always_inline)) inline unsigned char get_rand() {
    return *(unsigned char *)GET_RAND;
}

// Get the value of the system millisecond tick counter.
unsigned long millis(void);

// Wait for a specific number of milliseconds.
void delay(unsigned ms);

#ifdef __cplusplus
}
#endif

#endif // not _6502fun_H_
