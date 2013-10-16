/* initialize.c - initproc */

#include <i386.h>
#include <xeroskernel.h>
#include <xeroslib.h>
#include <utility.h>
#include <mytests.h>
#include <sleep.h>
#include <evec.h>
#include <ctsw.h>
#include <user.h>
#include <create.h>

extern	int	entry( void );  /* start of kernel image, use &start    */
extern	int	end( void );    /* end of kernel image, use &end        */
extern  long	freemem; 	/* start of free memory (set in i386.c) */
extern  char	*maxaddr;	/* max memory address (set in i386.c)	*/

/************************************************************************/
/***				NOTE:				      ***/
/***								      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED.  The     ***/
/***   interrupt table has been initialized with a default handler    ***/
/***								      ***/
/***								      ***/
/************************************************************************/

//globals for kernel implementation
pcb proctable[MAXPROCS];
pcb* running;
pcb* ready_q;
pcb* blocked_q;
pcb* dead_q;

pcb dummy;

//use PID=-1 to indicated if pcb is in use
void initialize_queues(void ){
	TRACE

	running=NULL;
	ready_q=NULL;
	blocked_q=NULL;
	dead_q=NULL;
}

/**
	int PID;
	state_type state;
	int parent_pid;
	reg esp; // stack pointer 
	struct process_control_block* next;
	**/
	
void initialize_process(pcb* proc, 
	int PID,
	state_type state,
	int parent_pid,
	reg esp){

	proc->PID=PID;
	proc->state=state;
	proc->parent_pid=parent_pid;
	proc->esp=esp;
	proc->next=NULL;
}

// array of dead pcb's
void initialize_proctable(void){
	TRACE
	int i;
	for (i=0;i<MAXPROCS;i++){
		initialize_process(&(proctable[i]), -1, DEAD, -1, 0);
	}
	
}

// initialize interrupt table
void context_init(void){
	TRACE
	//void (*myfunc)(void);
	//myfunc = &(_ISREntryPoint);
	set_exception((unsigned int)CONTEXTSWITCH_INTERRUPT,(unsigned long)&_ISREntryPoint);
}


/*------------------------------------------------------------------------
 *  The init process, this is where it all begins...
 *------------------------------------------------------------------------
 */
void initproc( void )				/* The beginning */
{
	kprintf( "\n\nCPSC 415, 2012W1 \n32 Bit Xeros 1.1\nLocated at: %x to %x\n", &entry, &end ); 

	DBGMSG( "first free word after kernel stack  is freemem = %x\n", freemem);
	DBGMSG( "maximum memory address, maxaddr = %x\n", maxaddr);
	DBGMSG( "holestart = %x, holend = %x, holesize = %x \n", HOLESTART, HOLEEND, HOLESIZE);
	sleep (3);

	kmeminit();

// note randomMemoryTest never returns - continous test
#ifdef MEMTEST
	testkmalloc();
	testkfree();
	randomMemoryTest(314159);
#endif	

	initialize_proctable();
	initialize_queues();
	
    //initialize interupt table
	context_init();
	
	//create user process
	create( root, DEFAULT_STACK_SIZE); 

	//#start dispatcher
	dispatch();

	
        /* This code should never be reached after you are done */
	for(;;); /* loop forever */
}


