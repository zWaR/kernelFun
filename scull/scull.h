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

#ifndef SCULL_MAJOR
#define SCULL_MAJOR 0
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


#endif
