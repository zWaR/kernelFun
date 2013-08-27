//
//  scull.h
//  LinuxKernel
//
//  Created by zWaR on 5/13/13.
//
//
//  The code comes from the book "Linux Device Drivers" by Alessandro Rubini and Jonathan Corbet, published
//  by O'Reilly & Associates.

#ifndef LinuxKernel_scull_h
#define LinuxKernel_scull_h

#undef PDEBUG
#ifdef SCULL_DEBUG
#   ifdef __KERNEL__
#     define PDEBUG(fmt, args...) printk(KERN_DEBUG "scull: " fmt, ## args)
#   else
#     define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#   endif
#else
#   define PDEBUG(fmt, args...)
#endif

#undef PDEBUG
#define PDEBUG(fmt, args...)

#ifndef SCULL_MAJOR
#define SCULL_MAJOR 80
#endif

#ifndef SCULL_NR_DEVS
#define SCULL_NR_DEVS 4
#endif

#ifndef SCULL_QUANTUM
#define SCULL_QUANTUM 4000
#endif

#ifndef SCULL_QSET
#define SCULL_QSET 1000
#endif

extern int scull_quantum;
extern int scull_qset;

struct scull_dev {
    struct scull_qset *data;    /* Pointer to first quantum set */
    int quantum;                /* the current quantum size */
    int qset;                   /* the current array size */
    unsigned long size;         /* ammount of data stored here */
    unsigned int access_key;    /* used by sculluid and scullpriv */
    struct semaphore sem;       /* mutual exclusion semaphore */
    struct cdev cdev;           /* char device structure */
};

struct scull_qset {
    void **data;
    struct scull_qset *next;
};

loff_t  scull_llseek    (struct file*, loff_t, int);
ssize_t scull_read      (struct file*, char __user*, size_t, loff_t*);
ssize_t scull_write     (struct file*, const char __user*, size_t, loff_t*);
int     scull_ioctl     (struct inode*, struct file*, unsigned int, unsigned long);
int     scull_open      (struct inode*, struct file*);
int     scull_release   (struct inode*, struct file*);
int     scull_trim      (struct scull_dev*);


/*
 * Ioctl definitions
 */

/*Use 0xF5 as magic number */
#define SCULL_IOC_MAGIC 0xF5

#define SCULL_IOCRESET          _IO(SCULL_IOC_MAGIC,    0)

/*
 * S means "Set" through a ptr.
 * T means "Tell" directly with the argument value
 * G means "Get": reply by seting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": switch G ans S atomically
 * H means "sHift": switch T and Q atomically
 */
#define SCULL_IOCSQUANTUM       _IOW(SCULL_IOC_MAGIC,   1, int)
#define SCULL_IOCSQSET          _IOW(SCULL_IOC_MAGIC,   2, int)
#define SCULL_IOCTQUANTUM       _IO(SCULL_IOC_MAGIC,    3)
#define SCULL_IOCTQSET          _IO(SCULL_IOC_MAGIC,    4)
#define SCULL_IOCGQUANTUM       _IOR(SCULL_IOC_MAGIC,   5, int)
#define SCULL_IOCGQSET          _IOR(SCULL_IOC_MAGIC,   6, int)
#define SCULL_IOCQQUANTUM       _IO(SCULL_IOC_MAGIC,    7)
#define SCULL_IOCQQSET          _IO(SCULL_IOC_MAGIC,    8)
#define SCULL_IOCXQUANTUM       _IOWR(SCULL_IOC_MAGIC,  9, int)
#define SCULL_IOCXQSET          _IOWR(SCULL_IOC_MAGIC,  10, int)
#define SCULL_IOCHQUANTUM       _IO(SCULL_IOC_MAGIC,    11)
#define SCULL_IOCHQSET          _IO(SCULL_IOC_MAGIC,    12)

#define SCULL_P_IOCTSIZE        _IO(SCULL_IOC_MAGIC,    13)
#define SCULL_P_IOCQSEIZE       _IO(SCULL_IOC_MAGIC,    14)

#define SCULL_IOC_MAXNR 14


#endif /* _SCULL_H_ */
