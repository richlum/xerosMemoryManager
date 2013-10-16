// dispatcher
#ifndef __DISP_H__
#define __DISP_H__
#include <xeroskernel.h>

// request values to signal dispatcher
typedef enum {
	NONE, CREATE, YIELD, STOP
} request_t;



//removes next process from ready_q and returns a ptr to it 
//extern pcb* next(void);
//add process to ready_q
extern void ready(pcb* process);
extern void cleanup(pcb* process);
// policy engine for process switching
extern void dispatch(void);


#endif
