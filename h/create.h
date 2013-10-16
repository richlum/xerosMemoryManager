// process creator
#ifndef __CREATE_H__
#define __CREATE_H__

//offset from end of memory area for stack while developing
//#define SAFETYMARGIN 16  		//bytes
#define SAFETYMARGIN 0  		//bytes

// create a process launching func with stack size of stack
// func must not return, otherwise, we run of end of stack
// it must call sysyield or systop when done.
extern int create(void (*func)(void),int stack);

#endif
