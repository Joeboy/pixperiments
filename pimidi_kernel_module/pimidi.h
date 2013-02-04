#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
#define AUX_BASE                (BCM2708_PERI_BASE + 0x215000) /* GPIO controller */

#define AUX_ENABLES     (0x004 / 4)
#define AUX_MU_IO_REG   (0x040 / 4)
#define AUX_MU_IER_REG  (0x044 / 4)
#define AUX_MU_IIR_REG  (0x048 / 4)
#define AUX_MU_LCR_REG  (0x04C / 4)
#define AUX_MU_MCR_REG  (0x050 / 4)
#define AUX_MU_LSR_REG  (0x054 / 4)
#define AUX_MU_MSR_REG  (0x058 / 4)
#define AUX_MU_SCRATCH  (0x05C / 4)
#define AUX_MU_CNTL_REG (0x060 / 4)
#define AUX_MU_STAT_REG (0x064 / 4)
#define AUX_MU_BAUD_REG (0x068 / 4)
#define GPFSEL1 (0x004 / 4)
#define GPSET0  (0x01C / 4)
#define GPCLR0  (0x028 / 4)
#define GPPUD       (0x094 / 4)
#define GPPUDCLK0   (0x098 / 4)

#define DEVICE_NAME "pimidi"   /* Dev name as it appears in /proc/devices   */

