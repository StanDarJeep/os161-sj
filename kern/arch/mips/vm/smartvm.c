#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

/* Initialization function */
void vm_bootstrap() {

}

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress) {
    (void)faultaddress;
    return faulttype;
}

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(unsigned npages) {
    (void)npages;
    return 0;
}
void free_kpages(vaddr_t addr) {
    (void)addr;
}

/* TLB shootdown handling called from interprocessor_interrupt */
void vm_tlbshootdown_all() {

}
void vm_tlbshootdown(const struct tlbshootdown *tlbshootdown) {
    (void)tlbshootdown;
}