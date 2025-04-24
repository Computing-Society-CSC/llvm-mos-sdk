#include <6502fun.h>

// --- Button Masks (from example) ---
#define BTN_UP_MASK 0x10
#define BTN_DOWN_MASK 0x80
#define BTN_LEFT_MASK 0x20
#define BTN_RIGHT_MASK 0x40
#define BTN_A_MASK 0x08
#define BTN_B_MASK 0x04
#define BTN_SELECT_MASK 0x01
#define BTN_START_MASK 0x02

// --- Game Configuration ---
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define MAX_SNAKE_LENGTH 128 // Max segments snake can have
#define INITIAL_SNAKE_LENGTH 3
#define SNAKE_COLOR 1         // Pixel on
#define FOOD_COLOR 1          // Pixel on
#define BACKGROUND_COLOR 0    // Pixel off (used implicitly by screen_clear)
#define DELAY_MS 150          // Game speed (milliseconds per step)
#define CLEAR_DELAY_MS 10     // Delay after screen_clear as per example

// --- Game Structures ---
typedef struct {
    unsigned char x;
    unsigned char y;
} Point;

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef enum {
    START_SCREEN,
    PLAYING,
    GAME_OVER
} GameState;

// --- Global Variables ---
Point snake[MAX_SNAKE_LENGTH];
unsigned char snake_length;
Direction current_direction;
Direction next_direction; // Buffer for input to prevent 180 turns between steps
Point food;
unsigned int score;
GameState game_state;

// Draw "GAME OVER" text using screen_draw_string
void draw_game_over_text(unsigned char x, unsigned char y) {
    screen_draw_string("GAME", x, y, SNAKE_COLOR);
    screen_draw_string("OVER", x, y + 5, SNAKE_COLOR); // Draw "OVER" on the next line (4 height + 1 space)
}

// Place food randomly, ensuring it's not on the snake
void place_food() {
    unsigned char i;
    unsigned char collision;
    do {
        collision = 0;
        // Use get_rand() - result is unsigned short, modulo works fine
        food.x = get_rand() % SCREEN_WIDTH;
        food.y = get_rand() % SCREEN_HEIGHT;

        // Check collision with snake body
        for (i = 0; i < snake_length; ++i) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                collision = 1;
                break;
            }
        }
    } while (collision);
}

// Initialize game variables
void initialize_game() {
    unsigned char i;
    // Center snake horizontally, slightly above center vertically
    unsigned char start_x = SCREEN_WIDTH / 2;
    unsigned char start_y = SCREEN_HEIGHT / 2;

    snake_length = INITIAL_SNAKE_LENGTH;
    current_direction = DIR_RIGHT;
    next_direction = DIR_RIGHT; // Initialize buffered direction too
    score = 0;

    // Initialize snake segments (head first)
    for (i = 0; i < snake_length; ++i) {
        snake[i].x = start_x - i;
        snake[i].y = start_y;
    }

    place_food();
    game_state = PLAYING; // Set state to playing
}

// --- Main Game Logic ---
int main(void) {
    unsigned char i;
    unsigned char portb;
    Point next_head;

    // --- Hardware Setup ---
    // Set all port B pins to input for buttons
    via_pin_mode_b(0, VIA_PIN_INPUT);
    via_pin_mode_b(1, VIA_PIN_INPUT);
    via_pin_mode_b(2, VIA_PIN_INPUT);
    via_pin_mode_b(3, VIA_PIN_INPUT);
    via_pin_mode_b(4, VIA_PIN_INPUT);
    via_pin_mode_b(5, VIA_PIN_INPUT);
    via_pin_mode_b(6, VIA_PIN_INPUT);
    via_pin_mode_b(7, VIA_PIN_INPUT);

    // --- Initial State ---
    game_state = START_SCREEN; // Start with a title/start screen

    // --- Main Loop ---
    while (1) {
        // --- Read Input ---
        portb = via_read_port_b();

        // --- State Machine ---
        switch (game_state) {
            case START_SCREEN:
                // Display "Press Start" using screen_draw_string
                screen_clear();
                delay(CLEAR_DELAY_MS);

                // Center text approximately
                screen_draw_string("SNAKE", SCREEN_WIDTH / 2 - (5 * 4 / 2), 10, SNAKE_COLOR);
                screen_draw_string("PRESS START", SCREEN_WIDTH / 2 - (11 * 4 / 2), 20, SNAKE_COLOR);

                screen_refresh();

                // Wait for START button press
                if (!(portb & BTN_START_MASK)) {
                    initialize_game(); // This sets game_state = PLAYING
                }
                delay(50); // Small delay to prevent bouncing/fast loops
                break;

            case PLAYING:
                // --- Handle Input (Update next_direction) ---
                // Only change direction if it's not the opposite of the current one
                if (!(portb & BTN_UP_MASK) && current_direction != DIR_DOWN) {
                    next_direction = DIR_UP;
                } else if (!(portb & BTN_DOWN_MASK) && current_direction != DIR_UP) {
                    next_direction = DIR_DOWN;
                } else if (!(portb & BTN_LEFT_MASK) && current_direction != DIR_RIGHT) {
                    next_direction = DIR_LEFT;
                } else if (!(portb & BTN_RIGHT_MASK) && current_direction != DIR_LEFT) {
                    next_direction = DIR_RIGHT;
                }
                 // Allow pausing with START
                if (!(portb & BTN_START_MASK)) {
                   // Simple pause: just loop until START is pressed again
                   while(1) {
                       delay(50); // Prevent busy-waiting too hard
                       portb = via_read_port_b();
                       if (!(portb & BTN_START_MASK)) {
                           // Debounce - wait for release
                           while(!(via_read_port_b() & BTN_START_MASK)) { delay(20); }
                           break; // Exit pause loop
                       }
                   }
                }


                // --- Update Game State ---
                current_direction = next_direction; // Commit buffered direction change

                // Calculate next head position
                next_head = snake[0]; // Start with current head position
                switch (current_direction) {
                    case DIR_UP:    next_head.y--; break;
                    case DIR_DOWN:  next_head.y++; break;
                    case DIR_LEFT:  next_head.x--; break;
                    case DIR_RIGHT: next_head.x++; break;
                }

                // Collision Detection: Walls
                // Check boundaries (unsigned chars wrap around, so check both ends)
                if (next_head.x >= SCREEN_WIDTH || next_head.y >= SCREEN_HEIGHT) {
                    game_state = GAME_OVER;
                    break; // Go to GAME_OVER state processing
                }

                // Collision Detection: Self
                for (i = 0; i < snake_length; ++i) {
                    if (next_head.x == snake[i].x && next_head.y == snake[i].y) {
                        game_state = GAME_OVER;
                        break; // Go to GAME_OVER state processing
                    }
                }
                if (game_state == GAME_OVER) break; // Exit PLAYING state if collision detected


                // Check for Food
                if (next_head.x == food.x && next_head.y == food.y) {
                    // Grow snake
                    if (snake_length < MAX_SNAKE_LENGTH) {
                        snake_length++;
                    }
                    score++;
                    place_food();
                    // Don't move tail this turn (effectively adds segment at head)
                } else {
                    // Move snake body (shift segments back) - only if not eating
                    for (i = snake_length - 1; i > 0; --i) {
                        snake[i] = snake[i - 1];
                    }
                }

                // Move head to new position
                snake[0] = next_head;

                // --- Render ---
                screen_clear();
                delay(CLEAR_DELAY_MS); // Wait for clear

                // Draw food
                screen_draw_rect(food.x, food.y, 1, 1, FOOD_COLOR);

                // Draw snake
                for (i = 0; i < snake_length; ++i) {
                    screen_draw_rect(snake[i].x, snake[i].y, 1, 1, SNAKE_COLOR);
                }

                // Draw score using the new screen_draw_number function
                screen_draw_number(score, 1, 1, SNAKE_COLOR); // Draw score at top-left (x=1, y=1)

                screen_refresh();

                // --- Delay ---
                delay(DELAY_MS);
                break;

            case GAME_OVER:
                // Display Game Over message using the new functions
                screen_clear();
                delay(CLEAR_DELAY_MS);
                // Center "GAME OVER" text
                draw_game_over_text(SCREEN_WIDTH / 2 - (4 * 4 / 2), SCREEN_HEIGHT / 2 - 5);
                // Display final score below "GAME OVER"
                // Estimate width of score: max 5 digits * 4 width = 20. Center approx.
                { // Use a block to declare buffer locally
                    screen_draw_number(score, SCREEN_WIDTH / 2-5, SCREEN_HEIGHT / 2 + 8, SNAKE_COLOR);
                }

                screen_refresh();

                // Wait for START to restart
                if (!(portb & BTN_START_MASK)) {
                    // Debounce - wait for release before restarting
                    while(!(via_read_port_b() & BTN_START_MASK)) { delay(20); }
                    game_state = START_SCREEN; // Go back to start screen
                }
                 delay(50); // Small delay
                break;
        } // end switch(game_state)
    } // end while(1)

    return 0; // Should technically never be reached
}