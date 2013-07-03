
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


int main(int argc, char **argv)
{
        char bytes[2] = {11,0};
    
        if (argc == 2)
                bytes[1] = atoi(argv[1]);
        else
        {
                fprintf(stderr, "%s: need a single arg\n", argv[0]);
                exit(1);
        }
    
        if (ioctl(STDIN_FILENO, TIOCLINUX, bytes) < 0)
        {
                fprintf(stderr, "%s: ioctl(stdion, TIOCLINUX): %s\n", argv[0], strerror(errno));
                exit(1);
        }
    
        exit(0);
}
