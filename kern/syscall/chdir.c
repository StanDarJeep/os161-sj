#include <filetable.h>
#include <vfs.h>
#include <syscall.h>
#include <copyinout.h>

/*
The current directory of the current process is set to the directory named by pathname.
*/
int 
sys__chdir(const char *pathname)
{
    // We must copyin the pathname parameter userspace address
    char *path = kmalloc(PATH_MAX * sizeof(char));
    size_t *length = kmalloc(sizeof(size_t));
    int err = copyinstr((const_userptr_t)pathname, path, PATH_MAX, length);
    kfree(path);
    kfree(length);
    if (err) return err;

    // Change to the designated directory using vfs_chdir
    return vfs_chdir((char *)pathname);
}
