#ifndef __EVEC_H__
#define __EVEC_H__

//get access to set_evec, which is a static funtion and not reachable
//external to evec.c
extern void (*set_exception)(unsigned int xnum, unsigned long handler);
#endif
