#ifndef VIRTUAL_MM_H
#define VIRTUAL_MM_H

#include "../LinkedList.h"

extern void* const virtual_base_address;
extern const volatile void* remapped_RAM;
extern const void* old_RAM;
extern const void* remapped_stack_end;
#define REMAPPED_STACK_SIZE 32
extern const void* remapped_stack;



struct address_space {
	volatile uint32_t *tt;
	LinkedList *cpts; // store the coarse page tables itself (1K table)
	LinkedList *cptds; // store the corresponding addresses for the coarse page tables (address to pass to newCPTD)
};

bool initializeKernelSpace();

struct address_space* getKernelSpacePointer();

void addVirtualKernelPage(const void* page, const void* virtual_address);
void addVirtualKernelPage_noncached(const void* page, const void* virtual_address);

void migrateKernelCPT(uint32_t section,uint32_t *cpt,uint32_t pages);

void addVirtualPage(struct address_space *space,const void* page, const void* virtual_address);

struct address_space* createAddressSpace();
// you should switch out of the address space before destroying it
void destroyAddressSpace(struct address_space *space);
void changeAddressSpace(struct address_space *space);
void intoKernelSpace();

void freePagesFromCoarsePageTable(uint32_t *cpt);

uint32_t newCPTD(unsigned char domain,uint32_t base_address);
uint32_t newLPD(unsigned char c,unsigned char b,unsigned char ap,uint32_t base_address);
uint32_t newSPD(unsigned char c,unsigned char b,unsigned char ap,uint32_t base_address);
uint32_t newSD(unsigned char c,unsigned char b,unsigned char domain,unsigned char ap,uint32_t base_address);


void intoKernelSpaceSaveAddressSpace();
void restoreAddressSpace();

uint32_t* getKernelCPT(void* address);


void invalidate_TLB();
void clear_caches();

volatile uint32_t* getKernel_TT_Base();
void* getPhysicalAddress(struct address_space *space,void* address);
void* getKernelPhysicalAddress(void* address);

bool virtual_mm_self_test();


#endif