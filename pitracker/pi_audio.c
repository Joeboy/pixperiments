#include "pi_hardware.h"
#include "math.c"
#include "uart.c"

#define ERRORMASK (BCM2835_GAPO2 | BCM2835_GAPO1 | \
    BCM2835_RERR1 | BCM2835_WERR1)

#define AUDIO_BUFFER_SZ 128
uint32_t samplerate;

static struct bcm2708_dma_cb* cb_chain;

typedef struct {
    uint32_t *buffer;
    uint32_t read_p, write_p;
} ringbuffer;

ringbuffer buf;

void ringbuffer_init() {
    buf.buffer = malloc(sizeof(uint32_t) * AUDIO_BUFFER_SZ);
    buf.read_p = 0;
    buf.write_p = 0;
    uint32_t i;
    for (i=0;i<AUDIO_BUFFER_SZ;i++) {
        buf.buffer[i]=0;
    }
}

int32_t audio_buffer_free_space() {
    buf.read_p = (struct bcm2708_dma_cb*)GET32(DMA5_CNTL_BASE + DMA_CNTL_CONBLK_AD) - cb_chain;
    uint32_t spare;
    if (buf.read_p >= buf.write_p) {
        spare = buf.read_p - buf.write_p;
    } else {
        spare = AUDIO_BUFFER_SZ - (buf.write_p - buf.read_p);
    }
    return spare;
}

void audio_buffer_write(float*chunk, uint32_t chunk_sz) {
    uint32_t i;
    for (i=0;i<chunk_sz;i++) {
        buf.buffer[buf.write_p + i] = (uint32_t)(256+192*chunk[i]);
    }
    buf.write_p += chunk_sz;
    if (buf.write_p >= AUDIO_BUFFER_SZ) buf.write_p = 0;
}


void audio_init(void) {
    ringbuffer_init();
    // Set up the pwm clock
    // vals read from raspbian:
    // PWMCLK_CNTL = 148 = 10010100 - 100 is allegedly 'plla' but I can't make that work
    // PWMCLK_DIV = 16384
    // PWM_CONTROL=9509 = 10010100100101
    // PWM0_RANGE=1024
    // PWM1_RANGE=1024
    uint32_t range = 0x400;
    uint32_t idiv = 2; // 1 seems to fail
    SET_GPIO_ALT(40, 0); // set pins 40/45 (aka phone jack) to pwm function
    SET_GPIO_ALT(45, 0);
    pause(10); // I don't know if all these pauses are really necessary

    PUT32(CLOCK_BASE + 4*BCM2835_PWMCLK_CNTL, PM_PASSWORD | BCM2835_PWMCLK_CNTL_KILL);
    PUT32(PWM_BASE + 4*BCM2835_PWM_CONTROL, 0);

    samplerate = 19200000 / range / idiv;
    uart_print("samplerate=");
    printhex(samplerate);
    PUT32(CLOCK_BASE + 4*BCM2835_PWMCLK_DIV, PM_PASSWORD | (idiv<<12));
    
    PUT32(CLOCK_BASE + 4*BCM2835_PWMCLK_CNTL,
                                PM_PASSWORD | 
                                BCM2835_PWMCLK_CNTL_ENABLE |
                                BCM2835_PWMCLK_CNTL_OSCILLATOR);
    pause(1);
    PUT32(PWM_BASE + 4*BCM2835_PWM0_RANGE, range);
    PUT32(PWM_BASE + 4*BCM2835_PWM1_RANGE, range);
    pause(1);

    // Ensure buffer is 32-byte aligned
    void* cb_container = malloc(0x20 + sizeof(struct bcm2708_dma_cb) * AUDIO_BUFFER_SZ);
    uint32_t offset = (uint32_t)cb_container & 0x1f;
    cb_chain = cb_container + 0x20 - offset;

    uint32_t i;
    for (i=0;i<AUDIO_BUFFER_SZ;i++) {
        cb_chain[i].info   = BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP | BCM2708_DMA_D_DREQ | BCM2708_DMA_PER_MAP(5);
        cb_chain[i].src = (uint32_t)(buf.buffer+i);
        cb_chain[i].dst = ((PWM_BASE+4*BCM2835_PWM_FIFO) & 0x00ffffff) | 0x7e000000; // physical address of fifo
        cb_chain[i].length = sizeof(uint32_t);
        cb_chain[i].stride = 0;
        cb_chain[i].next = (uint32_t)&cb_chain[i+1];
    }
    cb_chain[i-1].next = (uint32_t)&cb_chain[0];
    pause(1);
    PUT32(PWM_BASE + 4*BCM2835_PWM_DMAC, PWMDMAC_ENAB | PWMDMAC_THRSHLD);
    pause(1);
    PUT32(PWM_BASE + 4*BCM2835_PWM_CONTROL, PWMCTL_CLRF);
    pause(1);

    PUT32(PWM_BASE + 4*BCM2835_PWM_CONTROL,
          BCM2835_PWM1_USEFIFO | 
//          BCM2835_PWM1_REPEATFF |
          BCM2835_PWM1_ENABLE | 
          BCM2835_PWM0_USEFIFO | 
//          BCM2835_PWM0_REPEATFF |
          BCM2835_PWM0_ENABLE | 1<<6);


    PUT32(DMA5_CNTL_BASE + DMA_CNTL_CS, BCM2708_DMA_RESET);
    pause(10);
    PUT32(DMA5_CNTL_BASE + DMA_CNTL_CS, BCM2708_DMA_INT | BCM2708_DMA_END);
    PUT32(DMA5_CNTL_BASE + DMA_CNTL_CONBLK_AD, (uint32_t)&cb_chain[0]);
    PUT32(DMA5_CNTL_BASE + DMA_CNTL_DEBUG, 7); // clear debug error flags
    pause(10);
    PUT32(DMA5_CNTL_BASE + DMA_CNTL_CS, 0x10880001);  // go, mid priority, wait for outstanding writes

//    uart_print("audio init done\r\n");
    pause(1);
}

