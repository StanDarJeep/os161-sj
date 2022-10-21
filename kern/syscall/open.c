#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>

int
sys__open(const char *filename, int flags, int *retval) {
    struct vnode *vn = NULL;
    int err = vfs_open((char *)filename, flags, 0, &vn);
    if (err == 0) {
        struct file_entry *file_entry = file_entry_create(flags, 0, vn);
        *retval = fd_table_add(curproc->file_descriptor_table, file_entry);
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