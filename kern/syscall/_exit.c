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

    lock_acquire(pid_table.pid_table_lock);
    if (pid_table.parent_has_exited[curproc->pid] == 1) {
        // parent has exited and we can free of the status
        pid_table_remove(&pid_table, curproc->pid);
    } else {
        // parent has not yet exited so we must keep the status in memory
        pid_table.status[curproc->pid] = ZOMBIE;
    }
    
    struct proc *child;
    for (int i = 0; i < array_num(curproc->p_children); i++) {
        child = array_get(curproc->p_children, i);
        pid_table.parent_has_exited[child->pid] = 1; // mark that the parent has exited for each child process
        if (pid_table.status[child->pid] == ZOMBIE) {
            pid_table_remove(&pid_table, child->pid); // Scenario 3 cleanup
        }
    }
    lock_release(pid_table.pid_table_lock);
    cv_broadcast(curproc->pid_cv, curproc->pid_lock);
    // free everything
    proc_destroy(curproc);
    thread_exit();
}