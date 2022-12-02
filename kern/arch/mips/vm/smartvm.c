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


struct coremap_entry *coremap;                                   // coremap representation
paddr_t coremap_paddr;                                           // physical address of coremap base
unsigned long num_pages;                                         // total number of physical memory pages
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;     // stealmem lock to be used before vm initialization 
static struct spinlock coremap_spinlock = SPINLOCK_INITIALIZER;  // coremap lock to be used after vm initialization
int vm_initialized = 0;                                          // boolean to keep track of vm initialization

/**
 * Helper function for vm_bootstrap
 * Initializes coremap data structure to keep track of physcial pages
 **/
static void initialize_coremap() {

    // Get ramsize and calculate number of physical pages
    paddr_t last = ram_getsize();
    paddr_t first = ram_getfirstfree();
    
    num_pages = ((last - first) / PAGE_SIZE) + 1;
    unsigned long coremap_size = (num_pages * sizeof(struct coremap_entry)) / PAGE_SIZE + 1;

    // Initialize coremap
    coremap_paddr = first + PAGE_SIZE - (first % PAGE_SIZE);
    coremap = (struct coremap_entry *)PADDR_TO_KVADDR(coremap_paddr);

    // Initialize fixed coremap entries used by kernel
    for (unsigned long i = 0; i < coremap_size; i++) {
        coremap[i].vaddr = PADDR_TO_KVADDR(i*PAGE_SIZE);
        coremap[i].status = PAGE_STATUS_FIXED;
    }

    // Initialize free coremap entries used by user
    for (unsigned long i = coremap_size; i < num_pages; i++) {
        coremap[i].vaddr = 0;
        coremap[i].status = PAGE_STATUS_FREE;
        coremap[i].size = 0;
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

/**
 * Helper function for alloc_kpages.
 * Finds continuous npages in the coremap
 * Returns physical address of first page
 **/
static paddr_t page_nalloc(unsigned long npages) {
    KASSERT(npages > 0);
    unsigned long pages_left = npages;
    unsigned long first_index = 0;
    unsigned long i = 0;

    // Find the first index of npages continuous free pages
    while (i < num_pages && pages_left > 0) {
        if (coremap[i].status == PAGE_STATUS_FREE) pages_left--;
        else {
            pages_left = npages;
            first_index = i + 1;
        }
        i++;
    }
    if (pages_left != 0) {
        panic("page_nalloc - not enough continuous pages\n");
    }

    // Allocate npages to user starting from the first index
    for (i = first_index; i < npages + first_index; i++) {
        if (i == first_index) {
            coremap[i].size = npages;
        }
        else coremap[i].size = 0;
        coremap[i].status = PAGE_STATUS_DIRTY;
        coremap[i].vaddr = PADDR_TO_KVADDR(i * PAGE_SIZE + coremap_paddr);
    }

    return first_index * PAGE_SIZE + coremap_paddr;
}

/* Allocate kernel heap pages (called by kmalloc) */
vaddr_t alloc_kpages(unsigned npages) {
    paddr_t paddr;
    if (vm_initialized) {
        spinlock_acquire(&coremap_spinlock);
        paddr = page_nalloc((unsigned long) npages);
        spinlock_release(&coremap_spinlock);
    } else {
        spinlock_acquire(&stealmem_lock);
        paddr = ram_stealmem(npages);
        spinlock_release(&stealmem_lock);
    }
    if (paddr == 0)
    {
        panic("alloc_kpages - paddr is 0");
    }
    return PADDR_TO_KVADDR(paddr);
}

/* Free kernel heap pages (called by kfree) */
void free_kpages(vaddr_t addr) {
	
    // Search through coremap and find matching virtual address
    spinlock_acquire(&coremap_spinlock);
    for (unsigned long i = 0; i < num_pages; i++) {
        if (coremap[i].vaddr == addr) {

            // Free pages based on the virtual address' coremap size
            for (unsigned long j = i; j < coremap[i].size + i; j++) {
                coremap[i].vaddr = 0;
                coremap[i].status = PAGE_STATUS_FREE;
                coremap[i].size = 0;
            }
        }
    }
    spinlock_release(&coremap_spinlock);
}

/* TLB shootdown handling called from interprocessor_interrupt */
void vm_tlbshootdown_all() {
}
void vm_tlbshootdown(const struct tlbshootdown *tlbshootdown) {
    (void)tlbshootdown;
}