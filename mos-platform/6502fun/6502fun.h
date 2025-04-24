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

#define BTN_UP_MASK 0x10
#define BTN_DOWN_MASK 0x80
#define BTN_LEFT_MASK 0x20
#define BTN_RIGHT_MASK 0x40
#define BTN_A_MASK 0x08
#define BTN_B_MASK 0x04
#define BTN_SELECT_MASK 0x01
#define BTN_START_MASK 0x02

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
