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

#include "scull.h"


int scull_major = SCULL_MAJOR;
int scull_minor = 0;
int scull_nr_devs = SCULL_NR_DEVS;


module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_nr_devs, int, S_IRUGO);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("zWaR");


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