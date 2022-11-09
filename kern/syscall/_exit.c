#include <filetable.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <synch.h>
#include <machine/trapframe.h>
#include <pidtable.h>

void 
sys__exit(int exitcode) {
    pid_table_remove(&pid_table, curproc->pid);
    cv_broadcast(curproc->pid_cv, curproc->pid_lock);
    // free everything
    thread_exit();
}