#include <stdio.h>
#include <6502fun.h>
#include <stdint.h> // Required for int8_t, int32_t, uint8_t
#include <string.h> // Required for memset

// --- Include the model and its helpers ---
#include "mnist_weights_int8.h" // Use the header file with int8 weights and quantization params
#include <limits.h>             // Required for INT8_MIN (though not used in final code below)
#include <math.h>               // Required for roundf if using standard lib, otherwise use custom one

// --- Screen and Drawing Definitions ---
#define SCREEN_WIDTH 64 // Assuming 64x32 screen based on example
#define SCREEN_HEIGHT 32
#define DRAW_SIZE 28
#define RESIZE_SIZE 14
#define DRAW_OFFSET_X 0
#define DRAW_OFFSET_Y 0 // Draw at top-left
#define PREDICTION_X (DRAW_OFFSET_X + DRAW_SIZE + 4) // Position for prediction output
#define PREDICTION_Y 10
#define STATUS_X 0
#define STATUS_Y 24 // Position for status updates during prediction

// --- Drawing Buffer ---
uint8_t drawing_buffer[DRAW_SIZE][DRAW_SIZE]; // 0=black, 1=white (user drawing)

// --- Resized Buffer (Input to Model) ---
uint8_t resized_buffer[RESIZE_SIZE][RESIZE_SIZE]; // 0-255 grayscale

// --- Cursor Position ---
uint8_t cursor_x = DRAW_SIZE / 2;
uint8_t cursor_y = DRAW_SIZE / 2;

// --- Prediction Result ---
int8_t prediction = -1; // -1 means no prediction yet

// --- Model Intermediate Buffer ---
#if defined(L1_OUTPUT_SIZE)
static float layer1_accumulators_float[L1_OUTPUT_SIZE];
#else
#error "L1_OUTPUT_SIZE is not defined in mnist_weights_int8.h"
#endif

// --- Model Input Parameters ---
#define INPUT_ZERO_POINT 0
#define INPUT_SCALE (1.0f / 255.0f)
#define LEAKY_RELU_ALPHA 0.01f

// --- Custom roundf ---
float roundf(float x) {
    if (x >= 0) {
        if (x - (int)x >= 0.5) {
            return (int)x + 1;
        } else {
            return (int)x;
        }
    } else {
        if ((int)x - x >= 0.5) {
            return (int)x - 1;
        } else {
            return (int)x;
        }
    }
}

// --- Helper function to clear a rectangular area ---
void clear_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    uint8_t ix, iy;
    // Loop through the rectangle's area and set pixels to 0 (off/black)
    // Use screen_set_pixel_safe to avoid drawing outside screen bounds
    for (iy = y; iy < y + height; ++iy) {
        for (ix = x; ix < x + width; ++ix) {
            screen_set_pixel_safe(ix, iy, 0);
        }
    }
}

// --- Quantization/Dequantization Helpers ---
static inline int8_t quantize_float_to_int8(float value, float scale, int32_t zero_point) {
    int32_t quant_val = (int32_t)roundf(value / scale) + zero_point;
    if (quant_val < -128) quant_val = -128;
    if (quant_val > 127) quant_val = 127;
    return (int8_t)quant_val;
}

static inline float dequantize_int8_to_float(int8_t value, float scale, int32_t zero_point) {
    return ((float)value - zero_point) * scale;
}

// --- Bilinear Interpolation (same as before) ---
void resize_image_bilinear() {
    float src_x, src_y;
    int x1, y1, x2, y2;
    float dx, dy;
    float p11, p12, p21, p22;
    float interp_top, interp_bottom, final_value;
    const float x_ratio = (float)DRAW_SIZE / (float)RESIZE_SIZE;
    const float y_ratio = (float)DRAW_SIZE / (float)RESIZE_SIZE;

    for (uint8_t ry = 0; ry < RESIZE_SIZE; ++ry) {
        for (uint8_t rx = 0; rx < RESIZE_SIZE; ++rx) {
            src_x = ((float)rx + 0.5f) * x_ratio - 0.5f;
            src_y = ((float)ry + 0.5f) * y_ratio - 0.5f;
            x1 = (int)src_x; y1 = (int)src_y;
            x2 = x1 + 1; y2 = y1 + 1;
            dx = src_x - (float)x1; dy = src_y - (float)y1;
            p11 = (x1 >= 0 && x1 < DRAW_SIZE && y1 >= 0 && y1 < DRAW_SIZE) ? (drawing_buffer[y1][x1] * 255.0f) : 0.0f;
            p21 = (x2 >= 0 && x2 < DRAW_SIZE && y1 >= 0 && y1 < DRAW_SIZE) ? (drawing_buffer[y1][x2] * 255.0f) : 0.0f;
            p12 = (x1 >= 0 && x1 < DRAW_SIZE && y2 >= 0 && y2 < DRAW_SIZE) ? (drawing_buffer[y2][x1] * 255.0f) : 0.0f;
            p22 = (x2 >= 0 && x2 < DRAW_SIZE && y2 >= 0 && y2 < DRAW_SIZE) ? (drawing_buffer[y2][x2] * 255.0f) : 0.0f;
            interp_top = p11 * (1.0f - dx) + p21 * dx;
            interp_bottom = p12 * (1.0f - dx) + p22 * dx;
            final_value = interp_top * (1.0f - dy) + interp_bottom * dy;
            if (final_value < 0.0f) final_value = 0.0f;
            if (final_value > 255.0f) final_value = 255.0f;
            resized_buffer[ry][rx] = (uint8_t)roundf(final_value);
        }
    }
}

// --- Helper to draw status string and number ---
// Clears a small area before drawing to prevent overlapping text
void draw_status(const char* label, int current, int total) {
    // Clear previous status line using the new function
    clear_rect(STATUS_X, STATUS_Y, SCREEN_WIDTH, 8); // Clear 8 pixels high area

    screen_draw_string(label, STATUS_X, STATUS_Y, 1);
    screen_draw_number(current, STATUS_X + strlen(label) * 4, STATUS_Y, 1); // Assuming 4px char width
    screen_draw_char('/', STATUS_X + strlen(label) * 4 + 4*3, STATUS_Y, 1); // Approx position for '/'
    screen_draw_number(total, STATUS_X + strlen(label) * 4 + 4*4, STATUS_Y, 1); // Approx position for total
}


// --- Neural Network Inference with Status Updates ---
uint8_t run_model() {
    uint16_t i, j; // Use appropriate size based on loop limits
    const uint16_t refresh_interval_l1_input = 8; // Refresh screen every N input pixels
    const uint16_t refresh_interval_l1_act = 8;   // Refresh screen every N activations
    const uint8_t refresh_interval_l2 = 1;         // Refresh screen every output neuron
    draw_status("L1 Init", 0, 1);
    screen_refresh();


    // --- Layer 1: Fully Connected + Leaky ReLU ---
    // Initialize accumulators with biases
    for (j = 0; j < L1_OUTPUT_SIZE; j++) {
        int8_t bias_quant = L1_BIASES_INT8[j];
        layer1_accumulators_float[j] = dequantize_int8_to_float(bias_quant, L1_BIAS_SCALE, L1_BIAS_ZERO_POINT);
    }
    // Optional: Initial status update
    // draw_status("L1 Init Done", L1_OUTPUT_SIZE, L1_OUTPUT_SIZE);
    // screen_refresh();
    // delay(10); // Allow time to see message

    // Matrix multiplication: Input * Weights
    for (i = 0; i < L1_INPUT_SIZE; i++) {
        // --- Status Update ---
        if ((i % refresh_interval_l1_input) == 0) {
            draw_status("L1 Input ", i, L1_INPUT_SIZE);
            screen_refresh();
            delay(1); // Minimal delay to allow screen update
        }
        // --- Calculation ---
        uint8_t p = resized_buffer[i / RESIZE_SIZE][i % RESIZE_SIZE];
        float dequantized_input = (float)p * INPUT_SCALE;
        if (p != 0) {
            for (j = 0; j < L1_OUTPUT_SIZE; j++) {
                int8_t weight_quant = L1_WEIGHTS_INT8[j][i];
                float dequantized_weight = dequantize_int8_to_float(weight_quant, L1_WEIGHT_SCALE, L1_WEIGHT_ZERO_POINT);
                layer1_accumulators_float[j] += dequantized_weight * dequantized_input;
            }
        }
    }
    // Final status for L1 Input
    draw_status("L1 Input ", L1_INPUT_SIZE, L1_INPUT_SIZE);
    screen_refresh();
    delay(10);

    // Apply Leaky ReLU activation
    for (j = 0; j < L1_OUTPUT_SIZE; j++) {
         // --- Status Update ---
         if ((j % refresh_interval_l1_act) == 0) {
            draw_status("L1 ReLU ", j, L1_OUTPUT_SIZE);
            screen_refresh();
            delay(1); // Minimal delay
         }
         // --- Calculation ---
        if (layer1_accumulators_float[j] < 0.0f) {
            layer1_accumulators_float[j] *= LEAKY_RELU_ALPHA;
        }
    }
    // Final status for L1 ReLU
    draw_status("L1 ReLU ", L1_OUTPUT_SIZE, L1_OUTPUT_SIZE);
    screen_refresh();
    delay(10);


    // --- Layer 2: Fully Connected (Output Layer) ---
    float max_val = -3.402823e+38;
    uint8_t pred_digit = 0;

    for (j = 0; j < L2_OUTPUT_SIZE; j++) {
        // --- Status Update ---
         if ((j % refresh_interval_l2) == 0) { // Update for every output neuron
            draw_status("L2 Output", j, L2_OUTPUT_SIZE);
            screen_refresh();
            delay(1); // Minimal delay
         }
        // --- Calculation ---
        int8_t bias_quant = L2_BIASES_INT8[j];
        float accumulator_float = dequantize_int8_to_float(bias_quant, L2_BIAS_SCALE, L2_BIAS_ZERO_POINT);
        for (i = 0; i < L2_INPUT_SIZE; i++) {
            float dequantized_hidden_input = layer1_accumulators_float[i];
            int8_t weight_quant = L2_WEIGHTS_INT8[j][i];
            float dequantized_weight = dequantize_int8_to_float(weight_quant, L2_WEIGHT_SCALE, L2_WEIGHT_ZERO_POINT);
            accumulator_float += dequantized_weight * dequantized_hidden_input;
        }
        if (accumulator_float > max_val) {
            max_val = accumulator_float;
            pred_digit = j;
        }
    }
     // Final status for L2
    draw_status("L2 Output", L2_OUTPUT_SIZE, L2_OUTPUT_SIZE);
    screen_refresh();
    delay(10);

    return pred_digit;
}

// --- Drawing Functions ---
void draw_canvas() {
    uint8_t x, y;
    for (y = 0; y < DRAW_SIZE; ++y) {
        for (x = 0; x < DRAW_SIZE; ++x) {
            if (drawing_buffer[y][x]) {
                screen_set_pixel_safe(x + DRAW_OFFSET_X, y + DRAW_OFFSET_Y, 1);
            }
        }
    }
    // Draw cursor (blinking effect might be too slow, use solid)
    screen_set_pixel_safe(cursor_x + DRAW_OFFSET_X, cursor_y + DRAW_OFFSET_Y, 1);
}

void clear_drawing() {
    memset(drawing_buffer, 0, sizeof(drawing_buffer));
    prediction = -1; // Reset prediction when clearing
}

// --- Splash Screen Function ---
void show_splash_screen() {
    screen_clear();
    delay(10);
    screen_draw_string("D-Pad: Move", 2, 0, 1);
    screen_draw_string("A:Draw/B:Erase", 2, 6, 1);
    screen_draw_string("Select: Clear", 2, 12, 1);
    screen_draw_string("Start: Run", 2, 18, 1);

    screen_draw_string("Press START", 8, 26, 1);
    screen_refresh();

    // Wait for START button press
    while (via_read_port_b() & BTN_START_MASK) {
        // Wait for button press (reading 0 means pressed)
        delay(20); // Small delay to prevent busy-waiting
    }
    // Wait for START button release (debounce)
    while (!(via_read_port_b() & BTN_START_MASK)) {
        // Wait for button release  (reading 1 means released)
         delay(20);
    }
}


// --- Main Program ---
int main(void) {
    unsigned char portb;
    unsigned char last_portb = 0xFF;
    unsigned long last_input_time = 0;
    const unsigned long debounce_delay = 50; // ms delay for button debounce/repeat

    // --- Initialization ---
    printf("6502 Handwriting Recognizer\n"); // For debug console

    // Configure VIA Port B for input
    via_pin_mode_b(0, VIA_PIN_INPUT); via_pin_mode_b(1, VIA_PIN_INPUT);
    via_pin_mode_b(2, VIA_PIN_INPUT); via_pin_mode_b(3, VIA_PIN_INPUT);
    via_pin_mode_b(4, VIA_PIN_INPUT); via_pin_mode_b(5, VIA_PIN_INPUT);
    via_pin_mode_b(6, VIA_PIN_INPUT); via_pin_mode_b(7, VIA_PIN_INPUT);

    // Show splash screen first
    show_splash_screen();

    // Initialize drawing buffer
    clear_drawing();

    // Initial screen clear for drawing area
    screen_clear();
    screen_refresh();
    delay(20);

    // --- Main Loop ---
    while (1) {
        unsigned long current_time = millis();
        portb = via_read_port_b();

        // --- Handle Input (with simple time-based debounce/repeat) ---
        // Process input only if state changed OR enough time passed for repeat
        if (portb != last_portb || (portb != 0xFF && (current_time - last_input_time > debounce_delay)))
        {
             if (portb != 0xFF) { // Only update time if a button is pressed
                 last_input_time = current_time;
             }

            // --- Cursor Movement ---
            // Store previous position to know if we need to redraw cursor area
            uint8_t prev_cursor_x = cursor_x;
            uint8_t prev_cursor_y = cursor_y;
            uint8_t moved = 0; // Flag to check if cursor moved

            if (!(portb & BTN_UP_MASK) && cursor_y > 0) { cursor_y--; moved = 1; }
            if (!(portb & BTN_DOWN_MASK) && cursor_y < DRAW_SIZE - 1) { cursor_y++; moved = 1; }
            if (!(portb & BTN_LEFT_MASK) && cursor_x > 0) { cursor_x--; moved = 1; }
            if (!(portb & BTN_RIGHT_MASK) && cursor_x < DRAW_SIZE - 1) { cursor_x++; moved = 1; }

            // --- Drawing / Erasing (2x2 PENCIL) --- // <<< UPDATED COMMENT
            // Use signed loop variables for easier boundary checks relative to cursor
            int8_t dx, dy; // Can remain int8_t, or use uint8_t since offsets are non-negative
            uint8_t draw_val = 0; // 0 for erase, 1 for draw
            uint8_t is_drawing_or_erasing = 0;

            if (!(portb & BTN_A_MASK)) { // Draw
                draw_val = 1;
                is_drawing_or_erasing = 1;
            } else if (!(portb & BTN_B_MASK)) { // Erase
                draw_val = 0;
                is_drawing_or_erasing = 1;
            }

            if (is_drawing_or_erasing) {
                // Loop through the 2x2 area starting at the cursor
                for (dy = 0; dy <= 1; ++dy) { // <<< MODIFIED LOOP BOUNDS
                    for (dx = 0; dx <= 1; ++dx) { // <<< MODIFIED LOOP BOUNDS
                        // Calculate the target coordinates in the drawing buffer
                        // Using uint8_t directly might be slightly more efficient if target types are checked carefully
                        uint8_t target_x = cursor_x + dx;
                        uint8_t target_y = cursor_y + dy;

                        // Check if the target coordinates are within the buffer bounds
                        // No need for >= 0 check since cursor_x/y and dx/dy are >= 0
                        if (target_x < DRAW_SIZE && target_y < DRAW_SIZE)
                        {
                            // Set or clear the pixel in the drawing buffer
                            drawing_buffer[target_y][target_x] = draw_val;
                        }
                    }
                }
            }
            // --- Actions ---
            // Check SELECT press (only trigger once per press)
            if (!(portb & BTN_SELECT_MASK) && (last_portb & BTN_SELECT_MASK)) {
                clear_drawing();
            }
            // Check START press (only trigger once per press)
            if (!(portb & BTN_START_MASK) && (last_portb & BTN_START_MASK)) {
                // 1. Show "Processing..." message and clear area for status
                screen_clear();
                delay(10);
                screen_draw_string("Processing...", 0, 0, 1);
                // Status area will be updated by run_model
                screen_refresh();
                delay(10); // Allow message to show

                // 2. Resize the image (relatively fast, no status needed)
                resize_image_bilinear();

                // 3. Run the model (will display its own status)
                prediction = run_model();

                // 4. Model finished, main loop will redraw the result screen
                // No explicit clear needed here, main loop handles redraw
            }
        }
        last_portb = portb; // Store current button state for next loop comparison

        // --- Update Display ---
        screen_clear(); // Clear buffer before drawing new frame

        // Draw the canvas and cursor
        draw_canvas();

        // Draw prediction result (if available)
        if (prediction != -1) {
            screen_draw_string("Pred:", PREDICTION_X, PREDICTION_Y, 1);
            // Ensure enough space is cleared for the number if it changes
            // Use clear_rect instead of screen_draw_rect
            clear_rect(PREDICTION_X + 5*4, PREDICTION_Y, 4*3, 8); // Clear 3 digits space (approx 12x8 pixels)
            screen_draw_number(prediction, PREDICTION_X + 5*4, PREDICTION_Y, 1);
        } else {
             screen_draw_string("Pred:?", PREDICTION_X, PREDICTION_Y, 1);
        }

        screen_refresh(); // Show the updated frame

        // Delay to control loop speed
        delay(50); // Adjust as needed
    }

    return 0; // Should not be reached
}