#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include <limits.h>
#include <types.h>
#include <vnode.h>
#include <array.h>

/*
This is the representation of our file descriptor table, which is process specific and initialized in proc_create_runprogram()
*/
struct fd_table {
    int *count;                       // An array of values that will keep track of whether or not each file descriptor is already in use
    struct file_entry **file_entries; // An array of pointers to each file entry
    struct lock *fd_table_lock;
};

/*
This is the representation of our open file table, which is initialized in the global context in main()
*/
struct open_file_table {
    struct array *entries;             // All file entries created in the system are stored here
    struct lock *open_file_table_lock;
};

extern struct open_file_table open_file_table; // declared so that all files which include filetable.h will see this global structure

/*
Enum for the status of the file entry
*/
enum file_status {
    READ = 0,
    WRITE = 1,
    READ_WRITE = 2,
};

/*
This is our representation of a file entry. Alongside the standard status, offset, and file pointer fields, we also declare ref_count in
order for us to keep track of when a file_entry should be freed (i.e. when no file descriptors exist that point to it). This is necessary
due to the dup2 system call.
*/
struct file_entry {
    enum file_status status;
    off_t offset;
    struct vnode *file;
    int ref_count;
};

// FD table functions
struct fd_table *fd_table_create(void);
void fd_table_destroy(struct fd_table *fd_table);
int fd_table_add(struct fd_table *fd_table, struct file_entry *file_entry);
int fd_table_remove(struct fd_table *fd_table, int fd);

// Open file table functions
void open_file_table_init(struct open_file_table *ft);
int open_file_table_add(struct open_file_table *oft, struct file_entry *file_entry);
int open_file_table_remove(struct open_file_table *oft, struct file_entry *file_entry);
int open_file_table_getIndexOf(struct open_file_table *oft, struct file_entry *file_entry);

// File entry functions
struct file_entry *file_entry_create(enum file_status file_status, off_t offset, struct vnode *vnode);
int file_entry_destroy(struct file_entry *file_entry);

#endif /* _FILETABLE_H_*/
