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

/*
lseek alters the current seek position of the file handle filehandle, 
seeking to a new position based on pos and whence.
New position is stored in retval
On success, return 0
On error, return corresponding error code
*/
int 
sys__lseek(int fd, off_t pos, int whence, int64_t *retval)
{
    lock_acquire(curproc->file_descriptor_table->fd_table_lock);

    // Check if the fd is valid
    if (fd >= OPEN_MAX || fd < 0 || curproc->file_descriptor_table->count[fd] != 1 ) {
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        *retval = -1;
        return EBADF;
    }

    // Check that the fd corresponds to a seekable object
    if (fd < 3 || !VOP_ISSEEKABLE(curproc->file_descriptor_table->file_entries[fd]->file)) {
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        *retval = -1;
        return ESPIPE;
    }

    // Perform the offset calculation based on whence
    if (whence == SEEK_SET) {

        // Check if the resulting offset is negative
        if (pos < 0) {
            lock_release(curproc->file_descriptor_table->fd_table_lock);
            *retval = -1;
            return EINVAL;
        }

        // Modify the file entry to the new offset and return the offset in retval
        lock_acquire(open_file_table.open_file_table_lock);
        curproc->file_descriptor_table->file_entries[fd]->offset = pos;
        *retval = curproc->file_descriptor_table->file_entries[fd]->offset;
        lock_release(open_file_table.open_file_table_lock);
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return 0;
    }
    else if (whence == SEEK_CUR) {
        lock_acquire(open_file_table.open_file_table_lock);

        // Check if the resulting offset is negative
        if (curproc->file_descriptor_table->file_entries[fd]->offset + pos < 0) {
            lock_release(open_file_table.open_file_table_lock);
            lock_release(curproc->file_descriptor_table->fd_table_lock);
            *retval = -1;
            return EINVAL;
        }

        // Modify the file entry to the new offset and return the offset in retval
        curproc->file_descriptor_table->file_entries[fd]->offset += pos;
        *retval = curproc->file_descriptor_table->file_entries[fd]->offset;
        lock_release(open_file_table.open_file_table_lock);
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return 0;
    }
    else if (whence == SEEK_END) {

        // Create a stat struct in order find the EOF of the file
        struct stat *stat = kmalloc(sizeof(struct stat));;
        lock_acquire(open_file_table.open_file_table_lock);
        VOP_STAT(curproc->file_descriptor_table->file_entries[fd]->file, stat);

        // Check if the resulting offset is negative
        if (stat->st_size + pos < 0) {
            lock_release(open_file_table.open_file_table_lock);
            lock_release(curproc->file_descriptor_table->fd_table_lock);
            kfree(stat);
            *retval = -1;
            return EINVAL;
        }

        // Modify the file entry to the new offset and return the offset in retval
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