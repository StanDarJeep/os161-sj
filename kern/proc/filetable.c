#include <filetable.h>
#include <array.h>

struct fd_table *
fd_table_create()
{
    struct fd_table *fd_table = kmalloc(sizeof(*fd_table));
    if (fd_table == NULL) {
        kfree(fd_table);
        return NULL;
    }

    fd_table->file_entries = kmalloc(OPEN_MAX * sizeof(struct file_entry));
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
    ft->entries = array_create();
    array_init(ft->entries);
}

int open_file_table_add(struct open_file_table *oft, struct file_entry *file_entry) {

    KASSERT(oft != NULL);
    KASSERT(file_entry != NULL);
    int err = array_add(oft->entries, file_entry, NULL);
    return err;
}
