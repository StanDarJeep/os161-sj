#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>

int
open(const char *filename, int flags) {
    struct vnode *vn = NULL;
    int result = vfs_open((char *)filename, flags, 0, &vn);
    if (result == 0) {
        struct file_entry *file_entry = kmalloc(sizeof(*file_entry));
        file_entry->status = flags;
        file_entry->offset = 0;
        file_entry->file = vn;
        open_file_table_add(&open_file_table, file_entry);
        return fd_table_add(curproc->file_descriptor_table, file_entry);
    }
    return 0; // change this to reflect error codes correctly
}