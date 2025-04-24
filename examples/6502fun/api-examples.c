#include <stdio.h>
#include <6502fun.h>

// bit number -> btn on via port
// read 1 = released, 0 = pressed
// below are defined in 6502fun.h
// #define BTN_UP_MASK 0x10
// #define BTN_DOWN_MASK 0x80
// #define BTN_LEFT_MASK 0x20
// #define BTN_RIGHT_MASK 0x40
// #define BTN_A_MASK 0x08
// #define BTN_B_MASK 0x04
// #define BTN_SELECT_MASK 0x01
// #define BTN_START_MASK 0x02


int main(void) {
  printf("Hello, World!\n");
  unsigned short rand = get_rand();
  printf("Random number: %d\n", rand);
  via_pin_mode_b(0, VIA_PIN_INPUT);
  via_pin_mode_b(1, VIA_PIN_INPUT);
  via_pin_mode_b(2, VIA_PIN_INPUT);
  via_pin_mode_b(3, VIA_PIN_INPUT);
  via_pin_mode_b(4, VIA_PIN_INPUT);
  via_pin_mode_b(5, VIA_PIN_INPUT);
  via_pin_mode_b(6, VIA_PIN_INPUT);
  via_pin_mode_b(7, VIA_PIN_INPUT);
  screen_clear();
  screen_refresh();
  char i = 0;
  while (1){
    {
      unsigned char portb = via_read_port_b();
      printf("Buttons pressed:");
      if (!(portb & BTN_UP_MASK))     printf(" UP");
      if (!(portb & BTN_DOWN_MASK))   printf(" DOWN");
      if (!(portb & BTN_LEFT_MASK))   printf(" LEFT");
      if (!(portb & BTN_RIGHT_MASK))  printf(" RIGHT");
      if (!(portb & BTN_A_MASK))      printf(" A");
      if (!(portb & BTN_B_MASK))      printf(" B");
      if (!(portb & BTN_SELECT_MASK)) printf(" SELECT");
      if (!(portb & BTN_START_MASK))  printf(" START");
      printf("\n");
    }
    printf("i = %lu\n", millis());
    unsigned char portb = via_read_port_b();
    printf("PORTB: %d\n", portb);
    screen_clear();
    delay(10); // must wait a bit for screen to clear
    screen_draw_line(i++ % 32,0,63,31,1);
    screen_draw_rect(24,i % 32,6,6,1);
    screen_draw_char('H', i, 0, 1);
    screen_draw_string("ello", i+3, 0, 1);
    screen_set_pixel_safe(i, 24, 1); // check bound
    screen_set_pixel_wrap(i-1, 24, 0); // wrap around edges
    screen_draw_number(i, 32, 16, 1);
    screen_refresh();
    delay(90);
  }
  return 0;
}