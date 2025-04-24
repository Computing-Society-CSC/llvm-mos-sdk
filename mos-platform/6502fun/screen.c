#include "screen.h"
#include <stdint.h> // Ensure standard types are available

typedef unsigned short blit16_glyph;

// ascii map starting at code 32
blit16_glyph blit16_Glyphs[95] = {
0x0000,0x2092,0x002d,0x5f7d,0x279e,0x52a5,0x7ad6,0x0012,
0x4494,0x1491,0x017a,0x05d0,0x1400,0x01c0,0x0400,0x12a4,
0x2b6a,0x749a,0x752a,0x38a3,0x4f4a,0x38cf,0x3bce,0x12a7,
0x3aae,0x49ae,0x0410,0x1410,0x4454,0x0e38,0x1511,0x10e3,
0x73ee,0x5f7a,0x3beb,0x624e,0x3b6b,0x73cf,0x13cf,0x6b4e,
0x5bed,0x7497,0x2b27,0x5add,0x7249,0x5b7d,0x5b6b,0x3b6e,
0x12eb,0x4f6b,0x5aeb,0x388e,0x2497,0x6b6d,0x256d,0x5f6d,
0x5aad,0x24ad,0x72a7,0x6496,0x4889,0x3493,0x002a,0xf000,
0x0011,0x6b98,0x3b79,0x7270,0x7b74,0x6750,0x95d6,0xb9ee,
0x5b59,0x6410,0xb482,0x56e8,0x6492,0x5be8,0x5b58,0x3b70,
0x976a,0xcd6a,0x1370,0x38f0,0x64ba,0x3b68,0x2568,0x5f68,
0x54a8,0xb9ad,0x73b8,0x64d6,0x2492,0x3593,0x03e0,
};

// --- Internal Helper Macros/Functions ---

// Direct byte write to screen buffer, respecting volatile
#define SCREEN_WRITE_BYTE(addr, value) (*(volatile uint8_t*)(addr) = (value))

// Basic min/max helpers
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// function to set a pixel with boundary checks
inline void screen_set_pixel_safe(unsigned char x, unsigned char y, unsigned char value) {
    // Check bounds (0-63 for x, 0-31 for y)
    if (x < 64 && y < 32) {
        uint16_t addr = SCREEN_BUF_BASE + (y * 64) + x;
        SCREEN_WRITE_BYTE(addr, value);
    }
}
inline void screen_set_pixel_wrap(unsigned char x, unsigned char y, unsigned char value) {
    // Check bounds (0-63 for x, 0-31 for y)
    x = x % 64;
    y = y % 32;
    uint16_t addr = SCREEN_BUF_BASE + (y * 64) + x;
    SCREEN_WRITE_BYTE(addr, value);
}

// Internal function to draw a horizontal line (used by rect, circle fill)
static void draw_hline(unsigned char x1, unsigned char x2, unsigned char y, unsigned char value) {
    if (y >= 32) return; // Out of bounds vertically
    unsigned char start_x = MIN(x1, x2);
    unsigned char end_x = MAX(x1, x2);
    if (start_x >= 64) return; // Wholly out of bounds horizontally
    // Clip end_x if it goes out of bounds
    if (end_x >= 64) {
        end_x = 63;
    }
    uint16_t base_addr = SCREEN_BUF_BASE + (y * 64);
    for (unsigned char x = start_x; x <= end_x; ++x) {
        SCREEN_WRITE_BYTE(base_addr + x, value);
    }
}

// Internal function to draw a vertical line (used by rect outline)
static void draw_vline(unsigned char x, unsigned char y1, unsigned char y2, unsigned char value) {
    if (x >= 64) return; // Out of bounds horizontally
    unsigned char start_y = MIN(y1, y2);
    unsigned char end_y = MAX(y1, y2);
    if (start_y >= 32) return; // Wholly out of bounds vertically
    // Clip end_y if it goes out of bounds
    if (end_y >= 32) {
        end_y = 31;
    }
    for (unsigned char y = start_y; y <= end_y; ++y) {
        uint16_t addr = SCREEN_BUF_BASE + (y * 64) + x;
        SCREEN_WRITE_BYTE(addr, value);
    }
}

// Custom unsigned integer to ASCII conversion
// Writes the string representation of 'num' into 'buffer'
// Returns a pointer to the beginning of the generated string within the buffer.
static char* utoa_custom(unsigned int num, char *buffer, int base) {
    int i = 0;
    // Handle 0 explicitly, otherwise empty string is printed for 0
    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return buffer;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        buffer[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }

    // Reverse the string
    for (int j = 0; j < i/2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i-j-1];
        buffer[i-j-1] = temp;
    }

    buffer[i] = '\0'; // Null-terminate string

    return buffer;
}

// --- Public Function Implementations ---

/**
 * @brief Draws a line using Bresenham's algorithm.
 * @param x1 Starting x-coordinate.
 * @param y1 Starting y-coordinate.
 * @param x2 Ending x-coordinate.
 * @param y2 Ending y-coordinate.
 * @param on Value to draw with (0=off, non-zero=on).
 */
int abs(int x) {
    return (x < 0) ? -x : x;
}

void screen_draw_line(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2, unsigned char on) {
    int dx = abs((int)x2 - (int)x1);
    int dy = abs((int)y2 - (int)y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int e2;
    unsigned char current_x = x1;
    unsigned char current_y = y1;

    while (1) {
        screen_set_pixel_safe(current_x, current_y, on);

        if (current_x == x2 && current_y == y2) {
            break;
        }

        e2 = 2 * err;

        // Check bounds before potential increment/decrement
        // This prevents wrapping around unsigned char if we go below 0
        // Note: Bresenham guarantees progress towards the endpoint.

        if (e2 > -dy) {
            if (current_x == x2) break; // Avoid overshooting in case of horizontal/vertical lines
            err -= dy;
            current_x = (unsigned char)((int)current_x + sx);
        }

        if (e2 < dx) {
             if (current_y == y2) break; // Avoid overshooting
            err += dx;
            current_y = (unsigned char)((int)current_y + sy);
        }
    }
}

/**
 * @brief Draws a rectangle.
 * @param x Top-left x-coordinate.
 * @param y Top-left y-coordinate.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 * @param fill Value to draw with. If 0, draws outline with value 1. If non-zero, fills with that value.
 */
void screen_draw_rect(unsigned char x, unsigned char y, unsigned char width, unsigned char height, unsigned char fill) {
    if (width == 0 || height == 0) return;

    unsigned char x2 = x + width - 1;
    unsigned char y2 = y + height - 1;

    // Basic boundary checks for corners
    if (x >= 64 || y >= 32 || x2 >= 64 || y2 >= 32) {
        // For simplicity, we could clip here, but let's rely on the line/fill functions' checks
        // Or just return if the start is already out of bounds
        if (x >= 64 || y >= 32) return;
    }


    if (fill == 0) { // Draw outline with value 1
        unsigned char outline_val = 1;
        draw_hline(x, x2, y, outline_val);          // Top
        if (height > 1) {
             draw_hline(x, x2, y2, outline_val);     // Bottom (only if height > 1)
        }
        if (height > 2) { // Avoid drawing over corners again if height is 1 or 2
            draw_vline(x, y + 1, y2 - 1, outline_val); // Left
            if (width > 1) {
                draw_vline(x2, y + 1, y2 - 1, outline_val); // Right (only if width > 1)
            }
        } else if (height == 2 && width > 1) { // Special case for height 2
             draw_vline(x, y + 1, y + 1, outline_val); // Left pixel
             draw_vline(x2, y + 1, y + 1, outline_val); // Right pixel
        } else if (height == 1 && width > 1) {
            // Horizontal line already drawn
        } else if (width == 1 && height > 1) {
            draw_vline(x, y+1, y2, outline_val); // Single vertical line
        }

    } else { // Fill rectangle with 'fill' value
        unsigned char current_y;
        // Clip height if necessary
        unsigned char end_y = (y + height > 32) ? 31 : y + height - 1;

        for (current_y = y; current_y <= end_y; ++current_y) {
            // draw_hline handles horizontal clipping
            draw_hline(x, x2, current_y, fill);
        }
    }
}

// Draw a single character using the blit16_Glyphs map (4x4 pixels), correcting flipped axes
void screen_draw_char(char c, unsigned char x, unsigned char y, unsigned char color) {
    if (c < 32 || c > 126) return;
    blit16_glyph glyph_data = blit16_Glyphs[c - 32];
    unsigned char row, col;
    unsigned short bit_mask = 1 << 14;  // start at top‑left bit of the 15‑bit glyph

    for (row = 0; row < 5; ++row) {
        for (col = 0; col < 3; ++col) {
            if (glyph_data & bit_mask) {
                // flip both horizontally and vertically to undo the mirrored output
                screen_set_pixel(x + (2 - col), y + (4 - row), color);
            }
            bit_mask >>= 1;
        }
    }
}

// Draw a null-terminated string using screen_screen_draw_char
void screen_draw_string(const char* str, unsigned char x, unsigned char y, unsigned char color) {
    while (*str) {
        screen_draw_char(*str, x, y, color);
        x += 4; // Move to the next character position (4 pixels width)
        str++;
    }
}

// Draw an unsigned integer number at specified location using screen_draw_string
void screen_draw_number(unsigned int num, unsigned char x, unsigned char y, unsigned char color) {
    char buffer[7]; // Max 5 digits for unsigned int (assuming 16-bit, 65535) + null terminator + 1 for safety
    utoa_custom(num, buffer, 10); // Use custom utoa function (base 10)
    screen_draw_string(buffer, x, y, color); // Draw the string
}