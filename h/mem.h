#ifndef __MEM_H__
#define __MEM_H__

// signature to check on free to ensure memory address is consistent
#define SANITY 0xDA43 
#define PARAG_SIZE 16

// literal copy from cpsc415 slide 97
// the meta data about a memory block
struct memHeader{
	unsigned long size;
	struct memHeader *prev;
	struct memHeader *next;
	char* sanityCheck;	// for identifiable fingerprint to check address during free
	unsigned char dataStart[0]; // no impact on sizeof memHeader, yields address of freememory block following structure
};

// the anchor for the first memory block to allocate
extern struct memHeader *freeMemList;

// memory allocation deallocation methods
void* kmalloc(int requestedBytes);
void  kfree(void *ptr);

//utility function for debugging
void printFreeList(void);
char* hdrFromUsrPtr(char* ptr);


#endif
