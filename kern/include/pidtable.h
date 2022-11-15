#ifndef _PIDTABLE_H_
#define _PIDTABLE_H_

#include <limits.h>
#include <types.h>
#include <synch.h>
#include <proc.h>

#define UNOCCUPIED -1
#define RUNNING 0
#define ZOMBIE 1

/*
Pid table structure 
*/
struct pid_table{
	int *occupied;               // Used to determine whether a pid is used
	int *status;                 // Used to store a process' status
	int *exit_codes;             // Used to store the exit codes for a process when it calls exit
	int *parent_has_exited;      // Used to check if the parent of a process has exited
	struct lock *pid_table_lock; // lock for pidtable
};

/*
This is the representation of our pid table, which is initialized in the global context in main()
*/
extern struct pid_table pid_table;

// Process pid functions
void proc_pid_init(struct proc *proc);
void proc_pid_destroy(struct proc *proc);

// pid table functions
void pid_table_init(struct pid_table *pid_table);
int pid_table_add(struct pid_table *pid_table, struct proc *proc);
void pid_table_remove(struct pid_table *pid_table, pid_t pid);

#endif /* _PIDTABLE_H_ */