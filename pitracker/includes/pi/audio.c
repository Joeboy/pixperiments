#include <malloc.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pi/registers.h>

extern void PUT32 (uint32_t,uint32_t);
extern uint32_t GET32 (uint32_t);

#define SET_GPIO_ALT(g,a) *((uint32_t*)GPIO_BASE+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))


typedef struct {
    uint32_t buffer_sz; // in uint32_ts
    uint32_t *buffer;
    uint32_t read_p, write_p;
    void *dma_cb_container;
    struct bcm2708_dma_cb *dma_cb;
} ringbuffer_t;

static ringbuffer_t *buf;

ringbuffer_t * new_ringbuffer(uint32_t buffer_sz) {
    ringbuffer_t *rb = malloc(sizeof(ringbuffer_t));
    rb->buffer_sz = buffer_sz;
    rb->buffer = malloc(buffer_sz * sizeof(uint32_t));
    rb->read_p = 0;
    rb->write_p = 0;
    for (uint32_t i=0;i<buffer_sz;i++) rb->buffer[i] = 512;
    rb->dma_cb_container = malloc(0x20 + sizeof(struct bcm2708_dma_cb));
    uint32_t offset = (uint32_t)rb->dma_cb_container & 0x1f;
    rb->dma_cb = rb->dma_cb_container + 0x20 - offset;
    return rb;
}

void delete_ringbuffer(ringbuffer_t *rb) {
    free(rb->buffer);
    free(rb->dma_cb_container);
    free(rb);
}

int32_t audio_buffer_free_space() {
    // Return the number of uint32_ts that can be safely written to the buffer
    buf->read_p = ((uint32_t*)GET32(DMA5_CNTL_BASE + 0x0c) - buf->buffer);
    uint32_t spare;
    if (buf->read_p >= buf->write_p) {
        spare = buf->read_p - buf->write_p;
    } else {
        spare = buf->buffer_sz - (buf->write_p - buf->read_p);
    }
    return spare;
}

void audio_buffer_write(float *audio_buf_left, float *audio_buf_right, size_t size) {
    for (uint32_t i=0;i<size;i++) {
        buf->buffer[buf->write_p] = (uint32_t)(512.0+512.0*audio_buf_left[i]);
        buf->write_p++;
        buf->buffer[buf->write_p] = (uint32_t)(512.0+512.0*audio_buf_right[i]);
        buf->write_p++;
        if (buf->write_p >= buf->buffer_sz) buf->write_p = 0;
    }
}


int32_t audio_init(void) {
    buf = new_ringbuffer(256);
    // Set up the pwm clock
    // vals read from raspbian:
    // PWMCLK_CNTL = 148 = 10010100 - 100 is allegedly 'plla' but I can't make that work
    // PWMCLK_DIV = 16384
    // PWM_CONTROL=9509 = 10010100100101
    // PWM0_RANGE=1024
    // PWM1_RANGE=1024
    uint32_t range = 0x400;
    uint32_t idiv = 12;
    SET_GPIO_ALT(40, 0); // set pins 40/45 (aka phone jack) to pwm function
    SET_GPIO_ALT(45, 0);
    usleep(10); // I don't know if all these usleeps are really necessary

    PUT32(CLOCK_BASE + 4*BCM2835_PWMCLK_CNTL, PM_PASSWORD | BCM2835_PWMCLK_CNTL_KILL);
    PUT32(PWM_BASE + 4*BCM2835_PWM_CONTROL, 0);

    // In theory this should be divided by 2 again for the two channels, but
    // that seems to lead to the wrong rate. TODO: investigate
    uint32_t samplerate = 500000000.0 / idiv / range;

    PUT32(CLOCK_BASE + 4*BCM2835_PWMCLK_DIV, PM_PASSWORD | (idiv<<12));
    
    PUT32(CLOCK_BASE + 4*BCM2835_PWMCLK_CNTL,
                                PM_PASSWORD | 
                                BCM2835_PWMCLK_CNTL_ENABLE |
                                BCM2835_PWMCLK_CNTL_PLLD);
    usleep(1);
    PUT32(PWM_BASE + 4*BCM2835_PWM0_RANGE, range);
    PUT32(PWM_BASE + 4*BCM2835_PWM1_RANGE, range);
    usleep(1);

    buf->dma_cb->info   = BCM2708_DMA_S_INC | BCM2708_DMA_WAIT_RESP | BCM2708_DMA_D_DREQ | BCM2708_DMA_PER_MAP(5);
    buf->dma_cb->src = (uint32_t)(buf->buffer);
    buf->dma_cb->dst = ((PWM_BASE+4*BCM2835_PWM_FIFO) & 0x00ffffff) | 0x7e000000; // physical address of fifo
    buf->dma_cb->length = sizeof(uint32_t) * buf->buffer_sz;
    buf->dma_cb->next = (uint32_t)buf->dma_cb;
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
    PUT32(DMA5_CNTL_BASE + BCM2708_DMA_ADDR, (uint32_t)buf->dma_cb);
    PUT32(DMA5_CNTL_BASE + BCM2708_DMA_DEBUG, 7); // clear debug error flags
    usleep(10);
    PUT32(DMA5_CNTL_BASE + BCM2708_DMA_CS, 0x10880001);  // go, mid priority, wait for outstanding writes

//    printf("audio init done\r\n");
    usleep(1);
    return samplerate;
}

