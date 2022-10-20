#ifndef _FILETABLE_H_
#define _FILETABLE_H_

#include <limits.h>
#include <types.h>
#include <vnode.h>
#include <array.h>

struct fd_table {
    int count;
    struct file_entry **file_entries; // fd is index
};

struct open_file_table {
    struct array *entries;
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
};

struct fd_table *fd_table_create(void);
int fd_table_add(struct fd_table *fd_table, struct file_entry *file_entry);
void open_file_table_init(struct open_file_table *ft);
int open_file_table_add(struct open_file_table *oft, struct file_entry *file_entry);

#endif /* _FILETABLE_H_*/
