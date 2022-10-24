#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <synch.h>
#include <kern/errno.h>
#include <uio.h>
/*
read reads up to buflen bytes from the file specified by fd, at the location in the file specified by the 
current seek position of the file, and stores them in the space pointed to by buf. The file must be open for reading.

The count of bytes read is stored in retVal
On success, return 0
On error, return error code
*/
int
sys__read(int fd, void *buf, size_t buflen, int *retval) {
    lock_acquire(curproc->file_descriptor_table->fd_table_lock);
    if (fd < 0 || fd > OPEN_MAX) {
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return EBADF;
    }
    struct file_entry *file_entry = curproc->file_descriptor_table->file_entries[fd];
    if (file_entry == NULL) {
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return EBADF;
    }
    lock_acquire(file_entry->file_entry_lock);
    if (file_entry->status & WRITE) {
        lock_release(file_entry->file_entry_lock);
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return EBADF;
    }
    
    struct iovec iovec;
    struct uio uio;

    iovec.iov_ubase = buf;
    iovec.iov_len = buflen;
    uio.uio_iov = &iovec;
    uio.uio_iovcnt = 1;
    uio.uio_offset = file_entry->offset;
    uio.uio_resid = buflen;
    uio.uio_rw = UIO_READ;
    uio.uio_segflg = UIO_USERSPACE;
    uio.uio_space = curproc->p_addrspace;

    int err = VOP_READ(file_entry->file, &uio);
    if (err) {
        lock_release(file_entry->file_entry_lock);
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return err;
    }

    file_entry->offset += (off_t)(buflen - uio.uio_resid);
    lock_release(file_entry->file_entry_lock);
    lock_release(curproc->file_descriptor_table->fd_table_lock);
    *retval = (int)(buflen - uio.uio_resid);
    return 0;
}