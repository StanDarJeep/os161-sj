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

struct spinlock *coremap_spinlock;
struct coremap_entry *coremap;
unsigned int num_coremap_entries;
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
struct coremap_entry *coremap;
int vm_initialized = 0;

static void initialize_coremap() {
    coremap_spinlock = kmalloc(sizeof(struct spinlock));
    spinlock_init(coremap_spinlock);
    //get ramsize and calculate number of physical pages
    paddr_t last = ram_getsize();
    num_coremap_entries = last / PAGE_SIZE;
    size_t num_pages = (num_coremap_entries * sizeof(struct coremap_entry) + PAGE_SIZE - 1) / PAGE_SIZE;

    //acquire physical memory for coremap
    spinlock_acquire(&stealmem_lock);
    paddr_t paddr = ram_stealmem(num_pages);
    spinlock_release(&stealmem_lock);

    //initialize coremap
    coremap = (struct coremap_entry *)PADDR_TO_KVADDR(paddr);
    paddr_t first = ram_getfirstfree();
    unsigned long first_page_index = first >> 12;

    //initialize coremap entries used by kernel
    for (unsigned int i = 0; i < first_page_index; i++) {
        coremap[i].vaddr = PADDR_TO_KVADDR(i*PAGE_SIZE);
        coremap[i].free = 0;
        coremap[i].is_kernel = 1;
    }

    //initialize coremap entries used by user
    for (unsigned int i = first_page_index; i < num_coremap_entries; i++) {
        coremap[i].vaddr = 0;
        coremap[i].free = 1;
        coremap[i].is_kernel = 0;
    }
}

/* Initialization function */
void vm_bootstrap() {
    initialize_coremap();
    vm_initialized = 1;
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