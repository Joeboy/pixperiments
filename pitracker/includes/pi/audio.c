#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <pi/registers.h>

extern void PUT32 (uint32_t,uint32_t);
extern uint32_t GET32 (uint32_t);

volatile uint32_t* gpio = (void*)GPIO_BASE;
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))


static struct bcm2708_dma_cb* cb_chain; // TODO: Move this into ringbuffer

typedef struct {
    uint32_t buffer_sz; // in uint32_ts
    uint32_t *buffer;
    uint32_t read_p, write_p;
    uint32_t dmas_per_buffer;
    uint32_t dma_sz;    // in uint32_ts
//    void *dma_cb_container;
//    struct bcm2708_dma_cb *dma_cb_chain;
} ringbuffer;

static ringbuffer buf;

void init_ringbuffer(uint32_t buffer_sz, uint32_t dmas_per_buffer) {
    buf.buffer_sz = buffer_sz;
    buf.buffer = malloc(buffer_sz * sizeof(uint32_t));
    buf.dmas_per_buffer = dmas_per_buffer;
    buf.dma_sz = (float)buffer_sz / dmas_per_buffer;// Vile hack - TODO: fix
    buf.read_p = 0;
    buf.write_p = 0;
    for (uint32_t i=0;i<buffer_sz;i++) buf.buffer[i]=512;
}

int32_t audio_buffer_free_space() {
    // Return the number of uint32_ts that can be safely written to the buffer
    buf.read_p = buf.dma_sz * ((struct bcm2708_dma_cb*)GET32(DMA5_CNTL_BASE + BCM2708_DMA_ADDR) - cb_chain);
    uint32_t spare;
    if (buf.read_p >= buf.write_p) {
        spare = buf.read_p - buf.write_p;
    } else {
        spare = buf.buffer_sz - (buf.write_p - buf.read_p);
    }
    return spare;
}

void audio_buffer_write(float *audio_buf_left, float *audio_buf_right, size_t size) {
    for (uint32_t i=0;i<size;i++) {
        buf.buffer[buf.write_p] = (uint32_t)(512.0+512.0*audio_buf_left[i]);
        buf.write_p++;
        buf.buffer[buf.write_p] = (uint32_t)(512.0+512.0*audio_buf_right[i]);
        buf.write_p++;
        if (buf.write_p >= buf.buffer_sz) buf.write_p = 0;
    }
}


int32_t audio_init(void) {
    init_ringbuffer(256, 16); // 128 byte ringbuffer, split over 256 dma windows
    // Set up the pwm clock
    // vals read from raspbian:
    // PWMCLK_CNTL = 148 = 10010100 - 100 is allegedly 'plla' but I can't make that work
    // PWMCLK_DIV = 16384
    // PWM_CONTROL=9509 = 10010100100101
    // PWM0_RANGE=1024
    // PWM1_RANGE=1024
    uint32_t range = 0x400;
    uint32_t idiv = 8;
    SET_GPIO_ALT(40, 0); // set pins 40/45 (aka phone jack) to pwm function
    SET_GPIO_ALT(45, 0);
    usleep(10); // I don't know if all these usleeps are really necessary

    PUT32(CLOCK_BASE + 4*BCM2835_PWMCLK_CNTL, PM_PASSWORD | BCM2835_PWMCLK_CNTL_KILL);
    PUT32(PWM_BASE + 4*BCM2835_PWM_CONTROL, 0);

    uint32_t samplerate = 500000000.0 / idiv / range / 2; // 2 channels
    printf("samplerate=");
    dump_int_hex(samplerate);
    printf("\r\n");
    PUT32(CLOCK_BASE + 4*BCM2835_PWMCLK_DIV, PM_PASSWORD | (idiv<<12));
    
    PUT32(CLOCK_BASE + 4*BCM2835_PWMCLK_CNTL,
                                PM_PASSWORD | 
                                BCM2835_PWMCLK_CNTL_ENABLE |
                                BCM2835_PWMCLK_CNTL_PLLD);
    usleep(1);
    PUT32(PWM_BASE + 4*BCM2835_PWM0_RANGE, range);
    PUT32(PWM_BASE + 4*BCM2835_PWM1_RANGE, range);
    usleep(1);

    // TODO: Move these into ringbuffer struct and kill some globals
    // Ensure buffer is 32-byte aligned
    void* cb_container = malloc(0x20 + sizeof(struct bcm2708_dma_cb) * buf.dmas_per_buffer);
    uint32_t offset = (uint32_t)cb_container & 0x1f;
    cb_chain = cb_container + 0x20 - offset;

    uint32_t i;
    for (i=0;i<buf.dmas_per_buffer;i++) {
        cb_chain[i].info   = BCM2708_DMA_S_INC | BCM2708_DMA_NO_WIDE_BURSTS | BCM2708_DMA_WAIT_RESP | BCM2708_DMA_D_DREQ | BCM2708_DMA_PER_MAP(5);
        cb_chain[i].src = (uint32_t)(buf.buffer + i * buf.dma_sz);
        cb_chain[i].dst = ((PWM_BASE+4*BCM2835_PWM_FIFO) & 0x00ffffff) | 0x7e000000; // physical address of fifo
        cb_chain[i].length = sizeof(uint32_t) * buf.dma_sz;
        cb_chain[i].next = (uint32_t)&cb_chain[i+1];
    }
    cb_chain[i-1].next = (uint32_t)&cb_chain[0];
    usleep(1);
    PUT32(PWM_BASE + 4*BCM2835_PWM_DMAC, PWMDMAC_ENAB | PWMDMAC_THRSHLD);
    usleep(1);
    PUT32(PWM_BASE + 4*BCM2835_PWM_CONTROL, PWMCTL_CLRF);
    usleep(1);

    PUT32(PWM_BASE + 4*BCM2835_PWM_CONTROL,
          BCM2835_PWM1_USEFIFO | 
//          BCM2835_PWM1_REPEATFF |
          BCM2835_PWM1_ENABLE | 
          BCM2835_PWM0_USEFIFO | 
//          BCM2835_PWM0_REPEATFF |
          BCM2835_PWM0_ENABLE | 1<<6);


    PUT32(DMA5_CNTL_BASE + BCM2708_DMA_CS, BCM2708_DMA_RESET);
    usleep(10);
    PUT32(DMA5_CNTL_BASE + BCM2708_DMA_CS, BCM2708_DMA_INT | BCM2708_DMA_END);
    PUT32(DMA5_CNTL_BASE + BCM2708_DMA_ADDR, (uint32_t)&cb_chain[0]);
    PUT32(DMA5_CNTL_BASE + BCM2708_DMA_DEBUG, 7); // clear debug error flags
    usleep(10);
    PUT32(DMA5_CNTL_BASE + BCM2708_DMA_CS, 0x10880001);  // go, mid priority, wait for outstanding writes

//    printf("audio init done\r\n");
    usleep(1);
    return samplerate;
}

