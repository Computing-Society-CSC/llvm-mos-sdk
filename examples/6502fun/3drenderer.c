#include <stdio.h>
#include <6502fun.h>
#include <stdlib.h> // For abs() if needed, though we might avoid it

// --- Configuration ---
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCREEN_CENTER_X (SCREEN_WIDTH / 2)
#define SCREEN_CENTER_Y (SCREEN_HEIGHT / 2)

// --- Fixed Point Math ---
typedef signed short fixed; // Using 16-bit signed integers for fixed point
#define FIXED_SHIFT 8       // 8 bits for fractional part
#define SCALE (1 << FIXED_SHIFT) // Scale factor = 256

// Convert integer to fixed point
#define INT_TO_FIXED(x) ((fixed)(x) << FIXED_SHIFT)
// Convert fixed point to integer (truncating)
#define FIXED_TO_INT(x) ((signed short)((x) >> FIXED_SHIFT))
// Multiply two fixed point numbers
// Use long long temporarily to prevent overflow during multiplication before shifting
#define FIXED_MUL(a, b) ((fixed)(((long long)(a) * (long long)(b)) >> FIXED_SHIFT))

// --- Sine/Cosine Lookup Table (for 0-255 angle representation) ---
// Stores sin(angle * 2*PI / 256) * SCALE
// Only store 0-90 degrees (0-64 in our 0-255 range)
#define SIN_TABLE_SIZE 65 // 0 to 64 inclusive
const fixed sin_table[SIN_TABLE_SIZE] = {
0,   6,  13,  19,  25,  31,  38,  44,  50,  56,  62,  68,  74,  80,  86,  92,  // 0-15
    98, 103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176, // 16-31
   181, 185, 189, 193, 197, 201, 205, 209, 213, 216, 220, 223, 227, 230, 233, 236, // 32-47
   239, 241, 244, 246, 248, 250, 252, 253, 255, 256, 257, 258, 259, 260, 260, 261, // 48-63
   256 // index 64 (sin 90) - Corrected: sin(90) = 1 -> 256
};

// Get sine value for angle 0-255
fixed fixed_sin(unsigned char angle) {
    unsigned char quadrant = angle >> 6; // angle / 64
    unsigned char index = angle & 63;    // angle % 64

    switch (quadrant) {
        case 0: // 0-63 (0-89 deg)
            return sin_table[index];
        case 1: // 64-127 (90-179 deg)
            return sin_table[64 - index];
        case 2: // 128-191 (180-269 deg)
            return -sin_table[index];
        case 3: // 192-255 (270-359 deg)
            return -sin_table[64 - index];
    }
    return 0; // Should not happen
}

// Get cosine value using sin(angle + 90 deg) relation
// angle + 90 deg = angle + 64 (in our 0-255 range)
fixed fixed_cos(unsigned char angle) {
    return fixed_sin(angle + 64);
}


// --- Cube Definition ---
typedef struct {
    fixed x, y, z;
} Point3D;

typedef struct {
    unsigned char x, y;
} Point2D;

// Define the 8 vertices of the cube centered at origin
// Scaled up initially to work better with fixed point
#define CUBE_SIZE INT_TO_FIXED(10) // Size of half the cube side length

Point3D base_vertices[8] = {
    { -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE },
    {  CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE },
    {  CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE },
    { -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE },
    { -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE },
    {  CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE },
    {  CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE },
    { -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE }
};

// Define the 12 edges connecting vertices by index
typedef struct {
    unsigned char v1;
    unsigned char v2;
} Edge;

const Edge edges[12] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Back face
    {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Front face
    {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connecting edges
};

// Buffers for transformed and projected vertices
Point3D rotated_vertices[8];
Point2D projected_vertices[8];

// --- Rotation and Zoom State ---
unsigned char angleX = 0;
unsigned char angleY = 0;
unsigned char angleZ = 0;
fixed zoom = INT_TO_FIXED(1); // Start with 1x zoom

// --- Main Program ---
int main(void) {
    unsigned char i;
    unsigned char portb;
    fixed sx, cx, sy, cy, sz, cz; // Precalculated sin/cos
    fixed tempX, tempY, tempZ;    // Temps for rotation calculation
    fixed zoom_factor = zoom;     // Current zoom factor for projection

    // Initialize VIA Port B for input (reading buttons)
    // Assuming pins 0-7 are connected to buttons as defined in 6502fun.h
    for (i = 0; i < 8; ++i) {
        via_pin_mode_b(i, VIA_PIN_INPUT);
    }

    screen_clear();
    screen_refresh();

    while (1) {
        // --- 1. Handle Input ---
        portb = via_read_port_b();

        // Rotation controls (Left/Right/Up/Down)
        if (!(portb & BTN_LEFT_MASK))  angleY += 2; // Rotate left around Y axis
        if (!(portb & BTN_RIGHT_MASK)) angleY -= 2; // Rotate right around Y axis
        if (!(portb & BTN_UP_MASK))    angleX += 2; // Rotate up around X axis
        if (!(portb & BTN_DOWN_MASK))  angleX -= 2; // Rotate down around X axis
        // Optional: A/B for Z rotation
        if (!(portb & BTN_B_MASK))     angleZ += 2; // Rotate clockwise around Z
        if (!(portb & BTN_SELECT_MASK)) angleZ -= 2; // Rotate counter-clockwise around Z


        // Zoom controls (A / B or other buttons)
        // Let's use A for Zoom In, START for Zoom Out
        if (!(portb & BTN_A_MASK)) {
             zoom += (SCALE / 16); // Increase zoom (zoom > 1)
             if (zoom > INT_TO_FIXED(4)) zoom = INT_TO_FIXED(4); // Limit zoom in
        }
        if (!(portb & BTN_START_MASK)) {
             zoom -= (SCALE / 16); // Decrease zoom (zoom < 1)
             if (zoom < (SCALE / 4)) zoom = (SCALE / 4); // Limit zoom out
        }

        // --- 2. Calculate Transformations ---

        // Precalculate sines and cosines for current angles
        sx = fixed_sin(angleX); cx = fixed_cos(angleX);
        sy = fixed_sin(angleY); cy = fixed_cos(angleY);
        sz = fixed_sin(angleZ); cz = fixed_cos(angleZ);

        // Apply rotation to each base vertex
        for (i = 0; i < 8; ++i) {
            // Get original vertex
            Point3D p = base_vertices[i];

            // Rotate around Y axis
            tempX = FIXED_MUL(p.x, cy) + FIXED_MUL(p.z, sy);
            tempZ = FIXED_MUL(-p.x, sy) + FIXED_MUL(p.z, cy);
            p.x = tempX;
            p.z = tempZ;

            // Rotate around X axis
            tempY = FIXED_MUL(p.y, cx) - FIXED_MUL(p.z, sx);
            tempZ = FIXED_MUL(p.y, sx) + FIXED_MUL(p.z, cx);
            p.y = tempY;
            p.z = tempZ;

            // Rotate around Z axis
            tempX = FIXED_MUL(p.x, cz) - FIXED_MUL(p.y, sz);
            tempY = FIXED_MUL(p.x, sz) + FIXED_MUL(p.y, cz);
            p.x = tempX;
            p.y = tempY;

            // Store rotated vertex
            rotated_vertices[i] = p;
        }

        // --- 3. Project Vertices to 2D ---
        // Orthographic projection with zoom
        zoom_factor = zoom; // Use the current zoom state
        for (i = 0; i < 8; ++i) {
            // Apply zoom factor
            fixed projected_x = FIXED_MUL(rotated_vertices[i].x, zoom_factor);
            fixed projected_y = FIXED_MUL(rotated_vertices[i].y, zoom_factor);

            // Convert fixed point to screen coordinates and add offset
            projected_vertices[i].x = FIXED_TO_INT(projected_x) + SCREEN_CENTER_X;
            projected_vertices[i].y = FIXED_TO_INT(projected_y) + SCREEN_CENTER_Y;

            // Basic clipping (optional, screen_draw_line might handle it)
            // if (projected_vertices[i].x >= SCREEN_WIDTH) projected_vertices[i].x = SCREEN_WIDTH - 1;
            // if (projected_vertices[i].y >= SCREEN_HEIGHT) projected_vertices[i].y = SCREEN_HEIGHT - 1;
            // if (projected_vertices[i].x < 0) projected_vertices[i].x = 0;
            // if (projected_vertices[i].y < 0) projected_vertices[i].y = 0;
        }

        // --- 4. Draw Wireframe ---
        screen_clear();
        // delay(10); // Maybe needed depending on hardware/screen implementation

        for (i = 0; i < 12; ++i) {
            Point2D p1 = projected_vertices[edges[i].v1];
            Point2D p2 = projected_vertices[edges[i].v2];
            screen_draw_line(p1.x, p1.y, p2.x, p2.y, 1); // Draw line with color 1
        }

        // --- 5. Refresh Screen & Delay ---
        screen_refresh();
        // delay(50); // Adjust for desired animation speed (milliseconds)
    }

    return 0; // Should not be reached
}