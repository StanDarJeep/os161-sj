

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
int sys__fork(void) {
    
}