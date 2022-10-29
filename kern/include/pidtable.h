#ifndef _PIDTABLE_H_
#define _PIDTABLE_H_

#include <limits.h>
#include <types.h>

/*
Pid table structure 
*/
struct pid_table{
	int *occupied; //array to tell if pid is used or nots
	struct proc **procs; //array to store processes
	struct lock *pid_table_lock; //lock for pidtable
};

extern struct pid_table pid_table;

void proc_pid_init(struct proc *proc);
void proc_pid_destroy(struct proc *proc);
void pid_table_init(struct pid_table *pid_table);
int pid_table_add(struct pid_table *pid_table, struct proc *proc);
void pid_table_remove(struct pid_table *pid_table);

#endif /* _PIDTABLE_H_ */
