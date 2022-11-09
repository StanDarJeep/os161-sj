#include <filetable.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <synch.h>
#include <machine/trapframe.h>
#include <pidtable.h>

pid_t 
sys__waitpid(pid_t pid, int *status, int *retval) {
    if (curproc->p_children[pid] != 1) {
        return ECHILD;
    }
    cv_wait(pid_table.pid_cv[pid], pid_table.pid_table_lock);
}