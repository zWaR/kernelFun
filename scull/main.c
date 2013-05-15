//
//  main.c
//  LinuxKernel
//
//  Created by zWaR on 5/13/13.
//
//
//  The code comes from the book "Linux Device Drivers" by Alessandro Rubini and Jonathan Corbet, published
//  by O'Reilly & Associates.

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h> /* dev_t */
#include <linux/kdev_t.h> /* MKDEV */
#include <linux/kernel.h> /* prinkt */
#include <linux/moduleparam.h> /* module_param */
#include <linux/cdev.h>

#include "scull.h"


int scull_major = SCULL_MAJOR;
int scull_minor = 0;
int scull_nr_devs = SCULL_NR_DEVS;


module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_nr_devs, int, S_IRUGO);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("zWaR");

struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .llseek = scull_llseek,
    .read = scull_read,
    .write = scull_write,
    .ioctl = scull_ioctl,
    .open = scull_open,
    .release = scull_release,
};

struct scull_dev *scull_devices;

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
    int err;
    dev_t devno = MKDEV(scull_major, scull_minor+index);
    
    cdev_init(&dev->cdev, &scull_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &scull_fops;
    err = cdev_add(&dev->cdev,devno,1);
    
    if (err)
        printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

int scull_init_module(void)
{
    int result;
    dev_t dev = 0;
    
    if (scull_major)
    {
        dev = MKDEV(scull_major, scull_minor);
        result = register_chrdev_region(dev, scull_nr_devs, "scull");
    }
    else
    {
        result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs, "scull");
        scull_major = MAJOR(dev);
    }
    
    if (result < 0)
    {
        printk(KERN_WARNING "scull: can't get major %d",scull_major);
        return result;
    }
    
    return 0;
}

module_init(scull_init_module);