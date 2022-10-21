#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include <limits.h>
#include <types.h>
#include <vnode.h>
#include <array.h>

struct fd_table {
    int *count;
    struct file_entry **file_entries; // fd is index
    struct lock *fd_table_lock;
};

struct open_file_table {
    struct array *entries;
    struct lock *open_file_table_lock;
};

extern struct open_file_table open_file_table;

enum file_status {
    READ,
    WRITE,
    READ_WRITE,
};

struct file_entry {
    enum file_status status;
    off_t offset;
    struct vnode *file;
    int ref_count;
    struct lock *file_entry_lock;
};

struct fd_table *fd_table_create(void);
void fd_table_destroy(struct fd_table *fd_table);
int fd_table_add(struct fd_table *fd_table, struct file_entry *file_entry);
int fd_table_remove(struct fd_table *fd_table, int fd);
void open_file_table_init(struct open_file_table *ft);
int open_file_table_add(struct open_file_table *oft, struct file_entry *file_entry);
int open_file_table_remove(struct open_file_table *oft, struct file_entry *file_entry);

/*
    return index of file_entry in open_file_table. -1 if it doesn't exist
*/
int open_file_table_getIndexOf(struct open_file_table *oft, struct file_entry *file_entry);
struct file_entry *file_entry_create(enum file_status file_status, off_t offset, struct vnode *vnode);
int file_entry_destroy(struct file_entry *file_entry);

#endif /* _FILETABLE_H_*/
