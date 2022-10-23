#include <syscall.h>

int 
sys__lseek(int fd, off_t pos, int whence, int *retval)
{
    (void)fd;
    (void)pos;
    (void)whence;
    *retval = 0;
    return 0;
}