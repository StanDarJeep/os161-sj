#include <pidtable.h>
#include <proc.h>
#include <synch.h>

/*
Initializes pid related fields for a process. Called in proc_create
*/
void proc_pid_init(struct proc *proc) {
    proc->pid_lock = lock_create("pid_lock");
    proc->pid_cv = cv_create("pid cv");
    proc->p_children = array_create();
}

void proc_pid_destroy(struct proc *proc) {
    lock_destroy(proc->pid_lock);
    cv_destroy(proc->pid_cv);
    array_destroy(proc->p_children);
}

void
pid_table_init(struct pid_table *pid_table) {
    //pid_table->procs = kmalloc(PID_MAX * sizeof(struct proc));
    pid_table->occupied = kmalloc(PID_MAX * sizeof(int));
    pid_table->status = kmalloc(PID_MAX * sizeof(int));
    pid_table->parent_has_exited = kmalloc(PID_MAX * sizeof(int));
    for (int i = 0; i < PID_MAX; i++) {
        pid_table->occupied[i] = 0;
        pid_table->status[i] = UNOCCUPIED;
        pid_table->parent_has_exited = 0;
    }
    pid_table->pid_table_lock = lock_create("pid_table lock");
}

/*
Adds process to pid table.
Return the 0 on success and -1 if pid_table is full
*/
int
pid_table_add(struct pid_table *pid_table, struct proc *proc) {
    lock_acquire(pid_table->pid_table_lock);
    int index = -1;
    // process ID must be greater than 0
    for (int i = 1; i < PID_MAX; i++) {
        if (pid_table->occupied[i] == 0) {
            index = i;
            //pid_table->procs[index] = proc;
            pid_table->occupied[i] = 1;
            pid_table->status[pid] = ALIVE;
            pid_table->parent_has_exited[pid] = 0;
            proc->pid = (pid_t)index;
            lock_release(pid_table->pid_table_lock);
            return 0;
        }
    }
    lock_release(pid_table->pid_table_lock);
    return index;
}


void 
pid_table_remove(struct pid_table *pid_table, pid_t pid) {
    pid_table->occupied[pid] = 0; // handle errors maybe later?
    pid_table->status[pid] = UNOCCUPIED;
    pid_table->parent_has_exited[pid] = 0;
}