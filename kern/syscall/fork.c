#include <filetable.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <synch.h>
#include <machine/trapframe.h>
#include <pidtable.h>

/*
fork duplicates the currently running process. 
The two copies are identical, except that one (the "new" one, or "child"), 
has a new, unique process id, and in the other (the "parent") the process id is unchanged.

On success, fork returns twice, once in the parent process and once in the child process.
In the child process, 0 is returned. In the parent process,
the process id of the new child process is returned.

On error, no new process is created. fork, only returns once, returning -1, 
and errno is set according to the error encountered
*/
int sys__fork(struct trapframe *tf, int *retval) {
    int err;
    struct proc *newproc = proc_create_runprogram("new proc");

    /*
    Copy address space
    */
	struct addrspace *current_addrspace = proc_getas();
    err = as_copy(current_addrspace, &newproc->p_addrspace);
    if (err != 0) {
        proc_destroy(newproc);
        as_destroy(current_addrspace);
        *retval = -1;
        return err;
    }
    /*
    Copy file table
    */
    lock_acquire(open_file_table.open_file_table_lock);
    newproc->file_descriptor_table = fd_table_create();
    for (int i = 0; i < OPEN_MAX; i++) {
        newproc->file_descriptor_table->count[i] = curproc->file_descriptor_table->count[i];
        if (newproc->file_descriptor_table->count[i] == 1) {
            newproc->file_descriptor_table->file_entries[i] = curproc->file_descriptor_table->file_entries[i];
            newproc->file_descriptor_table->file_entries[i]->ref_count++;
        }
    }
    lock_release(open_file_table.open_file_table_lock);
    /*
    Copy architectural state
    */
    struct trapframe *newtf = kmalloc(sizeof(struct trapframe));
    if (newtf == NULL)
    {
        // destroy stuff
        proc_destroy(newproc);
        as_destroy(current_addrspace);
        kfree(newtf);
        *retval = -1;
        return ENOMEM;
    }
    memmove((void *)newtf, (const void *)tf, sizeof(struct trapframe));
    newtf->tf_v0 = 0;
	newtf->tf_a3 = 0;
	newtf->tf_epc += 4;

    /*
    Get PID
    */
    pid_table_add(&pid_table, newproc); // call pid_table_remove if thread_fork fails

    /*
    Kernel thread that returns to usermode
    */
    err = thread_fork("new_thread", newproc, &enter_forked_process, (void *)newtf, 0);
    if (err != 0) {
        proc_destroy(newproc);
        as_destroy(current_addrspace);
        kfree(newtf);
        *retval = -1;
        return err;
    }
    curproc->p_children[newproc->pid] = 1;
    // potentially dynamically create cv here for waitpid, if memory issues
    *retval = newproc->pid;
    return err;
}