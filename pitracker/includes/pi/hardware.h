#ifndef HARDWARE_H
void led_init();
void led_on();
void led_off();
void switch_init();
unsigned int get_switch_state();
uint32_t get_timer_ms();
uint32_t reset_timer_ms();
void hardware_init();
#endif
