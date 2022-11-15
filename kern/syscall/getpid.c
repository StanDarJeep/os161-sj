#include <types.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <synch.h>

/*
getpid returns the process id of the current process.
getpid does not fail.
*/
int sys__getpid(int *retval) {
    lock_acquire(curproc->pid_lock);
    *retval = curproc->pid;
    lock_release(curproc->pid_lock);
    return 0;
}