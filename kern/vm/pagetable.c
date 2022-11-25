#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>
#include <pagetable.h>

/**
 * 
 **/
void copy_pte(struct page_table_entry *newpte, struct page_table_entry *oldpte, struct addrspace *newas);
{

}

/**
 * 
 **/
void free_pte(struct page_table_entry *pte, struct addrspace *as)
{

}