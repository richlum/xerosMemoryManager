/* disp.c : dispatcher
 */

#include <xeroskernel.h>
#include <disp.h>
#include <create.h>  // create
#include <syscall.h>  //syscreate, syscall, sysstop, sysyield
#include <ctsw.h>
#include <utility.h>
#include <sleep.h>

//removes next process from head of ready_q and returns a ptr to it 
// set state to running as next always goes from ready to running
// returns null if no more processes in queue - caller must check for null
pcb* next(void){
TRACE
	pcb* process = NULL;
	if(ready_q){
		process = ready_q;
		ready_q = ready_q->next;
		process->next=NULL;
		DBGMSG("next returning process 0x%x\n", process->PID);
		kassert(process->parent_pid);
		// next is only to go from READY to RUNNING
		kassert(process->state == READY);
		process->state=RUNNING;
		// need to have a stack pointer to process state vars
		//kassert(process->esp);
	}
	if (!process) kprintf("nothing in ready_q, returning null\n");
	//global to get current running process
	running = process;  
	return process;
}

//add process to tail of ready_q
//set state to ready
//precondition: next==null, caller must remove links prior to calling ready
void ready(pcb* process){
TRACE
	if (!process){
		kprintf("trying to queue a null process, ignoring\n");
		return;
	}
	// calling method must remove links before calling us
	kassert(process->next==NULL);
	// make sure the rest of process has some values
	kassert(process->PID>=0);
	kassert(process->parent_pid>=0);
	kassert(process->esp>=0);
	kassert(process->state>0);//valid esp
	
	pcb* next=NULL;
	pcb* ptr = ready_q;
	if (ptr!=NULL)
		next = ptr->next;
	else
		ready_q = process;
	while(next!=NULL){
		ptr=next;
		next=ptr->next;
	}
	ptr->next=process;
		
#ifdef DEBUG
	next=ready_q;
	kprintf("ready_q members\n");
	while(next){
		kprintf("process address %x, pid = %d, state = %d, esp = %x \n", 
			&next, next->PID, next->state, next->esp);
			next=next->next;
		}
	sleep (5);

#endif
	DBGMSG("Process %d moving from state %d to READY(%d)\n",
		process->PID, process->state, READY);
	process->state=READY;
	
}

//precondition: remove pcb->next before handing us process
void cleanup(pcb* process){
	TRACE
	//caller must remove next before handing us process
	kassert(process->next==NULL);
	//insert process into head of dead_q
	process->next=dead_q;
	dead_q=process;
	DBGMSG("moving from state %d to DEAD(%d)\n",process->state, DEAD);
	process->state=DEAD;	
	//todo : kfree? how do we find malloced memory associated with this process
}



//policy engine for process switching.  who gets to run
// todo, while this works, we should get the address of the args
// within the stack and put it in pcb as a field.  That way
// we can retrieve args based on pcb entry rather than
// hardcoded offset from stackptr.
//   was implemented in different version but ran out of time to port 
//   back to here.

void dispatch(void){
TRACE 
	request_t request;
	unsigned long int ptr = NULL;
	unsigned long int size = 0;
	void (*func)(void) = NULL;

	while(!running)
		running = next();

	pcb* process = running;
	process->state = RUNNING;
	
	for( ;; ) {
		TRACE
		request = contextswitch( process );
		switch( request ) {
			case( CREATE )	: 
				ptr = process->esp;
				ptr += (sizeof( contextframe)); //includes gp reg, eip, cs, flag
				kassert(*((unsigned long int*)ptr)==CREATE);
				size = *(((unsigned long int*)ptr)+1);
				func = *(unsigned long int*)(ptr + 2 * (sizeof (unsigned long int)));
				DBGMSG("creating proc stack = %d, funcptr = %x\n",size, func);
				create( func, DEFAULT_STACK_SIZE); 
				break;
			case( YIELD )	: 
				ready	( process ); 
				process = next(); 
				DBGMSG("pid = %d\n",process->PID);
				break;
			case( STOP )	: 
				cleanup( process ); 
				process = next(); 
				break;
			default :
				kprintf("request %d unknown\n", request);
				break;
		}
	}

	
}

