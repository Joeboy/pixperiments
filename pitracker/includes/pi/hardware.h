#ifndef PI_HARDWARE_H
#define PI_HARDWARE_H

#include <pi/dma.h>

#define BCM2708_PERI_BASE 0x20000000
#define GPIO_BASE         (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
#define PWM_BASE          (BCM2708_PERI_BASE + 0x20C000) /* PWM controller */
#define CLOCK_BASE        (BCM2708_PERI_BASE + 0x101000)
#define PM_BASE           (BCM2708_PERI_BASE + 0x100000) /* Power Management, Reset controller and Watchdog registers */

#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define BCM2835_PWM_CONTROL 0
#define BCM2835_PWM_STATUS  1
#define BCM2835_PWM_DMAC    2
#define BCM2835_PWM0_RANGE  4
#define BCM2835_PWM0_DATA   5
#define BCM2835_PWM_FIFO    6
#define BCM2835_PWM1_RANGE  8
#define BCM2835_PWM1_DATA   9

#define BCM2835_PWMCLK_CNTL   40
#define BCM2835_PWMCLK_DIV    41

#define BCM2835_PWMCLK_CNTL_OSCILLATOR 0x01
#define BCM2835_PWMCLK_CNTL_PLLA 0x04
#define BCM2835_PWMCLK_CNTL_KILL 1<<5
#define BCM2835_PWMCLK_CNTL_ENABLE 1<<4

#define PWMCTL_MODE1	(1<<1) // mode (0=pwm, 1=serializer)
#define PWMCTL_PWEN1	(1<<0) // enable ch1
#define PWMCTL_CLRF		(1<<6) // clear fifo
#define PWMCTL_USEF1	(1<<5) // use fifo

#define PWMDMAC_ENAB	(1<<31)
// I think this means it requests as soon as there is one free slot in the FIFO
// which is what we want as burst DMA would mess up our timing..
//#define PWMDMAC_THRSHLD	((15<<8)|(15<<0))
#define PWMDMAC_THRSHLD	((15<<8)|(15<<0))

#define BCM2835_PWM1_MS_MODE    0x8000  /*  Run in MS mode                   */
#define BCM2835_PWM1_USEFIFO    0x2000  /*  Data from FIFO                   */
#define BCM2835_PWM1_REVPOLAR   0x1000  /* Reverse polarity             */
#define BCM2835_PWM1_OFFSTATE   0x0800  /* Ouput Off state             */
#define BCM2835_PWM1_REPEATFF   0x0400  /* Repeat last value if FIFO empty   */
#define BCM2835_PWM1_SERIAL     0x0200  /* Run in serial mode             */
#define BCM2835_PWM1_ENABLE     0x0100  /* Channel Enable             */

#define BCM2835_PWM0_MS_MODE    0x0080  /* Run in MS mode             */
#define BCM2835_PWM0_USEFIFO    0x0020  /* Data from FIFO             */
#define BCM2835_PWM0_REVPOLAR   0x0010  /* Reverse polarity             */
#define BCM2835_PWM0_OFFSTATE   0x0008  /* Ouput Off state             */
#define BCM2835_PWM0_REPEATFF   0x0004  /* Repeat last value if FIFO empty   */
#define BCM2835_PWM0_SERIAL     0x0002  /* Run in serial mode             */
#define BCM2835_PWM0_ENABLE     0x0001  /* Channel Enable             */

#define BCM2835_BERR  0x100
#define BCM2835_GAPO4 0x80
#define BCM2835_GAPO3 0x40
#define BCM2835_GAPO2 0x20
#define BCM2835_GAPO1 0x10
#define BCM2835_RERR1 0x8
#define BCM2835_WERR1 0x4
#define BCM2835_EMPT1 0x2
#define BCM2835_FULL1 0x1

#define PM_PASSWORD 0x5A000000 

#define GPFSEL1 0x20200004
#define GPSET0  0x2020001C
#define GPCLR0  0x20200028

// Watchdog registers
#define PM_BASE                     (BCM2708_PERI_BASE + 0x100000) /* Power Management, Reset controller and Watchdog registers */
#define PM_RSTC                     (PM_BASE+0x1c)
#define PM_WDOG                     (PM_BASE+0x24)
#define PM_WDOG_TIME_SET            0x000fffff
#define PM_RSTC_WRCFG_CLR           0xffffffcf
#define PM_RSTC_WRCFG_FULL_RESET    0x00000020

#define DMA_CONTROLLER_BASE 0x20007000
#define DMA5_CNTL_BASE DMA_CONTROLLER_BASE + 0x500
#define DMA_CNTL_CS          0x00
#define DMA_CNTL_CONBLK_AD   0x04
#define DMA_CNTL_DEBUG       0x20

extern void PUT32 (uint32_t,uint32_t);
extern uint32_t GET32 (uint32_t);
extern void dummy (uint32_t);

volatile uint32_t* gpio = (void*)GPIO_BASE;
volatile uint32_t* pwm = (void*)PWM_BASE;

void pause(uint32_t t) {
    // Pause for about t ms
    int i;
    for (;t>0;t--) {
        for (i=5000;i>0;i--) dummy(i);
    }
}


#endif
