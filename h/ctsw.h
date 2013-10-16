// context switcher
#ifndef __CTSW_H__
#define __CTSW_H__
#include <disp.h>

// contextswitcher controls kernel to user space and vice versa
// _ISREntryPoint is the location for interrupt  to vector to switch
// from user space to kernel space for kernel services

extern request_t contextswitch(pcb* process);
void _ISREntryPoint(void);
#endif
