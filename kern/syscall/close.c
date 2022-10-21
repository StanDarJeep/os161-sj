#include <filetable.h>
#include <vfs.h>
#include <proc.h>

int
sys__close(int fd, int *retval)
{
    int result = vfs_close(curproc->file_descriptor_table[fd]->file);
    if (result == 0) fd_table_remove(curproc->file_descriptor_table, fd);
    return result;
}