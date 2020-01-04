
#include <stdarg.h>
#include <libndls.h>
#include <nucleus.h>
#include <hook.h>
#include <ngc.h>
#include <os.h>
#include <usbdi.h>
#include <usb.h>
#include <time.h>
#include <nspireio2.h> //printf doesn't default to serial port with nio2, use uart_printf

#include <dirent.h>
#include "zehn_loader.h"
#include <strings.h>


#include "include/osext.h"

//from ndless utils.c
static void __attribute__ ((noreturn)) ut_calc_reboot(void) {
	*(volatile unsigned*)0x900A0008 = 2; //CPU reset
	__builtin_unreachable();
}


// definitions
void setSyscall(int syscall_number,unsigned int address);
void extendSWIHandler();

typedef struct Library Library;
struct Library
{
	char *name;
	void *functiontable;
	Library *next;
};
struct Library *libs = NULL;
typedef struct
{
	char *name;
	char *translation;
} TranslationEntry;

int resolveDependencies(NUC_FILE *file);
int fileExtensionTriggered(char *path);

bool s_addLibrary();
void* s_requestLibrary();
bool loadLibrary(char *name);
Library* searchLibrary(char * name);
void initDynlinker();
void addLibraryEntry(char *name, void *functiontable);


void changeSetting(char * name,void *data);
int getSetting(char *name);
void loadSettings();
void saveSettings();
void defaultSettings();

//definitions end

#include "apis/graphics.h"

bool shareFunction(char *name,unsigned int function);

//include the core modules needed for dynamic module loading
#include "modules/settings/settings.h"
#include "modules/addsyscalls/addsyscalls.h"
#include "modules/dynlinker/dynlinker.h"

#include <ngc.h>


#define precisetime



#ifdef precisetime
	volatile unsigned int *timer;
	unsigned int lasttime = 0;
#else
	time_t lasthook;
#endif



//static void osgctest();

//#include "config.h"


#define MODULEPATH "/documents/modules/"



//#include "modules/definitions.h"


#include "modules/noflicker/noflickerold.h"



















/* 
keypad reads: 
os 4.5: 0x100e0f64	0x1070bd68
noncas 4.5: 0x100e1128


screen writes:
os 4.5: 0x10023798	0x100237d4	0x100237cc 0x10023808   ??: 0x1002380c 
noncas 4.5: 0x10023808 0x10023840	








*/



// from ndless sdk utils.c
void ut_disable_watchdog(void) {
	// Disable the watchdog on CX that may trigger a reset
	*(volatile unsigned*)0x90060C00 = 0x1ACCE551; // enable write access to all other watchdog registers
	*(volatile unsigned*)0x90060008 = 0; // disable reset, counter and interrupt
	*(volatile unsigned*)0x90060C00 = 0; // disable write access to all other watchdog registers
}

#define MAXFUNCS 100

unsigned int earlyfuncs[MAXFUNCS];
int earlyfunclength = 0;
unsigned int funcs[MAXFUNCS];
int funclength = 0;
unsigned int drawfuncs[MAXFUNCS];
int drawfunclength = 0;
unsigned int syscallfuncs[MAXFUNCS];
int syscallfunclength = 0;
bool addFuncToArray(unsigned int *array,int *length,int maxlength,unsigned int function)
{
	if (*length+2 >= maxlength)
		return false;
	array[*length] = function;
	*length = *length + 1;
	return true;
}
void removeFuncFromArrayIndexed(unsigned int *array, int *length,int index)
{
	for (int i = index;i<*length-1;i++)
	{
		array[i] = array[i]+1;
	}
	array[*length] = 0;
	*length = *length - 1;
}
void removeFuncFromArray(unsigned int *array, int *length,unsigned int function)
{
	for (int i = 0;i<*length;i++)
	{
		if (array[i] == function)
		{
			removeFuncFromArrayIndexed(array,length,i);
			return;
		}
	}
}
bool addEarlyFunction(unsigned int function)
{
	return addFuncToArray(earlyfuncs,&earlyfunclength,MAXFUNCS,function);
}
bool addFunction(unsigned int function)
{
	return addFuncToArray(funcs,&funclength,MAXFUNCS,function);
}
bool addSyscallFunction(unsigned int function)
{
	return addFuncToArray(syscallfuncs,&syscallfunclength,MAXFUNCS,function);
}
bool addDrawFunction(unsigned int function)
{
	return addFuncToArray(drawfuncs,&drawfunclength,MAXFUNCS,function);
}

void removeEarlyFunction(unsigned int function)
{
	removeFuncFromArray(earlyfuncs,&earlyfunclength,function);
}
void removeFunction(unsigned int function)
{
	removeFuncFromArray(funcs,&funclength,function);
}
void removeSyscallFunction(unsigned int function)
{
	removeFuncFromArray(syscallfuncs,&syscallfunclength,function);
}
void removeDrawFunction(unsigned int function)
{
	removeFuncFromArray(drawfuncs,&drawfunclength,function);
}

#define MAXSHARED 200
unsigned int sharedfuncs[MAXSHARED];
char *sharedfuncnames[MAXSHARED];
int sharedfunclength = 0;
bool shareFunction(char *name,unsigned int function)
{
	if (name == NULL)
		return false;
	if (sharedfunclength+2 >= MAXSHARED)
		return false;
	sharedfuncnames[sharedfunclength] = name;
	sharedfuncs[sharedfunclength] = function;
	sharedfunclength++;
	return true;
}
unsigned int searchFunction(char *name)
{
	if (name == NULL)
		return 0;
	for (int i = 0;i<sharedfunclength;i++)
	{
		if (strcmp(name,sharedfuncnames[i]) == 0)
		{
			return sharedfuncs[i];
		}
	}
	return 0;
}
#define MAXMODULES 30
void *modules[MAXMODULES];
char *modnames[MAXMODULES];
unsigned int uninstallhandler[MAXMODULES];
int modlength = 0;
void initModules()
{
	for (int i = 0;i<MAXMODULES;i++)
	{
		uninstallhandler[i] = 0;
	}
}
bool moduleLoaded(char *name)
{
	for (int i = 0;i<modlength;i++)
	{
		if (strcmp(modnames[i],name) == 0)
		{
			return true;
		}
	}
	return false;
}
bool addUninstallHandler(char *name, unsigned int func)
{
	for (int i = 0;i<modlength;i++)
	{
		if (strcmp(modnames[i],name) == 0)
		{
			if (uninstallhandler[i] != 0)
				return false;
			uninstallhandler[i] = func;
			return true;
		}
	}
	return false;
}
unsigned int argfuncs[] = {(unsigned int) addEarlyFunction,(unsigned int) addFunction,(unsigned int) addSyscallFunction,(unsigned int) shareFunction,(unsigned int) searchFunction, 
						   (unsigned int) removeEarlyFunction, (unsigned int) removeFunction,(unsigned int) removeDrawFunction,(unsigned int) removeSyscallFunction,(unsigned int) addDrawFunction};
void loadModule(char *name)
{
	void *base;
	bool hw_w;
	int (*entry)(int,char*[]);
	char *path = malloc(strlen(MODULEPATH)+strlen(name)+5);
	for (int i = 0;i<strlen(MODULEPATH)+strlen(name)+4;i++)
	{
		path[i] = '\0';
	}
	memcpy(path,MODULEPATH,strlen(MODULEPATH));
	strcat(path,name);
	uart_printf("Module path: %s\n",path);
	NUC_FILE *f = nuc_fopen(path,"rb");
	free(path);
	if (f == NULL)
	{
		uart_printf("file could not be opened\n");
		return;
	}
	if (zehn_load(f,&base,&entry,&hw_w) == 0)
	{
		modules[modlength] = base;
		modnames[modlength] = calloc(1,strlen(name)+5);
		memcpy(modnames[modlength],name,strlen(name));
		modlength++;
		entry(10,argfuncs);
		nuc_fclose(f);
		return;
	}
	else
	{
		uart_printf("invalid zehn file\n");
		nuc_fclose(f);
		return;
	}
}
void freeModule(char *name)
{
	for (int i = 0;i<modlength;i++)
	{
		if (strcmp(modnames[i],name) == 0)
		{
			if (uninstallhandler[i] != 0)
			{
				void (*handler)() = uninstallhandler[i];
				handler();
				free(modnames[i]);
				free(modules[i]);
				uninstallhandler[i] = 0;
				for (int a = i;a<modlength-1;a++)
				{
					modules[a] = modules[a+1];
					modnames[a] = modnames[a+1];
					uninstallhandler[a] = uninstallhandler[a+1];
				}
				modules[modlength] = NULL;
				modnames[modlength] = NULL;
				uninstallhandler[modlength] = 0;
				return;
			}
			else
			{
				return; // assuming a module can't be unloaded if it has no uninstall handler, this can lead to a full crash if it's code was damaged by another programm and then executed
			}
		}
	}
}
// TODO checksum for modules to check whether a module somehow got damaged, maybe using the sha hardware




static void hookfunc();



HOOK_DEFINE(mainhook)
{
	
	
	ut_disable_watchdog();
	int intmask = TCT_Local_Control_Interrupts(-1);
	
	hookfunc();
	
	TCT_Local_Control_Interrupts(intmask);
	HOOK_RESTORE_RETURN(mainhook);
};
#ifdef MODULE_CLOCK
	#ifdef precisetime
		unsigned int lastclock = 0;
	#else
		int hookcount = 0;
	#endif
#endif

int undef_timer = 0;
static bool cr4 = true;
static void hookfunc()
{
	
	
	
	
	
	
	// works without flickering in the emulator and CX CAS HW W+ here, not in the drawhook
	
	//uart_printf("early!\n");
	
	
	//Gc gc = gui_gc_global_GC();
	
	
	/*
	uint16_t * off_screen = (((((char *****)gc)[9])[0])[0x8])[0];
	*off_screen = rgbto565(255,0,0);
	*(off_screen+1) = rgbto565(255,0,0);
	*(off_screen+2) = rgbto565(255,0,0);
	*/
	
	
	
	
	
	/*
	// for now draw the clock here if the screen isn't 240*320, because the miniclock hook doesn't get called
	if (! cr4)
	{
		#ifdef MODULE_CLOCK
			#ifdef precisetime
				unsigned int time = *timer;
				if (abs(time-lastclock) > 200)
				{
					drawclock();
					lastclock = *timer;
				}
			#else
				if (hookcount > 200)
				{
					hookcount++;
				}
				else
				{
					hookcount = 0;
					drawclock();
				}
			#endif
		#endif
	}
	*/
	/*
	#ifdef MODULE_SECURITY
		if (isKeyPressed(KEY_NSPIRE_ESC) && (isKeyPressed(KEY_NSPIRE_HOME) || on_key_pressed()))
		{
			ptt_pressed();
			wait_no_key_pressed();
		}
	#endif
	*/
	
	for (int i = 0;i<earlyfunclength;i++)
	{
		if (earlyfuncs[i] != 0)
		{
			void (*func)() = (void (*)()) earlyfuncs[i];
			func();
		}
	}
	
	
	
	if (isKeyPressed(KEY_NSPIRE_HOME))
	{
		if (! use_undef_access)
		{
			use_undef_access = true;
			uart_printf("undef breakpoint enabled\n");
		}
	}
	
	
	#ifndef precisetime
		time_t chook = time(NULL);
		if (chook-lasthook > 0)
		{
			return;
		}
		lasthook = chook;
	#else
		unsigned int time = *timer;
		if (abs(time-lasttime) < 500)
		{
			return;
		}
		lasttime = time;
	#endif
	
	for (int i = 0;i<funclength;i++)
	{
		if (funcs[i] != 0)
		{
			void (*func)() = (void (*)()) funcs[i];
			func();
		}
	}
	
	
	
	/*
	if (use_undef_access)
	{
		volatile uint32_t *contrast = 0x900F0020;
		if (undef_timer < 4)
			undef_timer++;
		uart_printf("contrast: %x\n",*contrast);
		if (undef_timer >= 4 && (*contrast != 0x100 && *contrast != 0))
		{
			use_undef_access = false;
			uart_printf("undef breakpoint disabled\n");
		}
	}
	else
	{
		undef_timer = 0;
	}
	*/
	
	//uart_printf("mainfunc\n");
	
	
	// still gets called when calculator crashes after on
	
	if (isKeyPressed(KEY_NSPIRE_CTRL) && isKeyPressed(KEY_NSPIRE_9) && isKeyPressed(KEY_NSPIRE_ENTER) && isKeyPressed(KEY_NSPIRE_COMMA))
	{
		ut_calc_reboot();
	}
	
	if (isKeyPressed(KEY_NSPIRE_1) && isKeyPressed(KEY_NSPIRE_9))
	{
		disableNoflicker();
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	//bkpt();
	/*
	
	bool ctrl = isKeyPressed(KEY_NSPIRE_CTRL);
	#ifdef MODULE_SECURITY
		if (ctrl && isKeyPressed(KEY_NSPIRE_SPACE))
		{
			lockScreen();
			wait_no_key_pressed();
			return;
		}
		testScreentime();
	#endif
	if (ctrl && isKeyPressed(KEY_NSPIRE_1))
	{
		volatile int *powp = 0x900B0018;
		int pow = *powp;
		pow &= 0b111101111111111111111111111111111;
		*powp = pow;
	}
	#ifdef MODULE_NOFLICKER
		if (ctrl && isKeyPressed(KEY_NSPIRE_9))
		{
			installSleds();
		}
		if (ctrl && isKeyPressed(KEY_NSPIRE_8))
		{
			uninstallSleds();
		}
	#endif
	
	
	#ifdef MODULE_DESKTOP
		if (ctrl && isKeyPressed(KEY_NSPIRE_COMMA))
		{
			desktop();
			wait_no_key_pressed();
			return;
		}
	#endif
	#ifdef MODULE_SHELL
		if (ctrl && isKeyPressed(KEY_NSPIRE_PI))
		{
			shell();
			wait_no_key_pressed();
			return;
		}
	#endif
	*/
	
	
	
	
	
}



/*
static void osgctest() // to get osgc address
{
	Gc gc = gui_gc_global_GC();
	gui_gc_begin(gc);
	gui_gc_setColorRGB(gc,255,0,0);
	bkpt();
	gui_gc_fillRect(gc,0,0,100,100);
	bkpt();
	gui_gc_finish(gc);
	bkpt();
}
*/



static const unsigned int hook_addrs[] =
{0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,
 0x0, 0x0,
 0x0, 0x0,
 0x0, 0x0,
 0x0, 0x0,
 0x0, 0x0,//0x10011178 works with fopen!   at osgc read
 0x10011178, 0x10011134,
 //0x100237d4,//0x10023810
 0x0, 0x0
};
static const unsigned int draw_hook_addrs[] =
{0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,
 0x0, 0x0,
 0x0, 0x0,
 0x0, 0x0,
 0x0, 0x0,
 0x0, 0x0,
 0x10023810, 0x100237d4, //0x100237d4
 0x0, 0x0
};


HOOK_DEFINE(drawhook)
{
	
	ut_disable_watchdog();
	int intmask = TCT_Local_Control_Interrupts(-1);
	
	
	 // TODO hook is only for HW W+, becaus it's apparently in the 240*320 compatibility code
	
	
	
	//unsigned int *abt = 0x10;
	//uart_printf("abort handler: %x\n",*abt);
	
	
	//uart_printf("drawfunc\n");
	// apparently not called when noflicker is on with immediate return
	
	
	//volatile uint32_t *undef_handler = 0x24;
	
	//uart_printf("undef_handler: 0x%x\n",*undef_handler);
	
	
	if (isKeyPressed(KEY_NSPIRE_CTRL) && isKeyPressed(KEY_NSPIRE_9) && isKeyPressed(KEY_NSPIRE_ENTER) && isKeyPressed(KEY_NSPIRE_COMMA))
	{
		ut_calc_reboot();
	}
	
	
	if (! noflicker)
		return;
	//bkpt();
	memcpy(current_lcd_mirror,buffupreal,320*240*2);
	
	
	
	for (int i = 0;i<drawfunclength;i++)
	{
		if (drawfuncs[i] != 0)
		{
			void (*func)() = (void (*)()) drawfuncs[i];
			func();
		}
	}
	
	volatile uint32_t *buffup = 0xe0000010;
	//memset(*buffup,0,100*100*2);
	//memset(current_lcd_mirror,0,100*100*2);
	
	
	
	
	blitMirrorToScreen();
	
	
	//memset(*buffup,0,100*100*2);
	
	TCT_Local_Control_Interrupts(intmask);
	HOOK_RESTORE_RETURN(drawhook);
};



/*
Functions passed to modules by argv:
void registerHookFunc(void (*func)())	function executed in hook
void registerHoocFuncEarly(void (*func)())	function executed before time check
void registerDrawHookFunction(void (*func)()) function executed in drawhook
void registerAfterSyscallHandler(void (*func)()) executed after each syscall, so should not use any syscalls or do time-intensive tasks
void registerCleanupHandler(void (*func)())	function executed when module is unloaded
void registerModuleAddedHandler(void (*func)(char *module_name))	function executed when a new module is added
bool setSyscall(void (*func)())	setting a syscall to func, returns 0 if syscall wasn't set, returns 1 if syscall was set an as such couldn't be changed
void registerInternalFunc(char *name,void (*func)())
void (*func()) requestInternalFunc(char *name); returns the function pointer or NULL if there was no function with that name
*/

uint16_t* getcurrentmirror()
{
	return current_lcd_mirror;
}


int main(int argsn,char **argv)
{
	assert_ndless_rev(2014);
	
	
	
	
	uart_printf("argsn: %d\n",argsn);
	
	for (int i = 0;i<argsn;i++)
	{
		uart_printf("argv[%d]: %s\n",i,argv[i]);
	}
	
	
	
	// values aren't shared because osext is run again, communication has to be done through syscalls
	if (argsn > 1 && argv[1] != NULL) 
	{
		return fileExtensionTriggered(argv[1]);
	}
	
	
	
	initModules();
	
	
	
	
	
	//needs ndless.cfg.tns to work
	//creating it doesn't seem to work
	//an empty config file has to be transferred to the calc once it seems
	
	
	cfg_register_fileext("libp","osext");
	cfg_register_fileext("lib","osext");
	
	
	
	//initOSGCBUFF();
	#ifdef precisetime
		// init timer from nsSDL
		*(volatile unsigned *)0x900B0018 &= ~(1 << 11);
		*(volatile unsigned *)0x900C0080 = 0xA;
		volatile unsigned *control = (unsigned *)0x900C0008;
		*control = 0b10000010;
		timer = (unsigned *)0x900C0004;
	#else
		lasthook = time(NULL);
	#endif
	
	
	
	
	/*
	if (lcd_type() != SCR_240x320_565)
	{
		cr4 = false;
	}
	*/
	
	
	
	/*
	#ifdef MODULE_SETTINGS
		loadSettings();
	#endif
	
	#ifdef MODULE_SECURITY
		initlogin();
		lockScreen();
	#endif
	#ifdef MODULE_CLOCK
		hook_minicklock();
	#endif
	
	*/
	
	shareFunction("loadModule",loadModule);
	shareFunction("uart_printf",uart_printf);
	shareFunction("isKeyPressed",isKeyPressed);
	shareFunction("getcurrentmirror",getcurrentmirror);
	
	
	
	extendSWIHandler();
	initDynlinker();
	uart_printf("Modulepath: %s\n",MODULEPATH);
	DIR *dir = opendir(MODULEPATH);
	struct dirent *d;
	if (dir != NULL)
	{
		while ((d = readdir(dir)) != NULL)
		{
			//uart_printf("file: %s\n",d->d_name);
			char *ext = strchr(d->d_name,'.');
			if (strcmp(ext,".mod.tns") == 0)
			{
				uart_printf("module: %s\n",d->d_name);
				loadModule(d->d_name);
			}
		}
		closedir(dir);
	}
	else
	{
		uart_printf("could not open dir!\n");
	}
	
	
	
	
	HOOK_INSTALL(nl_osvalue(hook_addrs,32),mainhook);
	HOOK_INSTALL(nl_osvalue(draw_hook_addrs,32),drawhook);
	
	
	
	/*
	#ifdef MODULE_DISABLENAVNET
		#ifdef MODULE_SETTINGS
			int navnetstatus = getSetting("navnet");
			if (navnetstatus != -1 && IntSetting(navnetstatus) == 0)
			{
				int navnetnextboot = getSetting("navnet_next");
				if (navnetnextboot != -1)
				{
					if (IntSetting(navnetnextboot) == 0)
					{
						disablenavnet();
					}
					else
					{
						changeSetting("navnet_next",0);
						saveSettings();
					}
					
				}
			}
		#else
			disablenavnet();
		#endif
	#endif
	
	#ifdef MODULE_ADDSYSCALLS
		extendSWIHandler();
	#endif
	
	#ifdef MODULE_DYNLINKER
		initDynlinker();
	#endif
	*/
	
	
	
	
	
	
	
	nl_set_resident();
	
	/*
	#ifdef USBTEST_H
		ums_register();
	#endif
	
	
	#ifdef MODULE_NOFLICKER
		initSleds();
	#endif
	*/
	
	
	
	enableNoflicker();
	
	
	
	
	clear_cache();
	
	
	
	
	return 0;
}






