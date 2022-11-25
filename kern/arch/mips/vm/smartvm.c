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
        coremap[i].status = PAGE_STATUS_FIXED;
        coremap[i].size = 1;
    }

    //initialize coremap entries used by user
    for (unsigned int i = first_page_index; i < num_coremap_entries; i++) {
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

static paddr_t page_nalloc(unsigned long npages) {
    KASSERT(npages > 0);
    unsigned long pages_left = npages;
    
    int first_index = 0;
    unsigned long i = 0;
    while (i < num_coremap_entries - 1 && pages_left > 0) {
        if (coremap[i].status == PAGE_STATUS_FREE) pages_left--;
        else {
            pages_left = npages;
            first_index = i + 1;
        }
    }
    
    if (pages_left != 0) {
        panic("page_nalloc - not enough continuous pages\n");
    }

    for (i = first_index; i < npages + first_index; i++) {
        if (i == 0) coremap[i].size = (size_t) npages;
        else coremap[i].size = 0;
        coremap[i].status = PAGE_STATUS_DIRTY;
        coremap[i].vaddr = PADDR_TO_KVADDR(i * PAGE_SIZE);
    }    
    return (paddr_t)(first_index * PAGE_SIZE);
}

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(unsigned npages) {
    paddr_t paddr;
    if (vm_initialized) {
        spinlock_acquire(coremap_spinlock);
        paddr = page_nalloc((unsigned long) npages);
        spinlock_release(coremap_spinlock);
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
void free_kpages(vaddr_t addr) {
    (void)addr;
}

/* TLB shootdown handling called from interprocessor_interrupt */
void vm_tlbshootdown_all() {
}
void vm_tlbshootdown(const struct tlbshootdown *tlbshootdown) {
    (void)tlbshootdown;
}