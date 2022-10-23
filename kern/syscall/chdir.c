#include <syscall.h>
#include <vfs.h>

int 
sys__chdir(const char *pathname, int *retval)
{
    int err = vfs_chdir(pathname);
    if (err == 0) {
        *retval = 0;
    }
    else {
        *retval = -1;
    }
    return err;
}
