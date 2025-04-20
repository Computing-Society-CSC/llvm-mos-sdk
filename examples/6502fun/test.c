#include <stdio.h>
#include <6502fun.h>

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
    printf("i = %d\n", i);
    i++;
    unsigned char portb = via_read_port_b();
    printf("PORTB: %d\n", portb);
    // screen_draw_line(0,0,63,31,1);
    // screen_draw_rect(24,24,6,6,1);
    // screen_refresh();
    // delay(100);
    // screen_clear();
    // screen_draw_quad(16,24,32,48,1);
    // // screen_draw_circle(48,48,6,1);
    // screen_refresh();
    // screen_clear();

    delay(1000);
  }
  return 0;
}