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
    lock_acquire(curproc->file_descriptor_table->fd_table_lock);
    int i = fd_table_remove(curproc->file_descriptor_table, fd);
    lock_release(curproc->file_descriptor_table->fd_table_lock);
    kprintf("reached: i = %d\n", i);
    if (i == -1) return EBADF;
    else return 0;
}