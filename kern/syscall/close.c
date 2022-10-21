#include <filetable.h>
#include <vfs.h>
#include <proc.h>
#include <kern/errno.h>
#include <syscall.h>
#include <current.h>

int
sys__close(int fd, int *retval)
{
    vfs_close(curproc->file_descriptor_table->file_entries[fd]->file);
    int err = fd_table_remove(curproc->file_descriptor_table, fd);
    if (err == -1) *retval = EBADF;
    else *retval = 0;
    return err;
}