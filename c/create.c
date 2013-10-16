/* create.c : create a process
 */

#include <xeroskernel.h>
#include <create.h>
#include <mem.h>
#include <disp.h>
#include <i386.h> // getCS
#include <xeroslib.h> //blkcopy
#include <utility.h>
#include <sleep.h>

//todo how do I get the parent pid?	
int get_parent_pid(void){
	return 99;
}

// given a memory header block calculate a start address at the high
// end of the memory block so stack can grow down towards the 
// start of the memory block.
// SAFTEYMARGIN is a temporary value to get us away from the end
// of our stack frame.
char* getstackpointer(struct memHeader* memhdr){
	return (char*) ((long)(memhdr->dataStart) + (long)(memhdr->size) - SAFETYMARGIN);
}

// build a stack from as if it was interrupted with an int 
// to allow iret to restart this "runningt" process
void initialize_my_stack(reg* esp,unsigned long eip ){
	DBGMSG("esp = %x, eip = %x\n" , *esp, eip);
	struct context_frame cf;
	cf.esi = 0;
	cf.edi = 0;
	cf.ebp = *esp;
	cf.esp = 0;
	cf.ebx = 0;
	cf.edx = 0;
	cf.ecx = 0;
	cf.eax = 0;
	cf.iret_eip = eip;
	cf.iret_cs = getCS();
	cf.eflags = 0;
	*esp = (reg*)(*esp - sizeof(struct context_frame));
	blkcopy((void*)*esp,(void*)&cf, sizeof(struct context_frame));
	#ifdef DEBUG
	DBGMSG("esp = %x\n", *esp);
	int i;
	reg* ptr;
	ptr=*esp;
	for(i=0;i<16;i++){
		DBGMSG("\t+ %x : %x\n", ptr,  *ptr ) ;
		ptr++;
	}
	sleep(5);
	#endif
}

// allocate a pcb, create the process and set it into read state
int create(void (*func)(void),int stack){
	TRACE
	//get free pcb
	int i=0;
	while ((proctable[i].PID!=-1)&&(i<MAXPROCS)){
		i++;
	}
	//ran out of process control blocks
	kassert(i<=MAXPROCS);
	
	pcb* myproc = &(proctable[i]);
	char* allocated = kmalloc(stack);
	struct memHeader* myMemoryHdr = (struct memHeader*) hdrFromUsrPtr(allocated);
	reg my_esp = (reg) getstackpointer(myMemoryHdr);
	DBGMSG("initializing stack for pid %d\n", i);
	//lets create as blocked state. not entering into blocked_q as we
	//  are going to place in ready_q right away
	initialize_process(myproc, i, BLOCKED,
		get_parent_pid(), my_esp);
		
	initialize_my_stack(&my_esp, (unsigned long ) func);
	myproc->esp = my_esp;	
	//add it to the ready_q
	ready(myproc);

	#ifdef DEBUG
		kprintf( "ready_q = %x\n", ready_q);
		kprintf("running = %x\n", running);
		kprintf("myproc addr = %x, pid = %d, state = %d, par_pid = %d, next = %0x\n",
			&myproc, myproc->PID, myproc->state, myproc->parent_pid, myproc->next);
		kprintf("myproc esp = %x \n", myproc->esp);

		kprintf(" stack just after placed inready_q\n");		
		showstack(myproc->esp);

	#endif
	return 1;
	
}
	
	
