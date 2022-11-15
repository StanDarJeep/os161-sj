#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <addrspace.h>
#include <limits.h>
#include <copyinout.h>
#include <synch.h>

/*
Get length of argument and store in arglen
Input:
* argument - argument to get length of
* max_lengt - max_length of the argument
Output:
*arglen - length of the argument
Returns:
* 0 on success
* error code on failure
*/
static
int 
get_arglen(const char *argument, int max_length, size_t *arglen) {
    int err;
    int i = -1;
    char next_char;
    do {
        i++;
        err = copyin((const_userptr_t)&argument[i], (void*) &next_char, (size_t) sizeof(char));
        if (err) return err;
    } while (next_char != 0 && i < max_length);
    if (next_char != 0) {
        return E2BIG;
    }
    *arglen = i+1;
    return 0;
}

/*
Copy in the arguments from args into kernel. 
Input:
* argc - number of arguments
* args - arguments given to exec call
Output:
* args_copy - copied in version of args
* args_size - array holding the size of each argument
Returns:
* 0 on success
* error code on error
*/
static
int
copyin_args(int argc, char **args, char **args_copy, int *args_size) {
    int arg_size_left = ARG_MAX;
    int err;
    size_t curr_arg_len;
    size_t actual_len;
    
    for (int i = 0; i < argc; i ++) {
        err = get_arglen((const char *)args[i], arg_size_left - 1, &curr_arg_len);
        if (err) {
            for (int j = 0; j < i; j++) {
                kfree(args_copy[j]);
            }
            return err;
        }
        args_copy[i] = kmalloc(curr_arg_len * sizeof(char));
        err = copyinstr((const_userptr_t)args[i], args_copy[i], curr_arg_len, &actual_len);
        if (err) {
            for (int j = 0; j < i; j++) {
                kfree(args_copy[j]);
            }
            return err;
        }
        arg_size_left -= curr_arg_len;
        args_size[i] = curr_arg_len;
    }
    return 0;
}

/*
free args_copy and args_size
*/
static void
free_copies(char **args_copy, int *args_size, int argc) {
    for (int i = 0; i < argc; i++) {
        kfree(args_copy[i]);
    }
    kfree(args_copy);
    kfree(args_size);
}

/*
copy out arguments (strings and pointers) from kernel to new address space and arrange them on stack.
Input:
* argc - number of arguments
* args_copy - copied in arguments from exec call
* args_size - array holding size of each argument
Output: 
* stackptr - stack pointer
* argv - user pointer pointing to copied out arguments
Returns argv address and and stackptr position
*/
static void
copyout_args(int argc, char **args_copy, int *args_size, vaddr_t *stackptr, userptr_t *argv) {
    //ptr to track arg pointers
    userptr_t *user_arg_ptr = (userptr_t *) (*stackptr - (argc * sizeof(userptr_t *)) - sizeof(NULL));
    //ptr to track arg strings
    userptr_t user_arg_str = (userptr_t) (*stackptr - (argc * sizeof(userptr_t *)) - sizeof(NULL));

	for (int i = 0; i < argc; i++) {
        //decrement string pointer based on arg size
		user_arg_str -= args_size[i];
		*user_arg_ptr = user_arg_str;
        size_t *length = kmalloc(sizeof(int));
        copyoutstr((const char *) args_copy[i], user_arg_str, (size_t) args_size[i], length);
        kfree(length);
        //go to next argument
		user_arg_ptr++;
	}
    //null terminated args
	*user_arg_ptr = NULL;
	*argv = (userptr_t) (*stackptr - argc*sizeof(int) - sizeof(NULL));
    //align string pointer to 4
	user_arg_str -= (int) user_arg_str % sizeof(void *);
	*stackptr = (vaddr_t) user_arg_str;
}

/*
execv replaces the currently executing program with a newly loaded program image. 
This occurs within one process; the process id is unchanged.

The pathname of the program to run is passed as program. 
The args argument is an array of 0-terminated strings. 
The array itself should be terminated by a NULL pointer.

The argument strings should be copied into the new process as the 
new process's argv[] array. In the new process, argv[argc] must be NULL.

On success, execv does not return; instead, the new program begins executing
On failure, execv returns corresponding error code
*/
int 
sys__execv(const char *program, char **args) {
if (program == NULL || args == NULL) {
    return EFAULT;
}
int err;

/*
1.
Copy the arguments from old address space
*/

//Copy process pathname into kernel
char *program_copy = kmalloc(PATH_MAX * sizeof(char));
size_t *path_len = kmalloc(sizeof(size_t));
err = copyinstr((const_userptr_t)program, program_copy, PATH_MAX, path_len);
if (err) {
    kfree(program_copy);
    kfree(path_len);
    return err;
}
kfree(path_len);

//Get number of arguments (argc) in args
int argc = 0;
char *next_arg;
do {
    argc++;
    err = copyin((const_userptr_t)&args[argc], (void *) &next_arg, (size_t)sizeof(char*));
    if (err) {
        kfree(program_copy);
        return (err);
    }
} while (next_arg != NULL && ((argc+1) * 4) < ARG_MAX);
if (next_arg != NULL) {
    kfree(program_copy);
    return E2BIG;
}

//allocate array for arg copy and another array for length of each arg
//make sure to check enough virutal mem is available
char **args_copy = kmalloc(argc * sizeof(char*));
if (args_copy == NULL) {
    kfree(program_copy);
    return ENOMEM;
}
int *args_size = kmalloc(argc * sizeof(int));
if (args_size == NULL) {
    kfree(args_copy);
    kfree(program_copy);
    return ENOMEM;
}

//populate args_copy and args_size
err = copyin_args(argc, args, args_copy, args_size);
if (err) {
    kfree(program_copy);
    kfree(args_copy);
    kfree(args_size);
    return (err);
}

struct vnode *v;
err = vfs_open(program_copy, O_RDONLY, 0, &v);
if (err) {
    kfree(program_copy);
    free_copies(args_copy, args_size, argc);
    return err;
}
/*
2.
Get new address space
*/
struct addrspace *old_as = proc_getas();
struct addrspace *new_as = as_create();
if (new_as == NULL) {
    kfree(program_copy);
    free_copies(args_copy, args_size, argc);
    return ENOMEM;
}

/*
3.
Switch to new address space
*/
proc_setas(new_as);
as_activate();

/*
4.
Load new executable
*/
vaddr_t entrypoint;
err = load_elf(v, &entrypoint);
if (err) {
    proc_setas(old_as);
    as_activate();
    as_destroy(new_as);
    vfs_close(v);
    kfree(program_copy);
    free_copies(args_copy, args_size, argc);
    return err;
}
vfs_close(v);

/*
5.
Define new stack region
*/
vaddr_t stackptr;
err = as_define_stack(new_as, &stackptr);
if (err) {
    proc_setas(old_as);
    as_activate();
    as_destroy(new_as);
    kfree(program_copy);
    free_copies(args_copy, args_size, argc);
    return err;
}
/*
6.
Copy arguments to new address space, properly arranging them
*/
userptr_t argv;
copyout_args(argc, args_copy, args_size, &stackptr, &argv);
/*
7.
Clean up old address space 
*/
kfree(program_copy);
free_copies(args_copy, args_size, argc);
as_destroy(old_as);
/*
8.
Warp to user mode
*/
enter_new_process(argc, argv, NULL, stackptr, entrypoint);

//panic if we return back
panic("execv returned from enter_new_process");
return EINVAL;
}