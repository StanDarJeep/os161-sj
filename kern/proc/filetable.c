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
    fd_table->count = 0;
    return fd_table;
}

int 
fd_table_add(struct fd_table *fd_table, struct file_entry *file_entry)
{
    fd_table->file_entries[fd_table->count] = file_entry;
    return fd_table->count++;
}

void open_file_table_init(struct open_file_table *ft) {
    ft->entries = kmalloc(sizeof(ft->entries));
}

