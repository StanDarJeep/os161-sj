#include <filetable.h>

struct fd_table *
fd_table_create()
{
    struct fd_table *fd_table = kmalloc(sizeof(*fd_table));
    if (fd_table == NULL) {
        kfree(fd_table);
        return NULL;
    }

    struct file_entry *file_entries[] = kmalloc(sizeof(*file_entries));
    return fd_table;
}
