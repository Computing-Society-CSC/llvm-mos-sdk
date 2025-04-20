#include "screen.h"
#include <stdint.h> // Ensure standard types are available
#include <stdlib.h> // For abs()

// --- Internal Helper Macros/Functions ---

// Direct byte write to screen buffer, respecting volatile
#define SCREEN_WRITE_BYTE(addr, value) (*(volatile uint8_t*)(addr) = (value))

// Basic min/max helpers
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Internal function to set a pixel with boundary checks
inline static void set_pixel_safe(unsigned char x, unsigned char y, unsigned char value) {
    // Check bounds (0-63 for x, 0-31 for y)
    if (x < 64 && y < 32) {
        uint16_t addr = SCREEN_BUF_BASE + (y * 64) + x;
        SCREEN_WRITE_BYTE(addr, value);
    }
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


// --- Public Function Implementations ---

/**
 * @brief Draws a line using Bresenham's algorithm.
 * @param x1 Starting x-coordinate.
 * @param y1 Starting y-coordinate.
 * @param x2 Ending x-coordinate.
 * @param y2 Ending y-coordinate.
 * @param on Value to draw with (0=off, non-zero=on).
 */
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
        set_pixel_safe(current_x, current_y, on);

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
