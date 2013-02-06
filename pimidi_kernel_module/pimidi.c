/* 
 * pimidi - read/write uart0 at nonstandard midi rate (31250Hz)
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "pimidi.h"


#define SUCCESS 0

static unsigned int pimidi_major;
static struct class *pimidi_class = NULL;
static struct cdev cdev;

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
    // I have no means of testing this, so it probably doesn't work.
    int bytes_written = 0;
    while (bytes_written < len) {
        while (1) {
            if (readl(aux + AUX_MU_LSR_REG) &0x20) break;
        }
	put_user(buff[bytes_written++], aux + AUX_MU_IO_REG);
    }
    return bytes_written;
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

    dev_t dev = 0;
    int err = 0;

    err = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (err < 0) {
        printk(KERN_WARNING "alloc_chrdev_region() failed\n");
        return err;
    }
    pimidi_major = MAJOR(dev);
    pimidi_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(pimidi_class)) {
        err = PTR_ERR(pimidi_class);
        goto fail;
    }

    err = 0;
    dev_t devno = MKDEV(pimidi_major, 1);
    struct device *device = NULL;
    
    BUG_ON(pimidi_class == NULL);
    
    cdev_init(&cdev, &fops);
    cdev.owner = THIS_MODULE;
    
    err = cdev_add(&cdev, devno, 1);
    if (err) {
        printk(KERN_WARNING "[target] Error %d while trying to add %s%d",
               err, DEVICE_NAME, 1);
        return err;
    }
    
    device = device_create(pimidi_class, NULL, /* no parent device */ 
    devno, NULL, /* no additional data */
    DEVICE_NAME "%d", 1);
    
    if (IS_ERR(device)) {
        err = PTR_ERR(device);
        printk(KERN_WARNING "[target] Error %d while trying to create %s%d",
               err, DEVICE_NAME, 1);
        cdev_del(&cdev);
        return err;
    }


    printk(KERN_INFO "Loaded pimidi uart driver");
    return 0; /* success */
    
    fail:
        cleanup_module();
        return err;
}


void cleanup_module(void)
{
    device_destroy(pimidi_class, MKDEV(pimidi_major, 1));
    cdev_del(&cdev);
    if (pimidi_class) class_destroy(pimidi_class);
    unregister_chrdev_region(MKDEV(pimidi_major, 0), 1);
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Joe Button");
MODULE_DESCRIPTION("Read/write uart0 at nonstandard midi rate (31250Hz)");
