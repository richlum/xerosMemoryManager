/* mem.c : memory manager
 */

#include <xeroskernel.h>
#include <i386.h>  // access freemem, HOLESTART, HOLEEND, maxaddr
#include <utility.h>
#include <mem.h>

/* Your code goes here */



// the anchor for the first memory block to allocate
struct memHeader *freeMemList;
// local working buffer for memHeader 
struct memHeader *memSlot;


// initially allocate 2 memory blocks, one before HOLE, one after
void kmeminit(void){
TRACE
	// use freemem as start point but advance to first page boundary if required
	char* start = (char*)freemem;
	while (((unsigned long int)start)%16 != 0)
		start++;
	// anchor first memory header 
	memSlot = (struct memHeader *)start;
	
DBGMSG("freemem = %x, set start = %x\n",freemem, start);
	//allocate first free memory block between freemem and HOLE
	memSlot->size=(HOLESTART - (int)memSlot->dataStart);
	memSlot->prev=0;
	memSlot->next=(struct memHeader*)(HOLEEND);
	memSlot->sanityCheck = (char*) SANITY;
	freeMemList = memSlot;   // address of 1st memHeader block
	// second memory header at HOLEEND to maxaddr
	memSlot = (struct memHeader*)HOLEEND;
	memSlot->size=((int)maxaddr - HOLEEND +1) - sizeof(struct memHeader);
	memSlot->prev=freeMemList;
	memSlot->next=0;
	memSlot->sanityCheck = (char*) SANITY; 
#ifdef DEBUG
	printFreeList();
#endif	
}

// return address of first byte of contiguous memory block of min size requestedBytes
// return null if unable to fullfill request
void* kmalloc(int requestedBytes){
	DBGMSG("kmalloc(%d)\n", requestedBytes);

	if (requestedBytes==0)
		return 0;	// we dont allow header allocation of zero bytes
		
	// round up requested amount to next PARAG_SIZE block and add header size
	int amt = (requestedBytes)/PARAG_SIZE + ((requestedBytes%PARAG_SIZE)?1:0);
	amt = amt*PARAG_SIZE + sizeof(struct memHeader);
	// amt in bytes for user data and header
	
	//check if we have big enoug block available
	struct memHeader* hdr = freeMemList;
	while((hdr->size < amt)&&(hdr->next!=0)){
		 hdr=hdr->next;
	}
	if (hdr->size < amt){
		 kprintf("Not enough Memory to allocate %d bytes\n", requestedBytes);
		 return 0;
	}
	
	// we have a hdr block big enough to fullfill request
	// do we have enough room left to write the next a memHeaderBlock and 
	//    at least 1 paragraph of user data?(32 byte min for hdr and user data)
	if ((hdr->size - amt) < sizeof(struct memHeader) + PARAG_SIZE) {
		// not enough room in remainder of memory block to write a memHeaderBlock
		// give whole block to user
		if (hdr==freeMemList){
			//this is the first block, make freeMemList point to the next
			//to remove this block from the free memory linked list
			freeMemList=hdr->next;
			hdr->next->prev=NULL;
		}else{
			//this is a block somehere in the linked list of free memory blocks 
			//remove this block from the free memory linked list.
			hdr->prev->next = hdr->next;
			hdr->next->prev = hdr->prev;
		}
	}else{	
		// subdivide free memory block, giving user hdr and make newHdr 
		// 		for smaller free memory block
		
		//start is address we will use for new memheader after memory
		//  to be given to user
		//hdr is the existing memory header block we will alloc from
		//offset start (new hdr) from existing header by amt
		unsigned char* start = (unsigned char*)(hdr) +amt;
		 
		// make sure we start on a page boundary for new memory header
		// 		for new smaller memory block	
		while (((unsigned long int)start)%16 != 0)
			start++;
			
		//hdr = memory block for user request
		//did we create a gap between start of new memory header and
		// 	end of user allocated data, put it into allocated memory 
		//	in mem hdr for the user block 
		unsigned long int gap = (unsigned long int) start - ((unsigned long int)(hdr) + (unsigned long int)amt);

		//make remainder of original freememory block available in freelist
		unsigned char* newMemBlockAddr = (unsigned char*) (start);
		struct memHeader* newHdr = (struct memHeader*)newMemBlockAddr;
		//amt includes allocation for memHeader but size excludes memHeader (user requested size)
		//any gap we created is in user block and is taken out of newHdr freemem
		newHdr->size=hdr->size - amt - gap ;
		newHdr->sanityCheck = (char*)SANITY;		
		kassert((((int)newHdr)&0xf) == 0);
			
		// update user hdr block size
		//amt includes memHeader, size excludes any memHeader
		//any gap created is put into this memory block that will be given to user
		hdr->size=amt+gap-(sizeof(struct memHeader)); 
		
		if (hdr==freeMemList){
			//giving first block in freeMemList to user.
			//to remove this block from the free memory linked list
			hdr->next->prev = newHdr;
			freeMemList=newHdr;
			newHdr->prev=NULL;
			newHdr->next=hdr->next;
		}else{
			//giving user a block somehere in the linked list of free memory blocks 
			//remove old hdr block from the free memory linked list by replacing
			//  pointers to hdr with pointers to newHdr
			hdr->prev->next=newHdr;
			hdr->next->prev=newHdr;
			//point this new block to old hdr's prev and next
			newHdr->prev=hdr->prev;
			newHdr->next=hdr->next;
		}
	}
	//remove pointers into free memory blocks from this user allocated hdr
	hdr->next=NULL;
	hdr->prev=NULL;
	//rewrite SANITY just to make sure before we let the user have at it
	hdr->sanityCheck=(char*)SANITY;
	// must be on a page boundary (16 byte address, so 4 least sig bits always zero
	kassert((((int)hdr->dataStart)&0xf) == 0);

	return (void*) (hdr->dataStart);
}

/*
 * given an address return true if that address is already covered by freeMemList
 * 
 * */
int addressIsFree(void* ptr){
	struct memHeader* hdr = freeMemList;
	while (hdr!=NULL){
		//is this a hdr in freeMemList
		if ((void*)hdr == ptr)
			return 1;
		//is this address inside this freeMemList hdr block
		if (((int)ptr >  (int)hdr)&&
			((int)ptr < ((int)(hdr->dataStart) + hdr->size)))
			return 1;
		hdr=hdr->next;
	}
	return 0;
}


//given 3 nodes either link them or coallesce them
//Precondition:  lowerbound must be linked to upperbound 
//		and target address is between them
void link_or_coallesce(unsigned char* lowerbound,
						unsigned char* upperbound, 	unsigned char* target){
		struct memHeader* targethdr = (struct memHeader*)target;
		struct memHeader* prev = (struct memHeader*)lowerbound;
		struct memHeader* next = (struct memHeader*)upperbound;
		kassert (prev->sanityCheck==(char*)SANITY);
		kassert (next->sanityCheck==(char*)SANITY);
		kassert (targethdr->sanityCheck=(char*)SANITY);
		
		// check for coallesing on lower side
		//   memory blocks have to be minimum 2 paragraphs apart in order
		// 	 to be linked in free list, otherwise coallesce into 1 memory block
		if (((long int)target -((long int)(prev->dataStart) + 
			(long int)(prev->size))) < (2*sizeof(struct memHeader)))  { 
			
			if (((long int)next - ((long int)(targethdr->dataStart) + 
				(long int)(targethdr->size))) < (2*sizeof(struct memHeader))){
				
				//coallese prev, target, next
				prev->size += targethdr->size + sizeof(struct memHeader) +
						next->size + sizeof(struct memHeader);
				next->next->prev=prev;
				prev->next=next->next;
				//make sure we dont mis-interpret, no longer valid headers.
				//removing targethdr and next memory headers
				targethdr->sanityCheck=0x0;
				targethdr->next=0x0;
				targethdr->prev=0x0;
				next->sanityCheck=0x0;
				next->prev=0x0;
				next->next=0x0;
			}else{
				//coallesc (prev, target) link next
				prev->size+=targethdr->size + sizeof(struct memHeader);
				//these pointers should already be in place but lets just make sure
				prev->next=next;
				next->prev=prev;
				//removing targethdr 
				targethdr->sanityCheck=0x0;
				targethdr->next=0x0;
				targethdr->prev=0x0;
			}
		}else if (((long int)next - ((long int)(targethdr->dataStart) + 
			(long int)(targethdr->size))) < (2*sizeof(struct memHeader))){
			
				//coallese (target, next), link prev
				targethdr->size += next->size + sizeof(struct memHeader);
				targethdr->prev=prev;
				prev->next=targethdr;
				targethdr->next=next->next;
				next->next->prev=targethdr;
				//removing next memHeader
				next->sanityCheck=0x0;
				next->next=0x0;
				next->prev=0x0;
		}else{
			// link prev target next
			targethdr->prev=prev;
			prev->next=targethdr;
			targethdr->next=next;
			next->prev=targethdr;
		}
}



// when user returns memory, check for sanity, if pass, then insert into
// appropriate spot in freeMemList (in address order)
// Otherwise it's been trampled by lower address user, in 
// which case halt system since we have no way to ask user to try again.
void kfree(void *ptr){
	unsigned char* target = (unsigned char*) ptr - sizeof(struct memHeader) ;
	DBGMSG("kfree ((0x%x),  msghdr = 0x%x\n", target);
	struct memHeader* targethdr = (struct memHeader*) target;
	
	kassert ((((unsigned long int)ptr)&0xf) == 0);
	kassert ( ((char*)ptr<(char*)HOLESTART)||((char*)ptr>=(char*)HOLEEND) );
	kassert ( (int)ptr >= (int)freemem );
	kassert ( ((int)targethdr->dataStart + targethdr->size) <= (int)maxaddr +1 );
	
	// is user trying to free memory that is already free, if so ignore
	if (addressIsFree(ptr)){
		DBGMSG("trying to free (0x%x) and address that is already free\n",ptr);
		return;
	}
	// if address is already free, this maybe an old defunct header(sanitycheck =0x0)
	kassert ((int)targethdr->sanityCheck==SANITY);
	// is there any existing free memory
	if (freeMemList==NULL){
		//there is currently no free memory : this will be the only one
		DBGMSG("returning first block to an empty freeMemList, 0x%x\n",targethdr);
		freeMemList=targethdr;
		targethdr->prev=NULL;
		targethdr->next=NULL;
		return;
	}

	// find the free memory block just below us and just above us so 
	// we know where to insert the newly free memory block
	struct memHeader* hdr = freeMemList;
	unsigned char* lowerbound;
	unsigned char* upperbound;
	lowerbound = (unsigned char*) 0x0;
	upperbound = (unsigned char*) 0x0;
	while(hdr!=NULL){
		//traversing from low address to high
		if(((long int)target - (long int)hdr)>0){
			//keeps getting smaller, stop when negative
			lowerbound=(unsigned char*)hdr; 
		}else{
			//only trigger once, the first negative entry above
			if(upperbound==0x0){
				upperbound=(unsigned char*)hdr;
				break;
			}
		}
		hdr=hdr->next;
	}
	
	if (lowerbound==0){
		// no free memory block below us.
		// memory block to be inserted at head
		// memory blocks have to be 2 paragraphs apart to be linked, otherwise coallesce
		if (((long int)freeMemList - ((long int)(targethdr->dataStart) + 
			(long int)(targethdr->size))) < (2*sizeof(struct memHeader))){
			
			// coallesce right, target to first freeMemList, insert at head
			targethdr->size+=freeMemList->size+sizeof(struct memHeader);
			targethdr->prev=0;
			targethdr->next=freeMemList->next;
			freeMemList->next->prev=targethdr;
			freeMemList->sanityCheck=0x0;
			freeMemList=targethdr;
			freeMemList->prev=0x0;
			DBGMSG("inserted 0x%x at head, new size %x, coallesce right with old head \n",
				freeMemList, freeMemList->size);
		}else{
			//insert at head and old head on right
			targethdr->prev=0;
			targethdr->next=freeMemList;
			freeMemList->prev=targethdr;
			freeMemList=targethdr;
			freeMemList->prev=0x0;
			DBGMSG("inserted ox%x at head, linked to oldhead 0x%x\n", 
				freeMemList, freeMemList->next); 
		} 
	}else if ((lowerbound!=0)&&(upperbound==0)){
		// no memBlocks in free list above us 
		// memory block to be appended at end
		struct memHeader* hdr = freeMemList;
		while (hdr->next!=NULL)
			hdr=hdr->next;
		//we now have the last memHeader in freeMemlist
		if (((long int)target - ((long int)(hdr->dataStart) + 
			(long int)(hdr->size))) < (2*sizeof(struct memHeader))){	
				
			//coallesce right hdr, targethdr 
			hdr->size+=targethdr->size + sizeof(struct memHeader);
			targethdr->sanityCheck=0x0;
			DBGMSG("append by coallescing with 0x%x , new size=%x",
				hdr, hdr->size);
		}else{
			//link
			hdr->next=targethdr;
			targethdr->prev=hdr;
			targethdr->next=NULL;
			DBGMSG("append by linking in 0x%x\n", targethdr);
		}		
	}else{
		//we have a memory block somewhere in the middle
		kassert(upperbound!=0);
		kassert(lowerbound!=0);
		kassert((int)(((struct memHeader*)lowerbound)->next)==(int)upperbound);
		kassert((int)(((struct memHeader*)upperbound)->prev)==(int)lowerbound);
		link_or_coallesce(lowerbound, upperbound, target);
	}
}

// to show free list and allow manual verification of pointers, sizes, address boundaries
void printFreeList(void){
	//TRACE
	struct memHeader * hdr = freeMemList;
	int i = 0;
	int total = 0;

	while(hdr!=0) 
	{
		kprintf( "  %2d  address=%6x, size=%6x, prev=%6x, next=%6x, sanity=%6x\n",
			i++, hdr, hdr->size, hdr->prev, hdr->next, hdr->sanityCheck);
		kprintf( "\t\tDataStart Address = %x\n", hdr->dataStart);
		total += hdr->size;
		hdr=hdr->next;
	};
	kprintf("\ttotal size available across %d (0x%x)memory blocks\n",total,total,i);
}


// utility funtion to get memHeader of newly allocated user memory pointer
char* hdrFromUsrPtr(char* ptr){
	return (char*)((int)ptr - sizeof(struct memHeader));
}

