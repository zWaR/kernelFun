//
//  main.c
//  LinuxKernel
//
//  Created by zWaR on 5/13/13.
//
//
//  The code comes from the book "Linux Device Drivers" by Alessandro Rubini and
//  Jonathan Corbet, published
//  by O'Reilly & Associates.

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h> /* dev_t */
#include <linux/kdev_t.h> /* MKDEV */
#include <linux/kernel.h> /* prinkt */
#include <linux/moduleparam.h> /* module_param */
#include <linux/cdev.h> /* char device registration */
#include <linux/slab.h> /* kfree and kmalloc */
#include <asm/uaccess.h> /* copy_to_user and copy_from_user */
#include <linux/proc_fs.h> /* used to create files in /proc */
#include <linux/seq_file.h> /* needed for seq_file */
#include <linux/ioctl.h> /* needed for ioctl obviously */
#include <asm/uaccess.h> /* needed for access_ok */
#include <linux/capability.h> /* needed for capability */
#include <linux/sched.h> /* needed for capable */

#include "scull.h"


int scull_major = SCULL_MAJOR;
int scull_minor = 0;
int scull_nr_devs = SCULL_NR_DEVS;
int scull_quantum = SCULL_QUANTUM;
int scull_qset = SCULL_QSET;


module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_nr_devs, int, S_IRUGO);
module_param(scull_quantum, int , S_IRUGO);
module_param(scull_qset, int, S_IRUGO);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("zWaR");

struct file_operations scull_fops = {
    .owner = THIS_MODULE,
//    .llseek = scull_llseek,
    .read = scull_read,
    .write = scull_write,
//    .ioctl = scull_ioctl,
    .open = scull_open,
    .release = scull_release,
};

struct scull_dev *scull_devices;

//char device registration
static void scull_setup_cdev(struct scull_dev *dev, int index)
{
        int err;
        dev_t devno = MKDEV(scull_major, scull_minor + index);
    
        cdev_init(&dev->cdev, &scull_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &scull_fops;
        err = cdev_add(&dev->cdev, devno, 1);
    
        if (err)
                printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

//open method
int scull_open (struct inode *inode, struct file *filp)
{
        struct scull_dev *dev;
    
        dev = container_of (inode->i_cdev, struct scull_dev, cdev);
        filp->private_data = dev; /* for other methods */
    
        if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
                scull_trim(dev);
        }
    
        return 0;
}

//relase method
int scull_release (struct inode *inode, struct file *filp)
{
        return 0;
}

//free scull data area (quantum and quantum set)
int scull_trim(struct scull_dev *dev)
{
        struct scull_qset *next, *dptr;
        int qset = dev->qset;
        int i;
    
        for (dptr = dev->data; dptr; dptr = next)
        {
                if (dptr->data)
                {
                        for (i = 0; i < qset; i++)
                                kfree(dptr->data[i]);
                        kfree(dptr->data);
                        dptr->data = NULL;
                }
                next = dptr->next;
                kfree(dptr);
        }
        dev->size = 0;
        dev->qset = scull_qset;
        dev->quantum = scull_quantum;
        dev->data = NULL;
        return 0;
}

#ifdef SCULL_DEBUG
/*
 * The proc filesystem function to read and entry
 */

int scull_read_procmem(char *buf, char **start, off_t offset,
                       int count, int *eof, void *data)
{
        int i, j, len = 0;
        int limit = count - 80;
        
        for (i = 0; i < scull_nr_devs && len <= limit; i++) {
                struct scull_dev *d = &scull_devices[i];
                struct scull_qset *qs = d->data;
                if (down_interruptible(&d->sem))
                        return -ERESTARTSYS;
                
                len += sprintf(buf+len, "\nDevice %i: qset %i, q %i, sz %li\n",
                               i, d->qset, d->quantum, d->size);
                
                for (; qs && len <= limit; qs = qs->next) {
                        len +=  sprintf(buf+len, " item at %p, qset at %p\n",
                                        qs, qs->data);
                        if (qs->data && !qs->next)                                
                                for (j=0; j < d->qset; j++) {
                                        if (qs->data[j])
                                                len += sprintf(buf+len,
                                                               "    % 4i: %8p\n",
                                                               j, qs->data[j]);
                                }
                }
                up(&scull_devices[i].sem);
        }
        *eof = 1;
        return len;
}

static void *scull_seq_start(struct seq_file *s, loff_t *pos)
{
        if (*pos >= scull_nr_devs)
                return NULL;
        return scull_devices + *pos;
}

static void *scull_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
        (*pos)++;
        if (*pos >= scull_nr_devs)
                return NULL;
        
        return scull_devices + *pos;
}

static void scull_seq_stop(struct seq_file *s, void *v)
{
        /* Nothing to do for us */
}

static int scull_seq_show(struct seq_file *s, void *v)
{
        struct scull_dev *dev = (struct scull_dev *) v;
        struct scull_qset *d;
        int i;
        
        if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;
        
        seq_printf(s, "\nDevice %i: qset %i, q %i, sz %li\n",
                   (int) (dev - scull_devices), dev->qset,
                   dev->quantum, dev->size);
        
        for (d = dev->data; d; d = d->next) {
                seq_printf(s, "item at %p, qset at %p\n", d, d->data);
                if (d->data && !d->next)
                        for (i=0; i < dev->qset; i++) {
                                if (d->data[i])
                                        seq_printf(s, "    % 4i: %8p\n",
                                                   i, d->data[i]);
                        }
        }
        up(&dev->sem);
        return 0;
}

static struct seq_operations scull_seq_ops = {
        .start  = scull_seq_start,
        .next   = scull_seq_next,
        .stop   = scull_seq_stop,
        .show   = scull_seq_show,
};

static int scull_proc_open(struct inode *inode, struct file *file)
{
        return seq_open(file, &scull_seq_ops);
}

static struct file_operations scull_proc_ops = {
        .owner          = THIS_MODULE,
        .open           = scull_proc_open,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = seq_release
};

static void scull_create_proc(void)
{
        struct proc_dir_entry *entry;
        create_proc_read_entry("scullmem", 0 /* default mode */,
                               NULL /* parent dir */, scull_read_procmem,
                               NULL /* client data */);
        entry = create_proc_entry("scullseq", 0, NULL);
        if (entry)
                entry->proc_fops = &scull_proc_ops;
}

static void scull_remove_proc(void)
{
        remove_proc_entry("scullmem", NULL /* parent dir */);
}


#endif /* SCULL_DEBUG */

/*
 * Follow the list
 */
struct scull_qset *scull_follow(struct scull_dev *dev, int n)
{
	struct scull_qset *qs = dev->data;
    
        /* Allocate first qset explicitly if need be */
	if (! qs)
        {
                qs = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		if (qs == NULL)
			return NULL;  /* Never mind */
		memset(qs, 0, sizeof(struct scull_qset));
	}
    
	/* Then follow the list */
	while (n--) {
		if (!qs->next) {
			qs->next = kmalloc(sizeof(struct scull_qset),
                                           GFP_KERNEL);
			if (qs->next == NULL)
				return NULL;  /* Never mind */
			memset(qs->next, 0, sizeof(struct scull_qset));
		}
		qs = qs->next;
		continue;
	}
	return qs;
}

ssize_t scull_read(struct file *filp, char __user *buf,
                   size_t count, loff_t *f_pos)
{
        struct scull_dev *dev = filp->private_data;
        struct scull_qset *dptr;  /* the first listitem */
        int quantum = dev->quantum, qset = dev->qset;
        int itemsize = quantum * qset;   /* how many bytes in the listitem */
        int item, s_pos, q_pos, rest;
        ssize_t retval = 0;
    
        if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;
        if (*f_pos >= dev->size)
                goto out;
        if (*f_pos + count > dev->size)
                count = dev->size - *f_pos;
    
        /* find listitem, qset index, and offset in the quantum */
        item = (long)*f_pos / itemsize;
        rest = (long)*f_pos % itemsize;
        s_pos = rest / quantum;
        q_pos = rest % quantum;
    
        /* follow the list up to the right position (defined elsewhere) */
        dptr = scull_follow(dev, item);
    
        if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
                goto out; /* don't fill holes */
    
        /* read only up tp the end of this quantum */
        if (count > quantum - q_pos)
                count = quantum - q_pos;
    
        if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count))
        {
                retval = -EFAULT;
                goto out;
        }
        //printk(KERN_WARNING "Scull: read some data\n");
        *f_pos += count;
        retval = count;
    
out:
        up(&dev->sem);
        return retval;
}

ssize_t scull_write(struct file *filp, const char __user *buf,
                    size_t count, loff_t *f_pos)
{
        struct scull_dev *dev = filp->private_data;
        struct scull_qset *dptr;
        int quantum = dev->quantum, qset = dev->qset;
        int itemsize = quantum * qset;
        int item, s_pos, q_pos, rest;
        ssize_t retval = -ENOMEM;  /* value used in "goto out" statements */
    
    
        if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;
    
        printk(KERN_WARNING "Scull: itemsize: %d\ncount: %d\n",
               itemsize, count);
    
        /* find listitem, qset index and offset in the quantum */
        item = (long)*f_pos / itemsize;
        rest = (long)*f_pos % itemsize;
        s_pos = rest / quantum;
        q_pos = rest % quantum;
    
        /* follow the list up to the right position */
        dptr = scull_follow(dev, item);
        if (dptr == NULL)
                goto out;
        if (!dptr->data)
        {
                dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
                if (!dptr->data)
                        goto out;
                memset(dptr->data, 0, qset * sizeof(char *));
        }
        if (!dptr->data[s_pos])
        {
                dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
                if (!dptr->data[s_pos])
                        goto out;
        }
        /* write only up to the end of this quantum */
        if (count > quantum - q_pos)
                count = quantum - q_pos;
    
        if (copy_from_user(dptr->data[s_pos] + q_pos, buf, count))
        {
                retval = -EFAULT;
                goto out;
        }
        //printk(KERN_WARNING "Scull: wrote a bunch of data: %s\n", buf);
        *f_pos += count;
        retval = count;
    
        /* update the size */
        if (dev->size < *f_pos)
                dev->size = *f_pos;
    
out:
        up(&dev->sem);
        return retval;
}

int scull_ioctl (struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
        int err = 0, tmp;
        int retval = 0;
        
        if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
        if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;
        
        if (_IOC_TYPE(cmd) & _IOC_READ)
                err = !access_ok(VERFIY_WRITE, (int __user *) arg, _IOC_SIZE(cmd));
        if (_IOC_TYPE(cmd) & _IOC_WRITE)
                err = !access_ok(VERIFY_READ, (int __user *) arg, _IOC_SIZE(cmd));
        
        if (err) return -EFAULT;
        
        switch (cmd)
        {
                case SCULL_IOCRESET:
                        scull_quantum = SCULL_QUANTUM;
                        scull_qset = SCULL_QSET;
                        break;
                        
                case SCULL_IOCSQUANTUM:
                        if ( !capable(CAP_SYS_ADMIN) )
                                return -EPERM;
                        retval = __get_user(scull_quantum, (int __user *) arg);
                        break;
                
                case SCULL_IOCSQSET:
                        if ( !capable(CAP_SYS_ADMIN) )
                                return -EPERM;
                        retval = __get_user(scull_qset, (int __user *) arg);
                        break;
                        
                case SCULL_IOCTQUANTUM:
                        if ( !capable(CAP_SYS_ADMIN) )
                                return -EPERM;
                        scull_quantum = arg;
                        break;
                        
                case SCULL_IOCTQSET:
                        if ( !capable(CAP_SYS_ADMIN) )
                                return -EPERM;
                        scull_qset = arg;
                        break;
                        
                case SCULL_IOCGQUANTUM:
                        retval = __put_user(scull_quantum, (int __user *) arg);
                        break;
                        
                case SCULL_IOCGQSET:
                        retval = __put_user(scull_qset, (int __user *) arg);
                        break;
                        
                case SCULL_IOCQQUANTUM:
                        return scull_quantum;
                        
                case SCULL_IOCQQSET:
                        return scull_qset;
                        
                case SCULL_IOCXQUANTUM:
                        if ( !capable(CAP_SYS_ADMIN) )
                                return -EPERM;
                        
                        tmp = scull_quantum;
                        retval = __get_user(scull_quantum, (int __user *) arg);
                        if (retval == 0)
                                retval = __put_user(tmp, (int __user *) arg);
                        break;
                        
                case SCULL_IOCXQSET:
                        if ( !capable(CAP_SYS_ADMIN) )
                                return -EPERM;
                        
                        tmp =  scull_qset;
                        retval = __get_user(scull_qset, (int __user *) arg);
                        if (retval == 0)
                                retval = __put_user(tmp, (int __user *) arg);
                        break;
                        
                case SCULL_IOCHQUANTUM:
                        if ( !capable(CAP_SYS_ADMIN) )
                                return -EPERM;
                        
                        tmp = scull_quantum;
                        scull_quantum = arg;
                        return tmp;
                        
                case SCULL_IOCHQSET:
                        if ( !capable(CAP_SYS_ADMIN) )
                                return -EPERM;
                        
                        tmp = scull_qset;
                        scull_qset = arg;
                        return tmp;
                        
                default:
                        return -ENOTTY;
                        
        }
        
        return retval;
}

void scull_cleanup_module(void)
{
        int i;
        dev_t devno = MKDEV(scull_major, scull_minor);
    
        if (scull_devices)
        {
                for (i = 0; i < scull_nr_devs; i++)
                {
                        scull_trim(scull_devices + i);
                        cdev_del(&scull_devices[i].cdev);
                }
                kfree(scull_devices);
        }
        
#ifdef SCULL_DEBUG
        scull_remove_proc();
#endif
    
        unregister_chrdev_region(devno, scull_nr_devs);
    
        //scull_p_cleanup();
        //scull_access_cleanup();
}

int scull_init_module(void)
{
        int result,i ;
        dev_t dev = 0;
    
        if (scull_major)
        {
                dev = MKDEV(scull_major, scull_minor);
                result = register_chrdev_region(dev, scull_nr_devs, "scull");
        }
        else
        {
                result = alloc_chrdev_region(
                                &dev, scull_minor, scull_nr_devs, "scull");
                scull_major = MAJOR(dev);
        }
    
        if (result < 0)
        {
                printk(KERN_WARNING "scull: can't get major %d",scull_major);
                return result;
        }
    
        /*
         * allocate the devices
         */
    
        scull_devices = kmalloc(scull_nr_devs *
                                sizeof(struct scull_dev), GFP_KERNEL);
        if (!scull_devices)
        {
                result = -ENOMEM;
                goto fail;
        }
        memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));
    
        for (i = 0; i < scull_nr_devs; i++)
        {
                scull_devices[i].quantum = scull_quantum;
                scull_devices[i].qset = scull_qset;
                init_MUTEX(&scull_devices[i].sem);
                scull_setup_cdev(&scull_devices[i], i);
        }
    
        //dev = MKDEV(scull_major, scull_minor + scull_nr_devs);
        //dev += scull_p_init(dev);
        //dev += scull_access_init(dev);
        
#ifdef SCULL_DEBUG
        scull_create_proc();
#endif
    
        return 0;
    
fail:
        scull_cleanup_module();
        return result;
}

module_init(scull_init_module);
module_exit(scull_cleanup_module);