
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define __LIBRARY__
#include <linux/unistd.h>
#include <sys/klog.h>


int main(int argc, char **argv)
{
        int level;
    
        if (argc == 2)
                level = atoi(argv[1]);
        else
        {
                fprintf(stderr, "%s: need a single arg\n", argv[0]);
                exit(1);
        }
    
        if (klogctl(8, NULL, level) < 0)
        {
                fprintf(stderr, "%s: syslog(setlevel): %s\n", argv[0], strerror(errno));
                exit(1);
        }
        exit(0);
}

