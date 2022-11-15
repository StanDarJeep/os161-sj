#include <filetable.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <synch.h>
#include <machine/trapframe.h>
#include <pidtable.h>
#include <thread.h>

/*
Cause the current process to exit. The exit code exitcode is reported back to other process(es) via the waitpid() call.
_exit does not return.
*/

void 
sys__exit(int exitcode) {

    lock_acquire(pid_table.pid_table_lock);
    if (pid_table.parent_has_exited[curproc->pid] == 1) {
        // parent has exited and we can free of the status: Scenario 3
        pid_table_remove(&pid_table, curproc->pid);

        cleanup_children();
        lock_release(pid_table.pid_table_lock);
        
        thread_leave();
    } else {
        // parent has not yet exited so we must keep the status and exit code in memory (zombie): Scenario 1 and 2
        pid_table.status[curproc->pid] = ZOMBIE;
        pid_table.exit_codes[curproc->pid] = exitcode;
        pid_table.parent_has_exited[curproc->pid] = -1; // mark that the process is waiting for parent to exit before freeing

        cleanup_children();
        lock_release(pid_table.pid_table_lock);

        lock_acquire(curproc->pid_lock);
        cv_broadcast(curproc->pid_cv, curproc->pid_lock);
        lock_release(curproc->pid_lock);

        thread_exit();
    }
}