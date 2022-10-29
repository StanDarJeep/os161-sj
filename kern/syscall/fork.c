#include <filetable.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <addrspace.h>
#include <synch.h>
#include <machine/trapframe.h>

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
    *retval = 420;
    int err;
    struct proc *newproc = proc_create_runprogram("new proc");

    /*
    Copy address space
    */
    spinlock_acquire(&curproc->p_lock);
	struct addrspace *current_addrspace = curproc->p_addrspace;
	spinlock_release(&curproc->p_lock);
    err = as_copy(current_addrspace, &newproc->p_addrspace);
    if (err != 0) {
        proc_destroy(newproc);
        as_destroy(current_addrspace);
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
        return ENOMEM;
    }
    memcpy((void *)newtf, (const void *)tf, sizeof(struct trapframe));
    newtf->tf_v0 = 0;
	newtf->tf_a3 = 0;
	newtf->tf_epc += 4;

    /*
    Kernel thread that returns to usermode
    */
   
    err = thread_fork("new_thread", newproc, &enter_forked_process, (void *)newtf, 0);
    if (err != 0) {
        proc_destroy(newproc);
        as_destroy(current_addrspace);
        kfree(newtf);
    }
    return err;
}