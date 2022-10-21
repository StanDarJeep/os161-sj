int
close(int fd)
{
    int result = vfs_close(curproc->file_descriptor_table[fd]->file);
    if (result == 0) curproc->file_descriptor_table->count--;
    return result;
}