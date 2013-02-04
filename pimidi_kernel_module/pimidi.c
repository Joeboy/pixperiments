/* 
 * pimidi - read/write uart0 at nonstandard midi rate (31250Hz)
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "pimidi.h"


#define SUCCESS 0

static int Major;
static int Device_Open = 0; // Is device open? Used to prevent multiple access to device

static volatile unsigned int *gpio;
static volatile unsigned int *aux;


static int device_open(struct inode *inode, struct file *file)
{
    if (Device_Open)
        return -EBUSY;

    Device_Open++;
    try_module_get(THIS_MODULE);

    return SUCCESS;
}


static int device_release(struct inode *inode, struct file *file)
{
    Device_Open--;
    module_put(THIS_MODULE);

    return 0;
}


static ssize_t device_read(struct file *filp,   /* see include/linux/fs.h   */
               char *buffer,    /* buffer to fill with data */
               size_t length,   /* length of the buffer     */
               loff_t * offset)
{
    int bytes_read = 0;
    char b;
    
    while (readl(aux + AUX_MU_LSR_REG) & 1) {
        b = readl(aux + AUX_MU_IO_REG);
        put_user(b, buffer++);
        bytes_read++;
    }
    return bytes_read;
}


static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
    // TODO
}


static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};


int init_module(void)
{    
    unsigned int a;
    gpio = ioremap(GPIO_BASE, 1024);
    aux = ioremap(AUX_BASE, 1024);

    //GPIO14  TXD0 and TXD1
    //GPIO15  RXD0 and RXD1
    //alt function 5 for uart1
    //alt function 0 for uart0
    //((250,000,000/115200)/8)-1 = 270

    writel(1, aux + AUX_ENABLES);
    writel(0, aux + AUX_MU_IER_REG);
    writel(0, aux + AUX_MU_CNTL_REG);
    writel(3, aux + AUX_MU_LCR_REG);
    writel(0, aux + AUX_MU_MCR_REG);
    writel(0, aux + AUX_MU_IER_REG);
    writel(0xc6, aux + AUX_MU_IIR_REG);
    writel(999, aux + AUX_MU_BAUD_REG); // 270 == console, 999=midi

    a = *(gpio + GPFSEL1);
    a &= ~(7<<12); //gpio14
    a |= 2<<12;    //alt5
    a &= ~(7<<15); //gpio15
    a |= 2<<15;    //alt5
    writel(a, gpio + GPFSEL1);

    writel(0, gpio + GPPUD);

    msleep(100);
    writel((1<<14)|(1<<15), gpio + GPPUDCLK0);
    msleep(100);
    writel(0, gpio + GPPUDCLK0);

    writel(3, aux + AUX_MU_CNTL_REG);

    Major = register_chrdev(0, DEVICE_NAME, &fops);

    if (Major < 0) {
      printk(KERN_ALERT "Registering char device failed with %d\n", Major);
      return Major;
    }

    printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
    printk(KERN_INFO "the driver, create a dev file with\n");
    printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);

    return SUCCESS;
}


void cleanup_module(void)
{
    unregister_chrdev(Major, DEVICE_NAME);
}

