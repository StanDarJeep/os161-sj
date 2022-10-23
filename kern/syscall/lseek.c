#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <kern/seek.h>
#include <limits.h>
#include <synch.h>

int 
sys__lseek(int fd, off_t pos, int whence, int *retval)
{
    lock_acquire(curproc->file_descriptor_table->fd_table_lock);
    if (curproc->file_descriptor_table->count[fd] != 1 || fd >= OPEN_MAX || fd < 0) {
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        *retval = -1;
        return EBADF;
    }
    lock_release(curproc->file_descriptor_table->fd_table_lock);
    if (fd < 3) {
        *retval = -1;
        return ESPIPE;
    }

    // consider big lock
    lock_acquire(curproc->file_descriptor_table->file_entries[fd]->file_entry_lock);
    if (whence == SEEK_SET) {
        if (pos < 0) {
            lock_release(curproc->file_descriptor_table->file_entries[fd]->file_entry_lock);
            *retval = -1;
            return EINVAL;
        }
        curproc->file_descriptor_table->file_entries[fd]->offset = pos;
        *retval = curproc->file_descriptor_table->file_entries[fd]->offset;
        lock_release(curproc->file_descriptor_table->file_entries[fd]->file_entry_lock);
        return 0;
    }
    else if (whence == SEEK_CUR) {
        if (curproc->file_descriptor_table->file_entries[fd]->offset + pos < 0) {
            lock_release(curproc->file_descriptor_table->file_entries[fd]->file_entry_lock);
            *retval = -1;
            return EINVAL;
        }
        curproc->file_descriptor_table->file_entries[fd]->offset += pos;
        *retval = curproc->file_descriptor_table->file_entries[fd]->offset;
        lock_release(curproc->file_descriptor_table->file_entries[fd]->file_entry_lock);
        return 0;
    }
    else if (whence == SEEK_END) {
        // add this
        return -1;
    }
    else {
        lock_release(curproc->file_descriptor_table->file_entries[fd]->file_entry_lock);
        *retval = -1;
        return EINVAL;
    }
}