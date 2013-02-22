#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <pi/registers.h>
#include <config.h>

extern void PUT32 (uint32_t, uint32_t);
extern uint32_t GET32 (uint32_t);
extern void dummy (uint32_t);


void uart_init() {
    //GPIO14  TXD0 and TXD1
    //GPIO15  RXD0 and RXD1
    //alt function 5 for uart1
    //alt function 0 for uart0

    //((250,000,000/115200)/8)-1 = 270
//    uint32_t baud_reg = (float)250000000 / (float)baudrate / (float)8 - 1;
    uint32_t ra;
    PUT32(AUX_ENABLES,1);
    PUT32(AUX_MU_IER_REG,0);
    PUT32(AUX_MU_CNTL_REG,0);
    PUT32(AUX_MU_LCR_REG,3);
    PUT32(AUX_MU_MCR_REG,0);
    PUT32(AUX_MU_IER_REG,0);
    PUT32(AUX_MU_IIR_REG,0xC6);
//    PUT32(AUX_MU_BAUD_REG,999); // 115200 (serial console)
#ifdef DEBUG
    PUT32(AUX_MU_BAUD_REG,270); // 115200 (serial console)
#else
    PUT32(AUX_MU_BAUD_REG,999);       // 31250 (midi)
#endif

    ra=GET32(GPFSEL1);
    ra&=~(7<<12); //gpio14
    ra|=2<<12;    //alt5
    ra&=~(7<<15); //gpio15
    ra|=2<<15;    //alt5
    PUT32(GPFSEL1,ra);

    PUT32(GPPUD,0);
    usleep(1);
    PUT32(GPPUDCLK0,(1<<14)|(1<<15));
    usleep(1);
    PUT32(GPPUDCLK0,0);

    PUT32(AUX_MU_CNTL_REG,3);
    usleep(5);
}


uint32_t uart_input_ready() {
    return GET32(AUX_MU_LSR_REG)&1;
}

uint32_t uart_read() {
    return GET32(AUX_MU_IO_REG);
}

int uart_getc () {
    while (1) {
        if (uart_input_ready()) break;
    }
    return uart_read();
}

int uart_putc (uint32_t c, ...) {
    while(1) {
        if(GET32(AUX_MU_LSR_REG)&0x20) break;
    }
    PUT32(AUX_MU_IO_REG,c);
    return c;
}

