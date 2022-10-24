#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <synch.h>

int
sys__close(int fd)
{
    if (fd_table_remove(curproc->file_descriptor_table, fd) == -1) return EBADF;
    else return 0;
}