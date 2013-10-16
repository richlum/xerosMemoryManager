/* xeroskernel.h - disable, enable, halt, restore, isodd, min, max */
#ifndef _XEROSKERNEL_
#define _XEROSKERNEL_

#include <user.h>  		//root process
#include <syscall.h>  	//system calls
#include <create.h>		//process creation

//maximum number of simultaneous processes allowed
#define MAXPROCS 128
//convience for per process stack size
#define DEFAULT_STACK_SIZE 1024	//bytes


// dispatcher
extern void dispatch(void);

/* Symbolic constants used throughout Xinu */

typedef	char		Bool;		/* Boolean type			*/
#define	FALSE		0		/* Boolean constants		*/
#define	TRUE		1
#define	EMPTY		(-1)		/* an illegal gpq		*/
#define	NULL		0		/* Null pointer for linked lists*/
#define	NULLCH		'\0'		/* The null character		*/


/* Universal return constants */

#define	OK		 1		/* system call ok		*/
#define	SYSERR		-1		/* system call failed		*/
#define	EOF		-2		/* End-of-file (usu. from read)	*/
#define	TIMEOUT		-3		/* time out  (usu. recvtim)	*/
#define	INTRMSG		-4		/* keyboard "intr" key pressed	*/
					/*  (usu. defined as ^B)	*/
#define	BLOCKERR	-5		/* non-blocking op would block	*/

/* Functions defined by startup code */


void bzero(void *base, int cnt);
void bcopy(const void *src, void *dest, unsigned int n);
int  kprintf(char * fmt, ...);
void lidt(void);
void init8259(void);
void disable(void);
void outb(unsigned int, unsigned char);
unsigned char inb(unsigned int);

// memory allocator
extern void  kmeminit(void);
extern void* kmalloc(int requestedBytes);
extern void  kfree(void *ptr);


//register type 
typedef  unsigned long reg;


// context structure that will save registers on to stack
// ref slide 185

// extened to include int pushed values.  only accurate
// on completion of a user called interrupt

typedef struct context_frame {
	reg   edi;
	reg   esi;
	reg   ebp;
	reg   esp;
	reg   ebx;
	reg   edx;
	reg   ecx;
	reg   eax;
	reg   iret_eip;
	reg   iret_cs;
	reg   eflags;
} contextframe;

// push all required state registers on to
// stack and save only sp in cpu state
//struct CPU {
	//reg sp;
//};

//process state type
typedef  unsigned long state_type ;
#define READY   1
#define RUNNING 2
#define DEAD    3
#define BLOCKED 4

//interrupt number for contextswitch
#define CONTEXTSWITCH_INTERRUPT 50 



// process control block type	
typedef struct process_control_block {
	int PID;
	state_type state;
	int parent_pid;
	reg esp; // stack pointer 
	struct process_control_block* next;
} pcb;


extern pcb* running;
extern pcb* ready_q;
extern pcb* blocked_q;
extern pcb* dead_q;
extern pcb proctable[];

// default, initilizes next to null
extern void initialize_process(pcb* proc, 
		int PID,
		state_type state,
		int parent_pid,
		reg esp);


#endif
