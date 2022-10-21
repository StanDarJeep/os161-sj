#include <filetable.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>

/*
read reads up to buflen bytes from the file specified by fd, at the location in the file specified by the 
current seek position of the file, and stores them in the space pointed to by buf. The file must be open for reading.
*/
int
sys__read(int fd, void *buf, size_t buflen) {
    struct file_entry *file_entry = curproc->file_descriptor_table->file_entries[fd];
    KASSERT(file_entry->status == READ);
    
}