#include <stdint.h>
#include <pi/registers.h>
#include <pi/uart.h>
#include <malloc.h>


extern unsigned int bss_start;
extern unsigned int bss_end;
extern unsigned int GET32(uint32_t);
extern unsigned int PUT32(uint32_t, uint32_t);


void led_init() {
    // set GPIO6 as an output
    unsigned int ra;

    ra = GET32(GPFSEL1);
    ra &=~ (7<<18);
    ra |= 1<<18;
    PUT32(GPFSEL1,ra);
}

void led_on() {
    PUT32(GPCLR0,1<<16);
}

void led_off() {
    PUT32(GPSET0,1<<16);
}

void switch_init() {
    // set GPIO1 as an input
    unsigned int ra;
    ra = GET32(GPFSEL1);
    ra &= ~(7<<3);
    ra |= 0 << 0;
    PUT32(GPFSEL1, ra);
}

unsigned int get_switch_state() {
    return GET32(GPLEV0) & (1<<18); // I don't understand why this comes through on bit 18, but I don't much care.
}

static uint32_t timer_ms_base; // 'zero' time

uint32_t get_timer_ms() {
    // Return ms elapsed since timer_ms_base. This is currently approximate - should do the math properly.
    return ((((uint64_t)GET32(SYSTIMERCHI) << 32) | (uint64_t)GET32(SYSTIMERCLO)) >> 10) - timer_ms_base;
}


uint32_t reset_timer_ms() {
    timer_ms_base = 0;
    timer_ms_base = get_timer_ms();
    return timer_ms_base;
}


void hardware_init() {
//    setup_heap();
    for(unsigned int i=bss_start;i<bss_end;i+=4) PUT32(i,0);
    uart_init();
}

#include <sys/types.h>
extern int dummy(int);

int usleep(useconds_t __useconds) {
    // Pause for about t ms
    int i;
    unsigned int t = __useconds;
    for (;t>0;t--) {
        for (i=5000;i>0;i--) dummy(i);
    }
    return __useconds;
}

