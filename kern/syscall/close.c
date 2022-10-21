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
    *retval = fd_table_remove(curproc->file_descriptor_table, fd);
    if (*retval == -1) return EBADF;
    else return 0;
}