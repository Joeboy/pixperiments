#include "pi_hardware.h"

#define ERRORMASK (BCM2835_GAPO2 | BCM2835_GAPO1 | \
    BCM2835_RERR1 | BCM2835_WERR1)

unsigned int samplerate;

void audio_init(void) {
    // Set up the pwm clock
    // vals read from raspbian:
    // PWMCLK_CNTL = 148 = 10010100 - 100 is allegedly 'plla' but I can't make that work
    // PWMCLK_DIV = 16384
    // PWM_CONTROL=9509 = 10010100100101
    // PWM0_RANGE=1024
    // PWM1_RANGE=1024
    unsigned int range = 0x200;
    unsigned int idiv = 2; // 1 seems to fail
    SET_GPIO_ALT(40, 0);
    SET_GPIO_ALT(45, 0);
    pause(1); // I don't know if all these pauses are really necessary
    *(clk + BCM2835_PWMCLK_CNTL) = PM_PASSWORD | (1 << 5);

    samplerate = 19200000 / range / idiv;
    *(clk + BCM2835_PWMCLK_DIV)  = PM_PASSWORD | (idiv<<12);
    
    *(clk + BCM2835_PWMCLK_CNTL) = PM_PASSWORD | 16 | 1; // enable + oscillator
//    *(clk + BCM2835_PWMCLK_CNTL) = PM_PASSWORD | 16 | 4; // enable + plla
    pause(1);

    // disable PWM
    *(pwm + BCM2835_PWM_CONTROL) = 0;
    
    pause(1);

    *(pwm+BCM2835_PWM0_RANGE) = range;
    *(pwm+BCM2835_PWM1_RANGE) = range;

    *(pwm+BCM2835_PWM_CONTROL) =
          BCM2835_PWM1_USEFIFO | 
//          BCM2835_PWM1_REPEATFF |
          BCM2835_PWM1_ENABLE | 
          BCM2835_PWM0_USEFIFO | 
//          BCM2835_PWM0_REPEATFF |
          BCM2835_PWM0_ENABLE | 1<<6;

    pause(1);
//    uart_print("audio init done\r\n");
}

int audio_fifo_unavailable() {
    long status =  *(pwm + BCM2835_PWM_STATUS);
    if (status & ERRORMASK) {
//        uart_print("error: ");
//        hexstring(status);
//        uart_print("\r\n");
          /* clear errors */
          *(pwm+BCM2835_PWM_STATUS) = ERRORMASK;
          return 1;
    }
    return !!(status & BCM2835_FULL1);
}
