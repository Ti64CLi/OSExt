#include "kernel.h"






asm(
".extern (main)\n"
"__init_lr: .word 0 \n"
"__init_sp: .word 0 \n"
"__entry: .global __entry\n"
//" .long 0xE1212374\n" // bkpt
" str lr, __init_lr\n"
" str sp, __init_sp\n"
" b main"
);








// some variables to test relocation
static uint32_t test = 20;

static uint32_t *test2 = &test;

static uint32_t *test3 = NULL;
// stack grows downwards!
/*
void teststack(void* t)
{
	int h = 1;
	if (&h < t)
	{
		uart_printf("stack grows downwards!\n");
	}
	else
	{
		uart_printf("stack grows upwards!\n");
	}
}
*/
// because we return with main, every error here is still recoverable without a kernel panic
int main(int argsn,char **argc)
{
	// no need to make the kernel resident, allocated memory isn't freed by ndless, so we can just copy the kernel and it will stay
	
	//int t = 0;
	//teststack(&t);
	
	
	
	
	if (argsn == 1 && ((unsigned int) argc) == 0x53544c41) //STandaloneLAunch
	{
		DEBUGPRINTF_1("relocated\n")
		// relocation finished or already done by loader
		initialize();
	}
	else
	{
		//test3 = ti_malloc(8);
		//uart_printf("test3: 0x%x\n",test3);
		relocate_self();
	}
	return 0;
}



// because we return with main after this, every error here is still recoverable without a kernel panic
void initialize()
{
	void* framebuffer = (void*) *LCD_UPBASE;
	framebuffer_fillrect(framebuffer,0,0,320,240,0,0,0);
	
	debug_shell_println("finished relocating");
	debug_shell_println("initializing");
	//TODO initialize physical and virtual memory manager properly, put large page descriptors for kernel space in dma memory
	
	
	/*
	for (uint32_t i = 0;i<50;i++)
	{
		debug_shell_println("i: %d",i);
	}
	*/
	
	debug_shell_println("performing physical memory manager self test");
	bool b = physical_mm_self_test();
	if (! b)
	{
		debug_shell_println("error in physical memory manager self test         aborting");
		keypad_press_release_barrier();
		free_init_pds();
		return;
	}
	
	debug_shell_println("allocating memory");
	allocPageblock(128);
	allocPageblock(128);
	allocPageblock(128);
	allocPageblock(128); // allocate 2mb
	debug_shell_println("done");
	
	
	/*
	register uint32_t domains asm("r0") = 0;
	asm volatile("mrc p15, 0, r0, c3, c0, 0":"=r" (domains));
	DEBUGPRINTF_3("domains: 0x%x\n",domains); // domain 0 is client, so we can use it for everything, because access permissions are checked
	*/
	
	debug_shell_println("performing virtual memory manager self test");
	b = virtual_mm_self_test();
	if (! b)
	{
		debug_shell_println("error in virtual memory manager self test         aborting");
		keypad_press_release_barrier();
		free_init_pds();
		return;
	}
	
	debug_shell_println("initializing kernel space");
	initializeKernelSpace();
	free_init_pds();
	debug_shell_println("done");
	//asm(".long 0xE1212374"); // bkpt
	
	
	
	
	debug_shell_println("running general self test");
	b = run_self_test();
	if (! b)
	{
		debug_shell_println("error in general self test         aborting");
		keypad_press_release_barrier();
		return;
	}
	
	
	
	
	
	
	
	
	
	debug_shell_println("osext installed");
	debug_shell_println("press any key to exit");
	// to be able to read the messages
	keypad_press_release_barrier();
}



























