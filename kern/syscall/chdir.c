#include <filetable.h>
#include <vfs.h>
#include <syscall.h>
#include <copyinout.h>

int 
sys__chdir(const char *pathname)
{
    char *path = kmalloc(PATH_MAX * sizeof(char));
    size_t *length = kmalloc(sizeof(size_t));
    int err = copyinstr((const_userptr_t)pathname, path, PATH_MAX, length);
    kfree(path);
    kfree(length);
    if (err) return err;
    return vfs_chdir((char *)pathname);
}
