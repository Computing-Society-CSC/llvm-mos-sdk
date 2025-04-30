#include <stdio.h>
#include <6502fun.h>

int main(){
    printf("Hello, world!!!!!");
    uint8_t rand = get_rand();
    printf("%d", rand);
    screen_clear();
    delay(10);
    screen_set_pixel(0,0,1);
    screen_refresh();
}