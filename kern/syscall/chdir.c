#include <syscall.h>
#include <vfs.h>

int 
sys__chdir(const char *pathname)
{
    return vfs_chdir(pathname);
}
