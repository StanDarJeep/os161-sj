#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <synch.h>
#include <kern/errno.h>
#include <uio.h>
#include <copyinout.h>
/*
The name of the current directory is computed and stored in buf, 
an area of size buflen. The length of data actually stored, 
which must be non-negative, is returned in retval.

return 0 on success
return the error code on error
*/
int sys__getcwd(char *buf, size_t buflen, int *retval) {
    if (!buf) return EFAULT;

    char *temp = kmalloc(sizeof(char *));
    struct uio uio;
    struct iovec iovec;
    //init uio for cwd
    uio_kinit(&iovec, &uio, temp, buflen, 0, UIO_READ);
    //get cwd
    int err = vfs_getcwd(&uio); 
    if (err) return err;

    err = copyout((const void *)temp, (userptr_t)buf, (size_t)(sizeof(char *)));
    kfree(temp);

    if (err) return err;

    *retval = buflen - uio.uio_resid;
    return 0;
}
