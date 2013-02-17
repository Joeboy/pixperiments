#include <linux/kbhit.h>
#include <stdio.h>
#include <stdint.h>

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

static uint32_t timer_ms_base; // 'zero' time

unsigned int get_timer_ms() {
    // Return ms elapsed since timer_ms_base. This is currently approximate - should do the math properly.
//    return (unsigned int)(((((uint64_t)GET32(SYSTIMERCHI) << 32) | (uint64_t)GET32(SYSTIMERCLO)) >> 10) - timer_ms_base);
    return 0;
}


unsigned int reset_timer_ms() {
    timer_ms_base = 0;
    timer_ms_base = get_timer_ms();
    return timer_ms_base;
}

