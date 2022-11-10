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
    get length of argument and store in arglen
    return 0 on success
    return error code on failure
*/
static
int 
get_arglen(const char *argument, int max_length, size_t *arglen) {
    int err;
    int i = -1;
    char next_char;
    do {
        i++;
        err = copyin((const_userptr_t) &argument[i], (void*) &next_char, (size_t) sizeof(char));
        if (err) return err;
    } while (next_char != 0 && i < max_length);
    if (next_char != 0) {
        return E2BIG;
    }
    *arglen = i;
    return 0;
}

static
int
copyin_args(int argc, char **args, char **args_copy, int *args_size, int *args_copy_strings_size) {
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
        
        err = copyinstr((const_userptr_t)&args[i], args_copy[i], curr_arg_len, &actual_len);
        if (err) {
            kfree(args_copy[i]);
            return err;
        }
        arg_size_left -= curr_arg_len;
        args_size[i] = curr_arg_len;
        *args_copy_strings_size += curr_arg_len;
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
Returns argv address and and stackptr position
return 0 on success
return corresponding error code on error
*/
static int
copyout_args(int argc, vaddr_t *stackptr, char **args_copy, int args_copy_strings_size, userptr_t *argv, int *args_size) {
    //calculate stackptr values based on arg_copy sizes
    //argc + 1 because argv needs null terminator
    userptr_t argv_base = (userptr_t)(*stackptr - args_copy_strings_size - ((argc+1) * sizeof(userptr_t)));
    userptr_t strings_base = (userptr_t) (*stackptr - args_copy_strings_size);
    //iterate through args_copy and arrange them on stack
    userptr_t curr_string_ptr = strings_base;
    userptr_t curr_argv_ptr = argv_base;
    size_t actual_len;
    int err;
    for (int i = 0; i < argc; i++) {
        //copy out argument string
        err = copyoutstr(args_copy[i], curr_string_ptr, args_size[i], &actual_len);
        if (err) return err;
        //copy out pointer for argument string
        err = copyout((void*) &curr_string_ptr, curr_argv_ptr, sizeof(char*));
        if (err) return err;
        //increment curr pointers
        curr_argv_ptr += sizeof(userptr_t);
        curr_string_ptr += args_size[i];
    }
    //set the stackptr and arv values
    *stackptr = (vaddr_t) argv_base;
    *argv = argv_base;
    return 0;
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
if ( args_size == NULL) {
    kfree(program_copy);
    return ENOMEM;
}

//populate args_copy and args_size and get total size of args_copy
int args_copy_strings_size;
err = copyin_args(argc, args, args_copy, args_size, &args_copy_strings_size);
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
err = copyout_args(argc, &stackptr, args_copy, args_copy_strings_size, &argv, args_size);
if (err) {
    proc_setas(old_as);
    as_activate();
    as_destroy(new_as);
    kfree(program_copy);
    free_copies(args_copy, args_size, argc);
    return err;
}
/*
7.
Clean up old address space 
*/
kfree(program_copy);
free_copies(args_copy, args_size, argc);
as_destroy(new_as);

/*
8.
Warp to user mode
*/
enter_new_process(argc, argv, NULL, stackptr, entrypoint);

//panic if we return back
panic("execv returned from enter_new_process");
return EINVAL;
}