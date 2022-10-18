#include <limits.h>
#include <types.h>
#include <vnode.h>
#include <array.h>

struct fd_table {
    struct file_entry* file_entries[OPEN_MAX]; //fd is index
};

struct open_file_table {
    struct array *entries;
};

extern struct open_file_table open_file_table;

struct file_entry {
    enum file_status status;
    off_t offset;
    struct vnode* file;
};

enum file_status {
    READ,
    WRITE,
    READ_WRITE,
};

struct fd_table *fd_table_create();
void open_file_table_init(struct open_file_table *ft);
int open_file_table_add(struct open_file_table *oft, struct file_entry *file_entry);
