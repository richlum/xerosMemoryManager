/* user.c : User processes
 */

#include <xeroskernel.h>
#include <user.h>
#include <utility.h>
#include <sleep.h>

// initial test process
void dummyfunc(void){
	TRACE
	while(1){
		kprintf("Hello, I'm a dummy\n");
		sleep (4);
		sysyield();
	}
}


// initial process to start user space processes
void root(void){
	TRACE
	kprintf("root Hello World!\n");
	stackprint();
	
	// startup a user  process
	syscreate( &producer, DEFAULT_STACK_SIZE); 
	syscreate( &consumer, DEFAULT_STACK_SIZE); 

	
	
	for (;;) {
		#ifdef DEBUG
		kprintf("root is yielding\n");
		sleep(3);
		#endif
		sysyield();
	}
}

// user processes
void producer(void){
	TRACE
	#ifdef DEBUG
	stackprint();
	#endif
	int i = 0;
	for(i = 0;i<5;i++){
		kprintf("%d Happy----------------",i);
		#ifdef DEBUG
		sleep(2);
		#endif
		sysyield();
	}
	sysstop();
		
}


void consumer(void){
	TRACE
	#ifdef DEBUG
	stackprint();
	#endif 
	int i=0;	
	for (i=0;i<12;i++) {
		#ifdef DEBUG
		sleep(2);
		#endif
		kprintf("%d New Year!!!!!!!!!!!!!!!!!!\n",i);
		sysyield();
	}
	sysstop();
}


