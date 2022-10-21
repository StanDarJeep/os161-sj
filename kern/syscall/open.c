#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>

int
open(const char *filename, int flags, int *retval) {
    struct vnode *vn = NULL;
    int err = vfs_open((char *)filename, flags, 0, &vn);
    if (err == 0) {
        struct file_entry *file_entry = file_entry_create(flags, 0, vn);
        int add_result = fd_table_add(curproc->file_descriptor_table, file_entry);
        if (add_result == -1) {
            // fd_table_add will return -1 in the event that the fd table is full
            kfree(vn);
            return EMFILE;
        } 
        else *retval = add_result;
    }
    else {
        kfree(vn);
    }
    return err; // change this to reflect error codes correctly
}