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

    struct file_entry *file_entries[] = kmalloc(sizeof(*file_entries));
    return fd_table;
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
