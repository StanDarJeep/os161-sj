#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <synch.h>
#include <kern/errno.h>
#include <uio.h>
/*
write writes up to buflen bytes to the file specified by fd, at the location in the file specified 
by the current seek position of the file, taking the data from the space pointed to by buf. 
The file must be open for writing.

The count of bytes written is stored in retVal
On success, return 0
On error, return error code
*/
int
sys__write(int fd, void *buf, size_t buflen, int *retval) {
    //Check to see if file descriptor is valid and points to valid file entry
    lock_acquire(curproc->file_descriptor_table->fd_table_lock);
    if (fd < 0 || fd > OPEN_MAX) {
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return EBADF;
    }
    lock_acquire(open_file_table.open_file_table_lock);
    struct file_entry *file_entry = curproc->file_descriptor_table->file_entries[fd];
    if (file_entry == NULL) {
        lock_release(open_file_table.open_file_table_lock);
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return EBADF;
    }
    if (!((file_entry->status & WRITE) == WRITE) && !((file_entry->status & READ_WRITE) == READ_WRITE)) {
        lock_release(open_file_table.open_file_table_lock);
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return EBADF;
    }
    
    //setup uio
    struct iovec iovec;
    struct uio uio;

    iovec.iov_ubase = buf;
    iovec.iov_len = buflen;
    uio.uio_iov = &iovec;
    uio.uio_iovcnt = 1;
    uio.uio_offset = file_entry->offset;
    uio.uio_resid = buflen;
    uio.uio_rw = UIO_WRITE;
    uio.uio_segflg = UIO_USERSPACE;
    uio.uio_space = curproc->p_addrspace;
    int err = VOP_WRITE(file_entry->file, &uio);
    if (err) {
        lock_release(open_file_table.open_file_table_lock);
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return err;
    }

    //set the offset
    file_entry->offset += (off_t)(buflen - uio.uio_resid);
    lock_release(open_file_table.open_file_table_lock);
    lock_release(curproc->file_descriptor_table->fd_table_lock);
    *retval = (int)(buflen - uio.uio_resid);

    return 0;
}