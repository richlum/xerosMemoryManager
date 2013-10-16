
#include <utility.h>
#include <xeroskernel.h>
#include <sleep.h>

// stackprint grabs the current executing stack ptr and prints it out.
// not very useufl, just shows the frame for stackprint itself.

void stackprint(void){
	TRACE
	unsigned long int my_esp;
	unsigned long int my_ebp;
	unsigned long int *my_stack;
	
	asm ("\
		movl %%esp,%0 \n\
		movl %%ebp,%1 \n\
		movl %%esp, %%eax \n\
		"
		: "=r" (my_esp), "=r" (my_ebp)  //outputs
		:	//inputs
		: "%eax"	//clobbered
		);
	DBGMSG("ebp = 0x%x\tesp = 0x%x\nCurrentStack:\n",my_ebp,my_esp);
	my_stack = (unsigned long int*)my_esp;
	#ifdef DEBUG
	while ((unsigned long)my_stack<=(unsigned long)my_ebp+12){
		kprintf("\t%x\t%x", my_stack, *my_stack);
		if (my_stack==my_ebp) kprintf("\t <==== ebp");
		kprintf("\n");
		my_stack++;
	}	
	sleep(5);
	#endif
}

// given a stack ptr, show the contents that follow
// allows hand verification of stack contents if you give it stack ptr.

void showstack(reg esp){
	#ifdef DEBUG
		reg* ptr;
		int i;
		ptr=esp;
		for(i=0;i<16;i++){
			DBGMSG("\t+ %x : %x\n", ptr,  *ptr ) ;
			ptr++;
		}
		sleep(5);
	#endif 
}
