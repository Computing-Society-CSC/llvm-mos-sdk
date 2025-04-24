#ifndef SCREEN_H
#define SCREEN_H

// Screen buffer control
#define SCREEN_BUF_SIZE 64*32
#define SCREEN_BUF_BASE 0x4000

#define REFRESH_SCREEN_BUF 0xfff3
#define CLEAR_SCREEN_BUF 0xfff2

__attribute__((always_inline)) inline void screen_refresh() {
  *(volatile char *)REFRESH_SCREEN_BUF = 0;
}

__attribute__((always_inline)) inline void screen_clear() {
  *(volatile char *)CLEAR_SCREEN_BUF = 0;
}

__attribute__((always_inline)) inline void screen_set_pixel(unsigned char x, unsigned char y, unsigned char value) {
    // Calculate byte address
    unsigned short addr = SCREEN_BUF_BASE + (y * 64) + x;
    // Write the byte value
    (*(volatile char*)(addr)) = value; // Use 0 for off, non-zero for on
}

void screen_set_pixel_safe(unsigned char x, unsigned char y, unsigned char value);
void screen_set_pixel_wrap(unsigned char x, unsigned char y, unsigned char value);

void screen_draw_line(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char on);
void screen_draw_rect(unsigned char x, unsigned char y, unsigned char width, unsigned char height, unsigned char fill);

void screen_draw_char(char c, unsigned char x, unsigned char y, unsigned char color);
void screen_draw_string(const char* str, unsigned char x, unsigned char y, unsigned char color);
void screen_draw_number(unsigned int num, unsigned char x, unsigned char y, unsigned char color);
#endif // not SCREEN_H