#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <limits.h>
#include <copyinout.h>

int
sys__open(const char *filename, int flags, int *retval) {
    char *path = kmalloc(PATH_MAX * sizeof(char));
    size_t *length = kmalloc(sizeof(size_t));
    int err = copyinstr((const_userptr_t)filename, path, PATH_MAX, length);
    if (err)
    {
        kfree(path);
        kfree(length);
        return err;
    }
    
    struct vnode *vn;
    err = vfs_open(path, flags, 0, &vn);
    kfree(path);
    kfree(length);
    
    if (err) {
        kfree(vn);
        *retval = -1;
        return err;
    }
    struct file_entry *file_entry = file_entry_create(flags, 0, vn);
    *retval = fd_table_add(curproc->file_descriptor_table, file_entry);
    if (*retval == -1) {
        // fd_table_add will return -1 in the event that the fd table is full
        kprintf("table full\n");
        file_entry_destroy(file_entry);
        kfree(vn);
        return EMFILE;
    } 
    return 0;
}