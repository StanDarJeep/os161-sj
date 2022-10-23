#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <limits.h>
#include <copyinout.h>
#include <synch.h>

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
    struct vnode *vn = NULL;
    err = vfs_open(path, flags, 0, &vn);
    kfree(path);
    kfree(length);
    if (err == 0) {
        struct file_entry *file_entry = file_entry_create(flags, 0, vn);
        lock_acquire(curproc->file_descriptor_table->fd_table_lock);
        *retval = fd_table_add(curproc->file_descriptor_table, file_entry);
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        if (*retval == -1) {
            // fd_table_add will return -1 in the event that the fd table is full
            kfree(vn);
            return EMFILE;
        } 
        return 0;
    }
    else {
        kfree(vn);
        *retval = -1;
        return err;
    }
}