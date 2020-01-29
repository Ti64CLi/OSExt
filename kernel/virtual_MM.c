#include "kernel.h"


const void *virtual_base_address = (const void*) 0xe0000000;


static struct address_space kernel_space;
static bool kernel_space_initialized = false;

void initializeKernelSpace()
{
	if (kernel_space_initialized)
	{
		return;
	}
	
	register uint32_t tt_base asm("r0");
	asm volatile("mrc p15, 0, r0, c2, c0, 0":"=r" (tt_base));
	
	tt_base = tt_base & (~ 0x3ff); // discard the first 14 bits, because they don't matter
	uint32_t *tt = (uint32_t*) tt_base;
	
	kernel_space.tt = tt_base;
	
	
	kernel_space.cpts = NULL;
	kernel_space.cptds = NULL;
	
	initSlabAllocator();
	
	
	
	// map the first sections where the interrupt vectors are as user mode no access
	tt[0] = newSD(0,0,0,0b01,0xa4000000);
	
	
	
	kernel_space_initialized = true;
}




void createAddressSpace()
{
	// TODO map the kernel pages and the first page, because the interrupt vectors are there
	
	
	
	
	
}


// only used to initialize the memory allocator and the virtual memory manager
void migrateKernelCPT(uint32_t section,uint32_t *migrate_cpt,uint32_t pages)
{
	section = section & (~ 0xfffff);
	LinkedList *cptd = requestLinkedListEntry();
	cptd->data = (void*) section;
	addLinkedListEntry(&kernel_space.cptds,cptd);
	
	LinkedList *cpt = requestLinkedListEntry();
	cpt->data = requestCPT();
	k_memset(cpt->data,0,1024);
	addLinkedListEntry(&kernel_space.cpts,cpt);
	
	k_memcpy(cpt->data,migrate_cpt,pages);
	
	
	kernel_space.tt[section >> 20] = newCPTD(0,cpt->data);
	
	clear_caches();
	invalidate_TLB();
	
	
	
}




void addVirtualKernelPage(void* page, void* virtual_address)
{
	uint32_t section = ((uint32_t) page) & (~ 0xfffff);
	uint32_t index = 0;
	LinkedList *sec = searchLinkedListEntry(&kernel_space.cptds,(void*) section,&index);
	if (sec == NULL)
	{
		
		LinkedList *cptd = requestLinkedListEntry();
		cptd->data = (void*) section;
		addLinkedListEntry(&kernel_space.cptds,cptd);
		
		LinkedList *cpt = requestLinkedListEntry();
		cpt->data = requestCPT();
		k_memset(cpt->data,0,1024);
		addLinkedListEntry(&kernel_space.cpts,cpt);
		
		kernel_space.tt[section >> 20] = newCPTD(0,cpt->data);
		
		uint32_t table_index = (uint32_t) (virtual_address-section);
		table_index = table_index / SMALL_PAGE_SIZE;
		uint32_t *table = cpt->data;
		table[table_index] = newSPD(1,1,0b01010101,page);
		
		
		
		// TODO if adding a new coarse page table descriptor, add it to all virtual address spaces
		
		
	}
	else
	{
		LinkedList *cpt = getLinkedListEntry(&kernel_space.cpts,index);
		if (cpt == NULL)
		{
			panic("no corresponding coarse page table for descriptor!\n");
		}
		uint32_t *table = cpt->data;
		uint32_t table_index = ((uint32_t) (virtual_address-section));
		table_index = table_index / SMALL_PAGE_SIZE;
		table[table_index] = newSPD(1,1,0b01010101,page);
	}
	clear_caches();
	invalidate_TLB();
}



static void expandKernelCPDTs()
{
	
	
	
	
}

uint32_t* getKernel_TT_Base()
{
	return kernel_space.tt;
}

void* getPhysicalAddress(uint32_t* space,void* address)
{
	uint32_t adr = (uint32_t) address;
	uint32_t descriptor = space[adr >> 20];
	if ((descriptor  & 0b11) == 0b0) // invalid descriptor
	{
		DEBUGPRINTF_3("invalid descriptor: %d\n",adr >> 20)
		return NULL;
	}
	if ((descriptor  & 0b11) == 0b10)
	{
		uint32_t section_base =  ((descriptor >> 20) << 20);
		DEBUGPRINTF_3("phys. address: %d\n",(section_base + ((adr << 12) >> 12)))
		return (void*) (section_base + ((adr << 12) >> 12));
	}
	if ((descriptor  & 0b11) == 0b01) // coarse page table
	{
		DEBUGPRINTF_3("coarse page table: %d\n",adr >> 20)
		uint32_t *cpt_base = (uint32_t*) (descriptor & (~ 0b1111111111));
		DEBUGPRINTF_3("coarse page table: %d\n",descriptor)
		DEBUGPRINTF_3("coarse page table base: %d\n",cpt_base)
		uint32_t index = ((adr & 0b11111111000000000000) >> 10)/4;
		DEBUGPRINTF_3("coarse page table index: %d\n",index)
		uint32_t page_descriptor = cpt_base[index];
		
		DEBUGPRINTF_3("small page descriptor: %d\n",page_descriptor)
		uint32_t page_offset = adr & 0b111111111111;
		uint32_t phys = (page_descriptor & (~ 0b111111111111)) + page_offset;
		DEBUGPRINTF_3("phys. address: %d\n",phys)
		return (void*) phys;// for small pages
		// TODO for large pages
	}
	if ((descriptor  & 0b11) == 0b11) // fine page table
	{
		DEBUGPRINTF_3("fine page table: %d\n",adr >> 20)
		// never used anyways
		return NULL;
	}
	DEBUGPRINTF_3("no known descriptor type: %d, %d\n",adr >> 20, descriptor)
	return NULL;
}


uint32_t newCPTD(unsigned char domain,uint32_t base_address)
{
	const uint32_t cptd_template = 0b000000000000000000000000010001;
	uint32_t desc = cptd_template;
	desc = desc | ((domain & 0b1111) << 5) | (base_address & (~ 0b1111111111));
	return desc;
}

uint32_t newSD(unsigned char c,unsigned char b,unsigned char domain,unsigned char ap,uint32_t base_address)
{
	const uint32_t sd_template = 0b00000000000000000000000000010010;
	uint32_t desc = sd_template;
	desc = desc | ((c & 0b1) << 3) | ((b & 0b1) << 2) | ((domain & 0b1111) << 5) | ((ap & 0b11) << 10) | (base_address & (~ 0b11111111111111111111));
	return desc;
}


uint32_t newLPD(unsigned char c,unsigned char b,unsigned char ap,uint32_t base_address)
{
	const uint32_t lpd_template = 0b00000000000000000000000000000001;
	uint32_t desc = lpd_template;
	desc = desc | ((c & 0b1) << 3) | ((b & 0b1) << 2) | (ap << 4) | ((base_address & 0xffff0000));
	return desc;
}

uint32_t newSPD(unsigned char c,unsigned char b,unsigned char ap,uint32_t base_address)
{
	const uint32_t lpd_template = 0b00000000000000000000000000000010;
	uint32_t desc = lpd_template;
	desc = desc | ((c & 0b1) << 3) | ((b & 0b1) << 2) | (ap << 4) | ((base_address & 0xfffff000));
	return desc;
}













void invalidate_TLB()
{
	asm volatile("mcr p15, 0, r0, c8, c7, 0":::"r0");
}






void clear_caches()
{
	asm volatile(
	"loop: mrc p15, 0, r15, c7, c10, 3 \n" // test and clean d-cache
	"bne loop \n"
	"mcr p15, 0, r0, c7, c7, 0\n" // invalidate cache
	:::"r0");
}


























