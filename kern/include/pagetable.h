#include <vm.h>
#include "opt-dumbvm.h"


struct page_table_entry {
        vaddr_t va;
        paddr_t pa;
}

void copy_pte(struct page_table_entry *newpte, struct page_table_entry *oldpte, struct addrspace *newas);
void free_pte(struct page_table_entry *pte, struct addrspace *as);