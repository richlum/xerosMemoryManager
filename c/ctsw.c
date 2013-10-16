/* ctsw.c : context switcher
 */
#include <xeroskernel.h>
#include <ctsw.h>
#include <utility.h>
#include <disp.h> //YIELD, func
#include <sleep.h>


// all private to contextswitch
// static so that assembly can directly access their addresses 
// without requiring reference to ebp or esp

static void *k_stack;
static unsigned int ESP;    
static unsigned int req;
static unsigned int REQ;
request_t contextswitch(pcb* process){
	//save this processes stack pointer
	TRACE
	reg* ptr;
	ESP = process->esp;
	
	DBGMSG(" process %d to be restored (before context switcher iret)\n",
		process->PID);
	showstack(ESP);
	ptr = ESP;
	contextframe* cf = (contextframe*)ESP;
	DBGMSG("ESP cast as cf, esp=%x, ebp=%x, iret=%x,iret_cs=%x\n",
		cf->esp, cf->ebp, cf->iret_eip, cf->iret_cs);
	
	DBGMSG("context switch ESP = %x\n",ESP);
	
	//REQ={create|yield|stop}
	//was pushed on in syscall just before interrupt
	//int places eip,cs,flag on stack, so REQ is 3*4 deep = 0xc
    __asm__ volatile( "\
    .globl	_ISREntryPoint \n\
		pushf \n\
		pusha \n\
		movl %%esp, k_stack \n\
		movl ESP, %%esp \n\
		popa \n\
		iret \n\
	_ISREntryPoint: \n\
		movl 0xc(%%esp), %%eax \n\
		movl %%eax, REQ \n\
		pusha \n\
		movl %%esp, ESP \n\
		movl k_stack, %%esp \n\
		popa \n\
		popf \n\
		"
	:   //output to eax
	: 			//input
	: "%eax"	//clobbered
    );
    
	DBGMSG("call = %d\n", REQ);
    process->esp= ESP;
// write into disp.c shared variables

	//todo extract syscall return value and return it here
	//return rv;
	return REQ;
}
