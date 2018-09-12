// #include <types.h>
// #include <kern/errno.h>
// #include <lib.h>
// #include <thread.h>
// #include <addrspace.h>
// #include <vm.h>
// #include <machine/tlb.h>

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <spl.h>
#include <mips/tlb.h>
#include <proc.h>
#include <current.h>


struct page_entry {
	uint32_t	pe_proc;					/* the process id */
	uint32_t	pe_ppn;						/* the frame table frame num */
	uint32_t	pe_vpn;						/* the frame table frame num */
	int		pe_flags;					/* page permissions and flags */
	struct page_entry *pe_next;				/* pointer to collion next entry */
};

/* pointer to the hashed page table */
struct page_entry **hpt;

/* number of entries in the page table */
unsigned int hpt_size;	


/* define static methods */
static uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr);

static struct page_entry * insert_hpt(struct addrspace *as, vaddr_t vaddr, vaddr_t n_frame);
static void tlb_fill(struct page_entry *pe, vaddr_t faultaddress);
static void hpt_init(void);

static void hpt_init(void){

	paddr_t ram_sz;

	ram_sz = ROUND_UP(ram_getsize());

	int n_pages = ram_sz / PAGE_SIZE;

	hpt_size = n_pages * 2;

	hpt = (struct page_entry **)kmalloc(hpt_size * sizeof(struct page_entry *));

}



static uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr)
{
    uint32_t index;
    index = (((uint32_t )as) ^ (faultaddr >> PAGE_BITS)) % hpt_size;
    return index;
}


void vm_bootstrap(void)
{
    hpt_init();
    frametable_init();


    /* create the page table */
    kprintf("time to setup page table\n");
    kprintf("[*] vm_boostrap: hpt_size is %d\n", hpt_size);

    unsigned int i;
    /* init the page table */
    for(i = 0; i < hpt_size; i++) {
        hpt[i] = NULL;
    }
}

int
vm_fault(int faulttype, vaddr_t faultaddress){
    int spl, perms;
    int region;
    //uint32_t ppn;
    struct page_entry *pe;
    struct addrspace *as;
    //int dirty;
    as = proc_getas();
    /* sanity check */
    if (!curproc || !hpt || !as) {
        return EFAULT;
    }

    /* check if request was to a valid region */
    region = region_type(as, faultaddress);
    if (region == 0) {
        return EFAULT;
    }

    /* get page entry */
    spl = splhigh();
    //pe = search_hpt(as, faultaddress);
    uint32_t pt_hash, proc, vpn;
                
        /* get the hash index for hpt */
    pt_hash = hpt_hash(as, faultaddress);
    
    /* get addrspace id (secretly the pointer to the addrspace) */
    proc = (uint32_t) as;

    /* declare the vpn */
    //vpn = ADDR_TO_PN(addr);
    vpn = (faultaddress >> 12);

    /* get the page table entry */
    pe = hpt[pt_hash];

    /* loop the chain while we don't have an entry for this proc */
    while(pe != NULL) {
            if (pe->pe_proc == proc && pe->pe_vpn == vpn) {
                break;
            }
            pe = pe->pe_next;
    }
    splx(spl);


	perms = region_perms(as, faultaddress);
    
    switch (faulttype) {
            case VM_FAULT_READ:
            	
            case VM_FAULT_WRITE:
                //goto normal_handle;

	            if (pe) { 

	        	} else { /* not in frame table */

	            spl = splhigh();
	            vaddr_t n_frame = alloc_kpages(1);
	            pe = insert_hpt(as, faultaddress, n_frame);
	            splx(spl);

	            
	        	}
	        	tlb_fill(pe, faultaddress);
	        	return 0;
            case VM_FAULT_READONLY:
                
                if (!pe)
                    panic("this shouldn't happen");
               
                /* region isn't writable anyway so EFAULT */
                // if (!(GET_WRITABLE(perms)))
                //     return EFAULT;

                if(!((perms >> 1) & 1))
                		return EFAULT;

                /* region is writable but page isn't - COW! */
                // if (!(GET_WRITABLE(pe->pe_flags >> 1)))
                //     goto do_cow;
        
                if(!((pe->pe_flags >> 2) & 1)){
                		//return EFAULT;
                	//goto do_cow;
                	spl = splhigh();
			        if (pe) {
			        //     struct frame_entry fe = ft[pe->pe_ppn]; /* get the frame entry */

			        //     if (fe.fe_refcount > 1) {
			        //         fe.fe_refcount--;
			        //         vaddr_t new_frame = alloc_kpages(1);
			        //         memcpy((void *)new_frame, (void *)PADDR_TO_KVADDR(pe->pe_ppn << 12), PAGE_SIZE);
			        //         //pe->pe_ppn = KVADDR_TO_FINDEX(new_frame);

			        //         pe->pe_ppn = (KVADDR_TO_PADDR(new_frame) >> 12);

			        //     }
			            
			        //      // if refcount is 1, then the other process in the fork has
			        //      // * already done their deed and copied the frame so we can just
			        //      // * become writeable for this frame 
			        //     //pe->pe_flags = SET_PAGE_WRITE(pe->pe_flags);
			             pe->pe_flags = (pe->pe_flags | 0x4);

                	//return EFAULT;
			        } else {
			            panic("COW but we don't have a page entry??");
			        }
			        splx(spl);
			        //flush_tlb();
			        int i, spl;
			        spl = splhigh();
			        for (i=0; i<NUM_TLB; i++) {
			                tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
			        }
			        splx(spl);
			        //
			        tlb_fill(pe, faultaddress);
			        return 0;

                	
                }

                /* region is writable and page is writeable - fix TLB */
                //ppn = (uint32_t) PN_TO_ADDR(pe->pe_ppn) | TLBLO_DIRTY;
          //       ppn = (uint32_t)(pe->pe_ppn << 12) | TLBLO_DIRTY;

          //       //replace_tlb(faultaddress, ppn);

          //       int vaddr = faultaddress;
          //       vaddr &= PAGE_FRAME;
 
		        //  //get index where the vaddr is 
		        // int index = tlb_probe(vaddr, 0);
		        // if (index < 0) {
		        //         panic("lol");
		        // }

		        // /* replace the ppn with something else */
		        // tlb_write(vaddr, ppn, index);

          //       return 0;

            default:
                   return EINVAL;
    }

}


static void tlb_fill(struct page_entry *pe, vaddr_t faultaddress){
		uint32_t ppn;
	    ppn = (uint32_t)(pe->pe_ppn << 12);

        //int dirty = (GET_WRITABLE(pe->pe_flags >> 1)) ? TLBLO_DIRTY : 0;

        int dirty = ((pe->pe_flags >> 2) & 1) ? TLBLO_DIRTY : 0;

        ppn = ppn | TLBLO_VALID | dirty; /* set valid bit */ 
        //ddpe->pe_flags = SET_PAGE_REF(pe->pe_flags);  /* set referenced */
        //insert_tlb(faultaddress, ppn);       /* load tlb */

        int vaddr = faultaddress;
        int spl = splhigh();
        vaddr &= PAGE_FRAME;  /* mask the vpn */
        tlb_random(vaddr, ppn);
        splx(spl);


}



/*
 *
 * SMP-specific functions.  Unused in our configuration.
 */

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
        (void)ts;
        panic("vm tried to do tlb shootdown?!\n");
}



static struct page_entry * insert_hpt(struct addrspace *as, vaddr_t vaddr, vaddr_t n_frame)
{
        struct page_entry *n_pe = kmalloc(sizeof(struct page_entry));
        //uint32_t vpn = ADDR_TO_PN(vaddr);

        uint32_t vpn =  (vaddr >> 12);

        //uint32_t ppn = KVADDR_TO_FINDEX(n_frame);

        uint32_t ppn = (KVADDR_TO_PADDR(n_frame) >> 12);

        uint32_t pt_hash = hpt_hash(as, vaddr);
        struct page_entry *pe = hpt[pt_hash];

        int perms = region_perms(as, vaddr);

        /* set up new page */
        n_pe->pe_proc = (uint32_t) as;
        n_pe->pe_ppn = ppn;
        n_pe->pe_vpn = vpn;
        //n_pe->pe_flags = SET_PAGE_PROT(0, perms);
        //n_pe->pe_flags = SET_PAGE_PRES(n_pe->pe_flags);

        n_pe->pe_flags = (0 | ((perms & 0x7) << 1));
        n_pe->pe_flags = (n_pe->pe_flags | 0x1);


        n_pe->pe_next = NULL;

        /* check if this record is taken */
        if (pe != NULL) {
                /* loop until we find the last of the chain */
                while (pe->pe_next != NULL) {
                        /* get next page */
                        pe = pe->pe_next;
                }
                pe->pe_next = n_pe;
        } else {
                hpt[pt_hash] = n_pe;
        }
        return n_pe;
}



void purge_hpt(struct addrspace *as)
{
        unsigned int i;
        uint32_t proc = (uint32_t) as;
       // kprintf("purging hpt: %x\n", proc);
        struct page_entry *c_pe, *n_pe, *t_pe;;
        
        int spl = splhigh();
        for (i=0; i < hpt_size; i++) {
                n_pe = c_pe = hpt[i];
                while (c_pe != NULL) {
                        if (c_pe->pe_proc == proc) {
                                if (hpt[i] == c_pe)
                                        t_pe = hpt[i] = n_pe = c_pe->pe_next;
                                else
                                        t_pe = n_pe->pe_next = c_pe->pe_next;
                                free_kpages(PADDR_TO_KVADDR(c_pe->pe_ppn << 12));
                                kfree(c_pe);
                                c_pe = t_pe;
                        } else {
                                n_pe = c_pe;
                                c_pe = c_pe->pe_next;
                        }
                }
        }
        splx(spl);
}

void
duplicate_hpt(struct addrspace *new, struct addrspace *old)
{
        uint32_t o_proc = (uint32_t) old;
        struct page_entry *pe, *n_pe;
        unsigned int i;
        
        int spl = splhigh();
        for (i=0; i < hpt_size; i++) {
                pe = hpt[i];
                while (pe != NULL) {
                        /* if we find a matching record, duplicate & insert */
                        n_pe = pe;
                        if (pe->pe_proc == o_proc) {
                                /* insert the new entry with the old frame */

                        		vaddr_t new_frame = alloc_kpages(1);
                                vaddr_t old_frame = PADDR_TO_KVADDR(pe->pe_ppn << 12);
                                memcpy((void *)new_frame, (void *)old_frame, PAGE_SIZE);
                                //n_pe = insert_hpt(new, PN_TO_ADDR(pe->pe_vpn), old_frame); 
                                n_pe = insert_hpt(new, (pe->pe_vpn << 12), new_frame);
                                /* disable write bit */
                                //n_pe->pe_flags = SET_PAGE_NOWRITE(n_pe->pe_flags);
                                //pe->pe_flags = SET_PAGE_NOWRITE(pe->pe_flags);
                                
                                n_pe->pe_flags = (n_pe->pe_flags & ~0x4) & 0xFFFFFFFF;
                                pe->pe_flags = (pe->pe_flags & ~0x4) & 0xFFFFFFFF;

                                /* increment refcount on frame */
                                //ft[pe->pe_ppn].fe_refcount++;
                        }
                        pe = n_pe->pe_next;
                }
        }
    splx(spl);
}


