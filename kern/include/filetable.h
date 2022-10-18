#include <limits.h>
#include <types.h>
#include <vnode.h>


struct fd_table {
    struct file_entry* file_entries[OPEN_MAX]; //fd is index
};

struct open_file_table {
    struct file_entry* entries;
};

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

