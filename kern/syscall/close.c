#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <synch.h>

/*
The file handle fd is closed. 
The same file handle may then be returned again from open, dup2, pipe, or similar calls.
Other file handles are not affected in any way, even if they are attached to the same file.

On success, return 0
On error, return corresponding error code
*/
int
sys__close(int fd)
{
    if (fd_table_remove(curproc->file_descriptor_table, fd) == -1) return EBADF;
    else return 0;
}