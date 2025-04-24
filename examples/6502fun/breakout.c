// Breakout Game Example for 6502fun
// Press A to start

#include <stdio.h> // Included but printf ignored by user
#include <6502fun.h>

// --- Configuration ---

// Screen dimensions (assuming based on example)
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

// Paddle properties
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 2
#define PADDLE_Y (SCREEN_HEIGHT - PADDLE_HEIGHT - 1)
#define PADDLE_SPEED 2 // Pixels per frame move

// Ball properties
#define BALL_SIZE 1 // Draw as a 1x1 rectangle

// Brick properties
#define BRICK_ROWS 3
#define BRICK_COLS 8
#define BRICK_WIDTH (SCREEN_WIDTH / BRICK_COLS - 1) // 64/8 - 1 = 7
#define BRICK_HEIGHT 2
#define BRICK_X_SPACING 1
#define BRICK_Y_SPACING 1
#define BRICK_X_OFFSET 0
#define BRICK_Y_OFFSET 2 // Start bricks a bit down from the top

// Game properties
#define INITIAL_LIVES 3
#define GAME_LOOP_DELAY 30 // milliseconds, controls game speed (approx 25 FPS)

// Button masks (already defined in header, but good to have here for reference)
// #define BTN_UP_MASK 0x10
// #define BTN_DOWN_MASK 0x80
// #define BTN_LEFT_MASK 0x20
// #define BTN_RIGHT_MASK 0x40
// #define BTN_A_MASK 0x08
// #define BTN_B_MASK 0x04
// #define BTN_SELECT_MASK 0x01
// #define BTN_START_MASK 0x02

// --- Game State ---

enum GameState {
    STATE_START_SCREEN,
    STATE_PLAYING,
    STATE_BALL_LAUNCH, // Ball attached to paddle, waiting for launch
    STATE_GAME_OVER,
    STATE_YOU_WIN
};

struct Paddle {
    unsigned char x;
};

struct Ball {
    unsigned char x;
    unsigned char y;
    signed char dx; // Velocity: -1 or 1
    signed char dy; // Velocity: -1 or 1
};

// --- Global Variables ---

struct Paddle paddle;
struct Ball ball;
unsigned char bricks[BRICK_ROWS][BRICK_COLS]; // 1 = visible, 0 = destroyed
unsigned char lives;
unsigned int score; // We won't display it graphically yet, but let's track it
unsigned char bricksRemaining;
enum GameState currentState;
unsigned char currentButtons;
unsigned char previousButtons;

// --- Helper Functions ---

// Check if a specific button was just pressed (released -> pressed)
unsigned char button_pressed(unsigned char mask) {
    return (!(currentButtons & mask)) && (previousButtons & mask);
}

// Check if a specific button is currently held down
unsigned char button_held(unsigned char mask) {
    return !(currentButtons & mask);
}

// Draw lives as small squares
void draw_lives() {
    unsigned char i;
    for (i = 0; i < lives; ++i) {
        screen_draw_rect(SCREEN_WIDTH - (i + 1) * 3, 0, 2, 2, 1); // Draw lives top right
    }
}

// Draw the score
void draw_score() {
    // Draw score top left (adjust position as needed)
    screen_draw_number(score, 0, 22, 1);
}

// --- Game Logic Functions ---

void setup() {
    unsigned char r, c;

    // Initialize VIA Port B for input (already done in example main, but good practice)
    for (r = 0; r < 8; ++r) {
        via_pin_mode_b(r, VIA_PIN_INPUT);
    }

    // Initialize game state
    paddle.x = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
    lives = INITIAL_LIVES;
    score = 0;
    bricksRemaining = BRICK_ROWS * BRICK_COLS;
    currentState = STATE_START_SCREEN;
    previousButtons = via_read_port_b(); // Initialize button state

    // Initialize bricks
    for (r = 0; r < BRICK_ROWS; ++r) {
        for (c = 0; c < BRICK_COLS; ++c) {
            bricks[r][c] = 1; // All bricks visible
        }
    }
}

void reset_ball() {
    // Place ball centered on the paddle
    ball.x = paddle.x + PADDLE_WIDTH / 2;
    if (ball.x >= SCREEN_WIDTH) ball.x = SCREEN_WIDTH - 1; // Clamp ball x
    ball.y = PADDLE_Y - BALL_SIZE;

    // Set initial velocity (upwards, random horizontal)
    ball.dy = -1;
    ball.dx = (get_rand() % 2) ? 1 : -1; // Randomly left or right

    currentState = STATE_BALL_LAUNCH;
}

void update_input() {
    previousButtons = currentButtons;
    currentButtons = via_read_port_b();

    // --- Global Controls ---
    if (button_pressed(BTN_SELECT_MASK)) {
         // Optional: Reset game? Go to start screen?
         setup(); // Simple reset
         reset_ball();
         currentState = STATE_START_SCREEN;
         return;
    }

    // --- State-Specific Controls ---
    switch (currentState) {
        case STATE_START_SCREEN:
            if (button_pressed(BTN_START_MASK) || button_pressed(BTN_A_MASK)) {
                reset_ball(); // This sets state to STATE_BALL_LAUNCH
            }
            break;

        case STATE_BALL_LAUNCH:
            // Move paddle with ball attached
            if (button_held(BTN_LEFT_MASK)) {
                if (paddle.x > 0) {
                    paddle.x -= PADDLE_SPEED;
                    if (paddle.x > SCREEN_WIDTH) paddle.x = 0; // Check wrap around if speed > 1
                } else {
                    paddle.x = 0; // Clamp to left edge
                }
            }
            if (button_held(BTN_RIGHT_MASK)) {
                if (paddle.x < SCREEN_WIDTH - PADDLE_WIDTH) {
                    paddle.x += PADDLE_SPEED;
                     if (paddle.x > SCREEN_WIDTH - PADDLE_WIDTH) paddle.x = SCREEN_WIDTH - PADDLE_WIDTH; // Clamp
                } else {
                     paddle.x = SCREEN_WIDTH - PADDLE_WIDTH; // Clamp to right edge
                }
            }
            // Keep ball centered on paddle
            ball.x = paddle.x + PADDLE_WIDTH / 2;
            if (ball.x >= SCREEN_WIDTH) ball.x = SCREEN_WIDTH - 1; // Clamp ball x

            // Launch ball
            if (button_pressed(BTN_A_MASK) || button_pressed(BTN_START_MASK)) {
                currentState = STATE_PLAYING;
            }
            break;

        case STATE_PLAYING:
            // Move paddle
            if (button_held(BTN_LEFT_MASK)) {
                if (paddle.x > 0) {
                    paddle.x -= PADDLE_SPEED;
                     if (paddle.x > SCREEN_WIDTH) paddle.x = 0; // Check wrap around if speed > 1
                } else {
                    paddle.x = 0; // Clamp to left edge
                }
            }
            if (button_held(BTN_RIGHT_MASK)) {
                if (paddle.x < SCREEN_WIDTH - PADDLE_WIDTH) {
                    paddle.x += PADDLE_SPEED;
                    if (paddle.x > SCREEN_WIDTH - PADDLE_WIDTH) paddle.x = SCREEN_WIDTH - PADDLE_WIDTH; // Clamp
                } else {
                     paddle.x = SCREEN_WIDTH - PADDLE_WIDTH; // Clamp to right edge
                }
            }
            break;

        case STATE_GAME_OVER:
        case STATE_YOU_WIN:
            if (button_pressed(BTN_START_MASK) || button_pressed(BTN_A_MASK)) {
                setup(); // Re-initialize everything
                reset_ball();
                currentState = STATE_START_SCREEN; // Go back to start screen
            }
            break;
    }
}


void update_ball() {
    unsigned char next_x, next_y;
    unsigned char r, c;
    unsigned char brick_x, brick_y;
    unsigned char hit_brick = 0;

    // Calculate next potential position
    // Need to handle signed addition carefully with unsigned positions
    if (ball.dx > 0) {
        next_x = ball.x + ball.dx;
    } else {
        if (ball.x > 0) next_x = ball.x + ball.dx; // ball.dx is negative
        else next_x = 0; // Already at left edge, can't go further left
    }

    if (ball.dy > 0) {
        next_y = ball.y + ball.dy;
    } else {
         if (ball.y > 0) next_y = ball.y + ball.dy; // ball.dy is negative
         else next_y = 0; // Already at top edge
    }


    // --- Collision Detection ---

    // 1. Walls
    // Check right wall BEFORE calculating next_x if dx is positive
    if (ball.dx > 0 && (ball.x + ball.dx) >= SCREEN_WIDTH - BALL_SIZE) {
        ball.dx = -ball.dx;
        next_x = SCREEN_WIDTH - BALL_SIZE - 1; // Adjust position slightly
    }
    // Check left wall BEFORE calculating next_x if dx is negative
    else if (ball.dx < 0 && (ball.x == 0)) { // Simpler check: if at edge and moving left
        ball.dx = -ball.dx;
        next_x = 0; // Stay at edge
    }

    // Check top wall BEFORE calculating next_y if dy is negative
    if (ball.dy < 0 && (ball.y == 0)) {
        ball.dy = -ball.dy;
        next_y = 0; // Stay at edge
    }
    // No bottom wall check here - handled separately for losing life

    // 2. Paddle
    // Check if ball is moving downwards and will be at or below paddle level in the next step
    if (ball.dy > 0 && next_y >= PADDLE_Y) {
        // Check if ball's *next* x position overlaps with the paddle's current x range
        if (next_x >= paddle.x && next_x < (paddle.x + PADDLE_WIDTH)) {
            // --- Collision Confirmed ---
            ball.dy = -ball.dy; // Bounce vertically (always happens)
            next_y = PADDLE_Y - BALL_SIZE; // Place ball just above paddle to prevent sticking

            // --- Adjust Horizontal Bounce (dx) based on hit location ---
            // Calculate approximate paddle zones (integer division is okay)
            // PADDLE_WIDTH is 10. zone_width = 10 / 3 = 3.
            // Left zone: paddle.x to paddle.x + 2 (pixels 0, 1, 2 relative to paddle)
            // Center zone: paddle.x + 3 to paddle.x + 6 (pixels 3, 4, 5, 6 relative)
            // Right zone: paddle.x + 7 to paddle.x + 9 (pixels 7, 8, 9 relative)

            unsigned char hit_pos_relative = next_x - paddle.x; // Position relative to paddle start (0 to PADDLE_WIDTH-1)
            unsigned char zone_width = PADDLE_WIDTH / 3; // Width of the side zones

            if (zone_width == 0) zone_width = 1; // Ensure zones exist even for very narrow paddles

            if (hit_pos_relative < zone_width) {
                // Hit left zone
                ball.dx = -1;
            } else if (hit_pos_relative >= (PADDLE_WIDTH - zone_width)) {
                // Hit right zone
                ball.dx = 1;
            }
            // Else: Hit center zone - ball.dx remains unchanged from before the hit

            // Ensure ball doesn't get stuck in a wall immediately after bounce
            if (next_x == 0 && ball.dx < 0) ball.dx = 1;
            if (next_x >= SCREEN_WIDTH - BALL_SIZE -1 && ball.dx > 0) ball.dx = -1;

        }
    }

    // 3. Bricks
    // Check only if moving upwards or potentially sideways into bricks
    // Simple check: is the ball's next center point inside any active brick?
    for (r = 0; r < BRICK_ROWS && !hit_brick; ++r) {
        for (c = 0; c < BRICK_COLS && !hit_brick; ++c) {
            if (bricks[r][c]) { // If brick is visible
                brick_x = BRICK_X_OFFSET + c * (BRICK_WIDTH + BRICK_X_SPACING);
                brick_y = BRICK_Y_OFFSET + r * (BRICK_HEIGHT + BRICK_Y_SPACING);

                // Collision check (simple bounding box for next position)
                if (next_x >= brick_x && next_x < (brick_x + BRICK_WIDTH) &&
                    next_y >= brick_y && next_y < (brick_y + BRICK_HEIGHT))
                {
                    bricks[r][c] = 0; // Destroy brick
                    hit_brick = 1;
                    score += 10;
                    bricksRemaining--;

                    // Bounce - simple vertical bounce is usually sufficient for bricks
                    // Could add horizontal bounce check here too, but let's keep it simple
                    ball.dy = -ball.dy;

                    // Prevent sticking: If ball is now inside the brick space after bounce, move it out
                    // This is a basic fix, more robust physics would be better
                     if (ball.dy > 0 && next_y < (brick_y + BRICK_HEIGHT)) { // Bounced down, but still inside?
                         next_y = brick_y + BRICK_HEIGHT;
                     } else if (ball.dy < 0 && next_y >= brick_y) { // Bounced up, but still inside?
                         next_y = brick_y - BALL_SIZE;
                     }


                    // Check for win condition
                    if (bricksRemaining == 0) {
                        currentState = STATE_YOU_WIN;
                        // Optional: Add sound effect or visual flash
                        return; // Exit update_ball early
                    }
                }
            }
        }
    }

    // 4. Bottom Edge (Miss)
    // Check if ball's *next* y position goes past the bottom edge
    if (next_y >= SCREEN_HEIGHT - BALL_SIZE) {
        lives--;
        if (lives == 0) {
            currentState = STATE_GAME_OVER;
            // Optional: Add sound effect or visual flash
        } else {
            reset_ball(); // Reset ball position, wait for launch
        }
        return; // Exit update_ball early
    }


    // --- Update Ball Position ---
    // Only update if not game over/win/resetting
    if (currentState == STATE_PLAYING) {
         ball.x = next_x;
         ball.y = next_y;
         // Clamp just in case (should be less necessary with careful collision checks)
         if (ball.x >= SCREEN_WIDTH) ball.x = SCREEN_WIDTH - 1;
         if (ball.y >= SCREEN_HEIGHT) ball.y = SCREEN_HEIGHT - 1;
    }
}

bool finished_end_text = false;

void render() {
    unsigned char r, c;
    unsigned char brick_x, brick_y;

    screen_clear();
    delay(10); // Short delay needed for screen clear to register on some hardware

    switch (currentState) {
        case STATE_START_SCREEN:
            finished_end_text = false; // Reset for next game
            // Draw "PRESS A" centered
            screen_draw_string("PRESS A", (SCREEN_WIDTH - 7*3)/2, SCREEN_HEIGHT/2 - 3, 1); // Approx center
            break;

        case STATE_BALL_LAUNCH: // Fall through to draw game elements
        case STATE_PLAYING:
            // Draw Paddle
            screen_draw_rect(paddle.x, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, 1);

            // Draw Ball
            screen_draw_rect(ball.x, ball.y, BALL_SIZE, BALL_SIZE, 1);

            // Draw Bricks
            for (r = 0; r < BRICK_ROWS; ++r) {
                for (c = 0; c < BRICK_COLS; ++c) {
                    if (bricks[r][c]) {
                        brick_x = BRICK_X_OFFSET + c * (BRICK_WIDTH + BRICK_X_SPACING);
                        brick_y = BRICK_Y_OFFSET + r * (BRICK_HEIGHT + BRICK_Y_SPACING);
                        screen_draw_rect(brick_x, brick_y, BRICK_WIDTH, BRICK_HEIGHT, 1);
                    }
                }
            }
            // Draw Lives
            draw_lives();
            // Draw Score
            draw_score();
            break;

        case STATE_GAME_OVER:
            if (!finished_end_text) {
                // Draw "GAME OVER" centered
                for (int i = 0; i< 3; i++) {
                    screen_draw_string("GAME OVER", (SCREEN_WIDTH - 9*3)/2, SCREEN_HEIGHT/2 - 3 + i, 1); // Approx center
                    screen_refresh();
                    delay(300); // Flash effect
                    screen_clear();
                    screen_refresh();
                    delay(300); // Flash effect

                } 
                finished_end_text = true; // Prevent re-drawing
            } else {
                screen_draw_string("GAME OVER", (SCREEN_WIDTH - 9*3)/2, SCREEN_HEIGHT/2 - 3, 1); // Approx center
            }
            break;

        case STATE_YOU_WIN:
             // Draw "YOU WIN" centered
            if (!finished_end_text) {
                // Draw "GAME OVER" centered
                for (int i = 0; i< 3; i++) {
                    screen_draw_string("YOU WIN", (SCREEN_WIDTH - 9*3)/2, SCREEN_HEIGHT/2 - 3, 1); // Approx center
                    screen_refresh();
                    delay(300); // Flash effect
                    screen_clear();
                    screen_refresh();
                    delay(300); // Flash effect

                } 
                finished_end_text = true; // Prevent re-drawing
            } else {
                screen_draw_string("YOU WIN", (SCREEN_WIDTH - 9*3)/2, SCREEN_HEIGHT/2 - 3, 1); // Approx center
            }
            break;
    }

    screen_refresh();
}

// --- Main Function ---

int main(void) {
    // Initial setup
    setup();

    // Game loop
    while (1) {
        // 1. Handle Input
        update_input();

        // 2. Update Game State (Ball movement, collisions, etc.)
        // Only update ball if playing or launching (ball might move with paddle)
        if (currentState == STATE_PLAYING) {
            update_ball();
        }
        // State transitions are handled within update_input and update_ball

        // 3. Render Graphics
        render();

        // 4. Delay
        delay(GAME_LOOP_DELAY);
    }

    return 0; // Should never reach here
}