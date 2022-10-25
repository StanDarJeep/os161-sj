#include <filetable.h>
#include <array.h>
#include <synch.h>
#include <vfs.h>

/*
Initializer function for fd_table
*/
struct fd_table *
fd_table_create()
{
    struct fd_table *fd_table = kmalloc(sizeof(*fd_table));
    if (fd_table == NULL) {
        kfree(fd_table);
        return NULL;
    }

    fd_table->file_entries = kmalloc(OPEN_MAX * sizeof(struct file_entry)); // max size of the fd_table is denoted by OPEN_MAX
    fd_table->count = kmalloc(OPEN_MAX * sizeof(int));
    for (int i = 0; i < OPEN_MAX; i++) fd_table->count[i] = 0;              // count array should be zeroes upon initialization
    fd_table->fd_table_lock = lock_create("fd_table_lock");
    return fd_table;
}

/*
This function adds a file entry into the fd_table. Returns the index upon success. If the fd_table is full, returns -1.
*/
int 
fd_table_add(struct fd_table *fd_table, struct file_entry *file_entry)
{
    lock_acquire(fd_table->fd_table_lock);
    int index = -1;
    for (int i = 0; i < OPEN_MAX; i++) {
        if (fd_table->count[i] == 0) {
            index = i;
            break;
        }
    }

    // check if the fd_table was full
    if (index == -1){
        lock_release(fd_table->fd_table_lock);
        return -1;
    } 
    fd_table->file_entries[index] = file_entry;
    fd_table->count[index] = 1;
    lock_release(fd_table->fd_table_lock);
    return index;
}

/*
This function removes a file entry from the fd_table. Returns 0 upon success. If the fd was invalid, returns -1.
*/
int 
fd_table_remove(struct fd_table *fd_table, int fd) {
    lock_acquire(fd_table->fd_table_lock);
    if (fd >= OPEN_MAX || fd < 0 || fd_table->count[fd] != 1) {
        lock_release(fd_table->fd_table_lock);
        return -1;
    } 
    KASSERT(fd_table->file_entries[fd]->ref_count >= 1);
    fd_table->file_entries[fd]->ref_count -= 1;

    // if there are no longer any references to the file entry then we should destroy it
    if (fd_table->file_entries[fd]->ref_count == 0) {
        vfs_close(fd_table->file_entries[fd]->file);
        file_entry_destroy(fd_table->file_entries[fd]);
    }
    fd_table->count[fd] = 0;
    lock_release(fd_table->fd_table_lock);
    return 0;
}

/*
Initializer function for the open file table
*/
void 
open_file_table_init(struct open_file_table *ft) {
    ft->entries = array_create();
    array_init(ft->entries);
    ft->open_file_table_lock = lock_create("open_file_table_lock");
}

/*
This function adds a file entry into the open file table. Called everytime a file entry is created
*/
int 
open_file_table_add(struct open_file_table *oft, struct file_entry *file_entry) {

    KASSERT(oft != NULL);
    KASSERT(file_entry != NULL);
    lock_acquire(oft->open_file_table_lock);
    int err = array_add(oft->entries, file_entry, NULL);
    lock_release(oft->open_file_table_lock);
    return err;
}

/*
This function creates a file entry from the given fields. Calls open_file_table_add, as each file entry should exist in
the open file table
*/
struct file_entry * 
file_entry_create(enum file_status file_status, off_t offset, struct vnode *vnode) {
    struct file_entry *file_entry = kmalloc(sizeof(*file_entry));
    file_entry->status = file_status;
    file_entry->offset = offset;
    file_entry->file = vnode;
    file_entry->ref_count = 1;
    open_file_table_add(&open_file_table, file_entry);
    return file_entry;
}

/*
This function removes a file entry from the open file table. Calls open_file_table_getIndexOf in order to determine which file
entry must be removed from the structure
*/
int
open_file_table_remove(struct open_file_table *oft, struct file_entry *file_entry) {
    lock_acquire(oft->open_file_table_lock);
    int index = open_file_table_getIndexOf(oft, file_entry);
    if (index != -1) {
        array_remove(oft->entries, index);
        lock_release(oft->open_file_table_lock);
        return 0;
    } else {
        lock_release(oft->open_file_table_lock);
        return -1;
    }
}

/*
Helper function to return index of file_entry in open_file_table, by iterating over the entire structure and comparing each field.
Returns -1 if it doesn't exist.
THIS FUNCTION REQUIRES THE OPEN FILE TABLE LOCK BEFOREHAND
*/
int 
open_file_table_getIndexOf(struct open_file_table *oft, struct file_entry *file_entry){
    struct file_entry *f;
    for (int i = 0; (unsigned)i < array_num(oft->entries); i++) {
        f = (struct file_entry *) array_get(oft->entries, i);
        if (f->status == file_entry->status &&
            f->offset == file_entry->offset &&
            f->file == file_entry->file &&
            f->ref_count == file_entry->ref_count)
        {
            return i;
        }
    }
    return -1;
}

/*
This function destroys a file entry by calling open_file_table_remove and then freeing the memory
*/
int
file_entry_destroy(struct file_entry *file_entry) {
    int i = open_file_table_remove(&open_file_table, file_entry);
    if (i != 0) {
        return i;
    }
    kfree(file_entry);
    return 0;
}

/*
This function destroys an fd_table in the event that an error occurs during the initial process creation
*/
void 
fd_table_destroy(struct fd_table *fd_table) {
    for (int i = 0; i < OPEN_MAX; i++) {
        if (fd_table->count[i] >= 1) {
            if (fd_table->file_entries[i]->ref_count == 1) {
                file_entry_destroy(fd_table->file_entries[i]);
            } else {
                fd_table->file_entries[i]->ref_count -= 1;
            }
        }
    }
    kfree(fd_table->count);
    lock_destroy(fd_table->fd_table_lock);
    kfree(fd_table);
}
