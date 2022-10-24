#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <kern/seek.h>
#include <limits.h>
#include <synch.h>
#include <kern/stat.h>
#include <vnode.h>

int 
sys__lseek(int fd, off_t pos, int whence, int64_t *retval)
{
    kprintf("fd is: %d\n", fd);
    lock_acquire(curproc->file_descriptor_table->fd_table_lock);
    if (fd >= OPEN_MAX || fd < 0 || curproc->file_descriptor_table->count[fd] != 1 ) {
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        *retval = -1;
        return EBADF;
    }
    if (fd < 3) {
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        *retval = -1;
        return ESPIPE;
    }
    if (whence == SEEK_SET) {
        if (pos < 0) {
            *retval = -1;
            return EINVAL;
        }
        lock_acquire(open_file_table.open_file_table_lock);
        curproc->file_descriptor_table->file_entries[fd]->offset = pos;
        *retval = curproc->file_descriptor_table->file_entries[fd]->offset;
        lock_release(open_file_table.open_file_table_lock);
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return 0;
    }
    else if (whence == SEEK_CUR) {
        lock_acquire(open_file_table.open_file_table_lock);
        if (curproc->file_descriptor_table->file_entries[fd]->offset + pos < 0) {
            lock_release(open_file_table.open_file_table_lock);
            lock_release(curproc->file_descriptor_table->fd_table_lock);
            *retval = -1;
            return EINVAL;
        }
        curproc->file_descriptor_table->file_entries[fd]->offset += pos;
        *retval = curproc->file_descriptor_table->file_entries[fd]->offset;
        lock_release(open_file_table.open_file_table_lock);
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return 0;
    }
    else if (whence == SEEK_END) {
        struct stat *stat = kmalloc(sizeof(struct stat));;
        lock_acquire(open_file_table.open_file_table_lock);
        VOP_STAT(curproc->file_descriptor_table->file_entries[fd]->file, stat);
        if (stat->st_size + pos < 0) {
            lock_release(open_file_table.open_file_table_lock);
            lock_release(curproc->file_descriptor_table->fd_table_lock);
            kfree(stat);
            *retval = -1;
            return EINVAL;
        }
        curproc->file_descriptor_table->file_entries[fd]->offset = stat->st_size + pos;
        *retval = curproc->file_descriptor_table->file_entries[fd]->offset;
        lock_release(open_file_table.open_file_table_lock);
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        kfree(stat);
        return 0;
    }
    else {
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        *retval = -1;
        return EINVAL;
    }
}