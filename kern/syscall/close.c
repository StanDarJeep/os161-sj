#include <filetable.h>
#include <vfs.h>
#include <proc.h>
#include <kern/errno.h>
#include <syscall.h>
#include <current.h>

int
sys__close(int fd)
{
    int i = fd_table_remove(curproc->file_descriptor_table, fd);
    if (i == -1) return EBADF;
    else return 0;
}