#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <synch.h>
#include <kern/errno.h>

/*
dup2 clones the file handle oldfd onto the file handle newfd. 
If newfd names an already-open file, that file is closed.

The two handles refer to the same "open" of the file -- that is, 
they are references to the same object and share the same seek pointer. 
Note that this is different from opening the same file twice.

The new fd is stored in retVal
On success, return 0
On error, return error code 
*/  
int sys__dup2(int oldfd, int newfd, int *retval) {
    if (oldfd < 0 || newfd < 0 || newfd >= OPEN_MAX || oldfd >= OPEN_MAX) {
        return EBADF;
    }
    if (oldfd == newfd) {
        *retval = newfd;
        return 0;
    }
    lock_acquire(curproc->file_descriptor_table->fd_table_lock);
    if (curproc->file_descriptor_table->count[oldfd] == 0) {
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return EBADF;
    }
    
    //if newfd is being used, close it
    if (curproc->file_descriptor_table->count[newfd] == 1) {
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        sys__close(newfd);
        lock_acquire(curproc->file_descriptor_table->fd_table_lock);
    }

    int index = -1;
    for (int i = 0; i < OPEN_MAX; i++) {
        if (curproc->file_descriptor_table->count[i] == 0) {
            index = i;
        }
    }
    if (index == -1) {
        //fd table is full
        lock_release(curproc->file_descriptor_table->fd_table_lock);
        return EMFILE;
    }
    
    curproc->file_descriptor_table->file_entries[newfd] = curproc->file_descriptor_table->file_entries[oldfd];
    curproc->file_descriptor_table->file_entries[newfd]->ref_count += 1;
    curproc->file_descriptor_table->count[newfd] = 1;
    *retval = newfd;
    lock_release(curproc->file_descriptor_table->fd_table_lock);
    return 0;
}