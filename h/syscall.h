//syscall mechanism
#ifndef __SYSCALL_H__
#define __SYSCALL_H__
// key system calls for user to kernel services

extern int syscreate(void (*func)(void), int stack);
extern void sysstop(void);
extern void sysyield(void);
extern int syscall(int call, ...);

#endif
