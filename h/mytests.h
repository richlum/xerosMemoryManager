#ifndef __MYTESTS_H__
#define __MYTESTS_H__

#include <mem.h>
// size of test array to hold test addresses for random memory tests
#define MAXADDR 2048

// test functions
void testkmalloc(void);
void testkfree(void);
int freeMemListOK(void);
int memHeaderOK(struct memHeader* hdr);
void showAllocatedHeader(char* ptr);
void allocXfreeModY(int x, int y);

#endif
