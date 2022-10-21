#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>

int
open(const char *filename, int flags, int *retval) {
    struct vnode *vn = NULL;
    int err = vfs_open((char *)filename, flags, 0, &vn);
    if (err == 0) {
        struct file_entry *file_entry = file_entry_create(flags, 0, vn);
        *retval = fd_table_add(curproc->file_descriptor_table, file_entry);
    }
    else {
        kfree(vn);
    }
    return err; // change this to reflect error codes correctly
}