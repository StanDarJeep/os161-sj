#include <filetable.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <synch.h>
#include <machine/trapframe.h>
#include <pidtable.h>
#include <copyinout.h>

/*
Wait for the process specified by pid to exit, and return an encoded exit status in the integer pointed to by status. If that process 
has exited already, waitpid returns immediately. If that process does not exist, waitpid fails.
waitpid returns the process id whose exit status is reported in status.
On error, -1 is returned, and errno is set to a suitable error code for the error condition encountered.
*/
int 
sys__waitpid(pid_t pid, int *status, int options, int *retval) {

    // check options is 0
    if (options != 0) {
        *retval = -1;
        return EINVAL;
    }

    lock_acquire(pid_table.pid_table_lock);

    // check for invalid pid's
    if (pid > PID_MAX || pid < PID_MIN || pid_table.occupied[pid] == 0) {
        lock_release(pid_table.pid_table_lock);
        *retval = -1;
        return ESRCH;
    }

    // check that the pid corresponds to a valid child process
    int err = -1;
    struct proc *child;
    for (int i = 0; i < (int)array_num(curproc->p_children); i++) {
        child = array_get(curproc->p_children, i);
        if (child->pid == pid) {
            err = 0;
            break;
        }
    }
    if (err != 0) {
        lock_release(pid_table.pid_table_lock);
        *retval = -1;
        return ECHILD;
    }

    if (pid_table.status[child->pid] == ZOMBIE) {
        // handle Scenario 2 where the child has already exited
        if (status != NULL) {
            err = copyout(&pid_table.exit_codes[child->pid], (userptr_t)status, sizeof(int));
            if (err) {
                lock_release(pid_table.pid_table_lock);
                *retval = -1;
                return EFAULT;
            }
        }
        lock_release(pid_table.pid_table_lock);
        *retval = child->pid;
        return 0;
    } else {
        // handle Scenario 1 where the parent waits for the child
        lock_release(pid_table.pid_table_lock);
        lock_acquire(child->pid_lock);
        cv_wait(child->pid_cv, child->pid_lock);
        lock_release(child->pid_lock);
        lock_acquire(pid_table.pid_table_lock);
        if (status != NULL) {
            err = copyout(&pid_table.exit_codes[child->pid], (userptr_t)status, sizeof(int));
            if (err) {
                lock_release(pid_table.pid_table_lock);
                *retval = -1;
                return EFAULT;
            }
        }
        lock_release(pid_table.pid_table_lock);
        *retval = child->pid;
        return 0;
    }
}