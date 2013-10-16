/* syscall.c : syscalls
 */

#include <xeroskernel.h>
#include <syscall.h>
#include <disp.h> //YIELD, CREATE,STOP
#include <utility.h>
#include <stdarg.h>


// system calls for user space to request kernel services

// the workhorse function that all other syscalls wil invoke
static	unsigned long int ssize;
static	int srequest;
static	void (*funcptr)(void);
int syscall(int call, ...){
	TRACE
	srequest=call;
	va_list ap;
	va_start(ap,call);
	switch (call){
		case NONE:
		case YIELD:
		case STOP:
			ssize=0;
			funcptr=NULL;
			break;
		case CREATE:
			ssize = va_arg(ap, int);
			funcptr = (void*)va_arg(ap, unsigned long int);
			break;
		default:
			kprintf("unknown call = %d\n",call);
			break;
		}
		
	// always pushing on 3 extra args for syscall for consitency
	// wastes some stack space but for our applciation, it saves
	// on code logic to only push 1 vs 2 vs 3 args.
	__asm__ volatile (" \
		pushl funcptr \n\
		pushl ssize\n\
		pushl srequest \n\
		int $50  \n\
		"
		:  			// outputs
		: 	// inputs
		: "%eax"					// clobbered
	);	
	
	return srequest;
}
	
//  call create with func pointer and stack from here.
int syscreate(void (*func)(void), int stack){
	TRACE
	int ppid=0;
	
	syscall(CREATE, stack, func,  ppid);
	return NULL;
}


void sysstop(void){
	TRACE
	syscall(STOP);
}
	
void sysyield(void){
	TRACE
	syscall(YIELD);
}





