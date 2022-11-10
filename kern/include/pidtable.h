#ifndef _PIDTABLE_H_
#define _PIDTABLE_H_

#include <limits.h>
#include <types.h>
#include <synch.h>
#include <proc.h>

#define UNOCCUPIED -1
#define ALIVE 0
#define ZOMBIE 1

/*
Pid table structure 
*/
struct pid_table{
	int *occupied; //array to tell if pid is used or nots
	int *status;
	int *parent_has_exited;
	//struct proc **procs;
	struct lock *pid_table_lock; //lock for pidtable
};

extern struct pid_table pid_table;

void proc_pid_init(struct proc *proc);
void proc_pid_destroy(struct proc *proc);
void pid_table_init(struct pid_table *pid_table);
int pid_table_add(struct pid_table *pid_table, struct proc *proc);
void pid_table_remove(struct pid_table *pid_table, pid_t pid);

#endif /* _PIDTABLE_H_ */
