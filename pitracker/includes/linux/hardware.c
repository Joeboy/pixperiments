#include <linux/kbhit.h>
#include <stdio.h>

void hardware_init() {
  init_keyboard();
}

void switch_init() {

}

unsigned int get_switch_state() {
    return 0;
}

void led_init() {

}

void led_on() {
    printf("*");
    fflush(stdout);
}

void led_off() {

}
