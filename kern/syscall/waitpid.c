#include <filetable.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <synch.h>
#include <machine/trapframe.h>
#include <pidtable.h>

int 
sys__waitpid(pid_t pid, int *status, int options, int *retval) {
    if (options != 0) {
        *retval = -1;
        return EINVAL;
    }

    lock_acquire(pid_table.pid_table_lock);
    if (pid > PID_MAX || pid < PID_MIN || pidtable.occupied[pid] == 0) {
        lock_release(pid_table.pid_table_lock);
        *retval = -1;
        return ESRCH;
    }
    int err = -1;
    struct proc *child;
    for (int i = 0; i < array_num(curproc->p_children); i++) {
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
        int *buf = pid_table.status[child->pid];
        err = copyout(buf, (userptr_t)status, sizeof(int));
        if (err) {
            lock_release(pid_table.pid_table_lock);
            kfree(buf);
            return EFAULT;
        }
        lock_release(pid_table.pid_table_lock);
        *retval = child->pid;
        return 0;
    } else {
        // Scenario 1 where the parent waits for the child
        int *buf = pid_table.status[child->pid];
        err = copyout(buf, (userptr_t)status, sizeof(int));
        if (err) {
            lock_release(pid_table.pid_table_lock);
            kfree(buf);
            return EFAULT;
        }
        cv_wait(child->pid_cv, child->pid_lock);
        lock_release(pid_table.pid_table_lock);
        *retval = child->pid;
        return 0;
    }
}