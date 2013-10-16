#include <mytests.h>
#include <mem.h>  
#include <utility.h>
#include <sleep.h>
#include <mytests.h>
#include <i386.h>  //HOLEstuff, freemem maxaddr
#include <xeroslib.h> //srand rand

/*
 * test variations on kmalloc and kfree
 *  also includes use of a random generator for allocating and freeing 
 * continously
 * 
* memHeaderTestOK does sanity check on the memHeader structure and 
* is used extensiv;y to ensure the memor4y block metadata is sane.
* We wnat to know as soon as possible if its not, so that we can
* back trace to source of corruption.
* 
 * */



// basic sanity check is automated by memHeaderOK and freememListOK
// 	failure on sanity conditions will halt system.
//  manual inspection is only required to verify values, not consistency or sanity	
void testkmalloc(void){
	TRACE
	//test allocating a single block
	int request = 1;
	char* abyte = (char*)kmalloc (request);
	showAllocatedHeader(abyte);
	memHeaderOK((struct memHeader*) (abyte-sizeof(struct memHeader)));
	kprintf("*******************TESTKMALLOC***************\n");
	kprintf("1. kmalloc(0x%x) returned address %x\n", request, abyte );
	kassert(((struct memHeader*)hdrFromUsrPtr(abyte))->size=16);
	printFreeList();
	freeMemListOK();
	sleep (10);
	// size of new smaller free block is correct
	//		32bytes for user data and 16 bytes for extra header = 48 bytes (0x30 size reduction)
	
	// test allocating a second block
	request+=1;
	abyte = (char*)kmalloc (request);
	showAllocatedHeader(abyte);
	memHeaderOK((struct memHeader*) (abyte-sizeof(struct memHeader)));
	kprintf("2. kmalloc(0x%x) returned address %x\n", request, abyte );
	kassert(((struct memHeader*)hdrFromUsrPtr(abyte))->size=16);
	printFreeList();
	freeMemListOK();
	sleep(5);
	
	// test allocating a single block size = pagesize
	request=16;
	abyte = (char*)kmalloc (request);
	showAllocatedHeader(abyte);
	kassert(((struct memHeader*)hdrFromUsrPtr(abyte))->size=16);
	memHeaderOK((struct memHeader*) (abyte-sizeof(struct memHeader)));
	kprintf("3. kmalloc(0x%x) returned address %x\n", request, abyte );
	printFreeList();
	freeMemListOK();
	sleep(5);
	
	// ask for size larger than in first memory block
	request=0x98650;  
	abyte = (char*)kmalloc (request);
	showAllocatedHeader(abyte);
	kassert(((struct memHeader*)hdrFromUsrPtr(abyte))->size>=request);
	memHeaderOK((struct memHeader*) (abyte-sizeof(struct memHeader)));
	kprintf("4. kmalloc(0x%x) returned address %x\n", request, abyte );
	printFreeList();
	freeMemListOK();
	sleep(5);
	
	// ask for size larger than largest block
	request = 0x1d20000;
	abyte = (char*)kmalloc (request);
	kassert(abyte==0);
	kprintf("5. kmalloc(0x%x) returned address %x\n", request, abyte );
	printFreeList();
	freeMemListOK();
	sleep(5);
	
	
	// ask for size just smaller than full memory block (leave less than 32 bytes)
	request = 0x980b1;
	abyte = (char*)kmalloc (request);
	showAllocatedHeader(abyte);
	kassert(((struct memHeader*)hdrFromUsrPtr(abyte))->size>request);
	memHeaderOK((struct memHeader*) (abyte-sizeof(struct memHeader)));
	kprintf("6. kmalloc(0x%x) returned address %x\n", request, abyte );
	printFreeList();
	freeMemListOK();
	sleep(5);
}


/* exercise all the branches of kfree.  This requires set up of memory
*	patterns to trigger the different branches to link/coallesce next/prev
* 	memory blocks.   Tests are labelled with the memory block setup with
* 	letters representing allocated memory blocks and F representing Free block.
* 
* 	eg. ABFDE   represents two allocated blocks, a free block and two more allocated
* 		we then can free block labeled D to test a left allocation, right link
* 		of a kfree memory block
**/
 
void testkfree(void){
	int i =0;
	int testno=0;
	char* addresses[16] ; 
	struct memHeader* ahdr;
	
// starting with ABCDE all allocated, test sequence will be
//    
//	free(A), which tested return first memblock, 
//  free(B), return 2nd memblock - coallesce left 
//  free(d), return a block that requires linking left and right
//  free(c), return a block that requires coallescing left and right
//  free(e), return a block that requires coallescing left and right  to restore all free

	
	kprintf("TEST %d ===================\n",++testno);
	kprintf("kfree test freememABCDE  free A----------------\n");
	kprintf("   --setting up for test 1\n");
	for (i =0; i<5 ;i++) {
		addresses[i] = (char*)kmalloc(1);
		memHeaderOK((struct memHeader*)hdrFromUsrPtr(addresses[i]));
		freeMemListOK();
	}
	kprintf("TEST %d === Allocated User Memory==============\n", testno); 
	for (i = 0; i< 5; i++){
		ahdr = (struct memHeader*) hdrFromUsrPtr(addresses[i]);
		kprintf("\t%d, user address=0x%x, hdr addres=0x%x, size=0x%x, sanity=0x%x, prev=%x, next=%x\n",
			i,addresses[i],ahdr,ahdr->size,ahdr->sanityCheck,ahdr->prev, ahdr->next );
	}
	kprintf("TEST %d --freelist before test\n", testno);
	printFreeList();
	sleep(5);
	
	kprintf("TEST %d  --freeing first memHeader\n", testno);	
	for (i =0; i<5 ;i++) memHeaderOK((struct memHeader*)hdrFromUsrPtr(addresses[i]));
	kprintf("TEST %d    -- before test\n", testno);
	showAllocatedHeader(addresses[0]);
	printFreeList();
	sleep(5);
	
	kfree(addresses[0]);
	
	for (i =0; i<5 ;i++) memHeaderOK((struct memHeader*)hdrFromUsrPtr(addresses[i]));
	kprintf(" TEST %d  --after test\n", testno);
	printFreeList();
	freeMemListOK();
	sleep(5);

	//free pattern   freememFBCDE  free B
	kprintf("TEST %d ===================\n",++testno);
	kprintf("TEST %d     -- freelist inherited from last test\n", testno);
	kprintf("TEST %d    --freeing B freememFBCDE  free B - coallesce left\n", testno);	
	memHeaderOK((struct memHeader*)hdrFromUsrPtr(addresses[1]));
	kprintf("TEST %d    -- before test\n", testno);
	showAllocatedHeader(addresses[1]);
	printFreeList();
	sleep (10);
	
	kfree(addresses[1]);
	
	//memHeaderOK((struct memHeader*)hdrFromUsrPtr(addresses[1]));
	// should no longer be valid header coallese into A,
	kprintf("TEST %d   -- after test\n", testno); 
	printFreeList();
	freeMemListOK();
	kprintf("TEST %d    --result should have first memblock at head of freelist\n",
		testno);
	sleep(5);

	//free pattern   freememFFCDE  free D
	kprintf("TEST %d ===================\n",++testno);
	kprintf("TEST %d --freeing D freememFFCDE - link in middle \n", testno);	
	memHeaderOK((struct memHeader*)hdrFromUsrPtr(addresses[3]));
	kprintf("TEST %d -- before test\n", testno);
	showAllocatedHeader(addresses[3]);
	sleep(5);
	
	kfree(addresses[3]);
	kprintf("TEST %d -- after test\n", testno);
	memHeaderOK((struct memHeader*)hdrFromUsrPtr(addresses[3]));
	printFreeList();
	freeMemListOK();
	kprintf("TEST %d --result should have first memblock at head of freelist\n", testno);
	sleep(5);

	//free pattern   freememFFCFE  free C
	kprintf("TEST %d ===================\n",++testno);
	kprintf(" TEST %d --freeing C freememFFCFE - coallesce (prev targ next)\n",
		testno);	
	memHeaderOK((struct memHeader*)hdrFromUsrPtr(addresses[2]));
	kprintf(" TEST %d   -- before test\n", testno);
	showAllocatedHeader(addresses[2]);
	printFreeList();
	sleep(5);
	
	kfree(addresses[2]);
	//memHeaderOK((struct memHeader*)hdrFromUsrPtr(addresses[2]));
	//coallesce no longer valid header
	kprintf("TEST %d -- after test\n", testno);
	printFreeList();
	freeMemListOK();
	kprintf("TEST %d   --result should have first memblock at head of freelist\n",
		testno);
	sleep(5);
	
	//free pattern   freememFFFFE  free E
	kprintf("TEST %d ===================\n",++testno);
	kprintf("TEST %d    --freeing E freememFFFFE - coallesce (prev, target, next) n",
		testno);	
	kprintf("TEST %d    -- before test\n", testno);
	showAllocatedHeader(addresses[4]);
	printFreeList();
	sleep(5);
	kfree(addresses[4]);
	
	kprintf("TEST %d  --after test\n", testno);
	printFreeList();
	freeMemListOK();
	kprintf("    --result should have first memblock at head of freelist\n");
	sleep(5);
	
// completed batch one of free tests

	//all of addreses are already free, 
	kprintf("TEST %d ===================\n",++testno);
	kprintf("TEST %d     -- free addresses that are already free\n", testno);
	kprintf("TEST %d    -- before test\n",testno);
	printFreeList();
	sleep(5);	
	for(i=0;i<5;i++){
		kprintf("%d %x ", i, addresses[i]);
		 kfree(addresses[i]);
		freeMemListOK();
	}
	kprintf("TEST %d    -- after test\n", testno);
	printFreeList();
	sleep(5);
	
	
	
	//free pattern   freememAFCDE  free A
	kprintf("TEST %d ===================\n",++testno);
	kprintf("TEST %d    -- kfree test freememAFCDE  free A---link in returning first memory block-------------\n",
		testno);
	kprintf("TEST %d    --setting up for test2\n", testno);
	freeMemListOK();
	for(i=0;i<5;i++) {
		addresses[i] = (char*)kmalloc(1);
		freeMemListOK();
		memHeaderOK( (struct memHeader*)hdrFromUsrPtr(addresses[i]));
	}
	kfree(addresses[1]); 
	kprintf("TEST %d    -- before test\n", testno);
	showAllocatedHeader(addresses[1]);
	printFreeList();
	sleep(5);
	kfree(addresses[0]); 
	
	kprintf("TEST %d    -- after test\n", testno);
	printFreeList();
	freeMemListOK();
	sleep (10);
	// cleanup - release rest
	kfree(addresses[2]);
	kfree(addresses[3]);
	kfree(addresses[4]);

	
	
	kprintf("=== setting up for next test: Allocated User Memory\n"); 
	for (i = 0; i< 5; i++){
		addresses[i]=(char*)kmalloc(1);
		ahdr = (struct memHeader*) hdrFromUsrPtr(addresses[i]);
		//kprintf("\t%d, user address=0x%x, hdr addres=0x%x, size=0x%x, sanity=0x%x\n",
			//i,addresses[i],ahdr,ahdr->size,ahdr->sanityCheck );
	}
	
	kprintf("   --second address should be part of free list (not adj to freemem)\n");
	kfree(addresses[1]);
	
	kprintf("TEST %d ===================\n",++testno);
	kprintf("TEST %d    --test freeing A - freememAFCDE - coallese right, inert head freememlist\n", testno);
	kprintf("TEST %d    -- before test\n", testno);
	showAllocatedHeader(addresses[0]);
	printFreeList();
	sleep(5);
	kfree(addresses[0]);
	
	kprintf("TEST %d   -- after test\n", testno);
	printFreeList();
	kprintf(" TEST %d   --result should have first memblock at head of freelist, size 30\n", testno);
	sleep(5);
	//cleanup
	kfree(addresses[2]);
	kfree(addresses[3]);
	kfree(addresses[4]);
	 
	////free pattern   AFCFE   free C for 3way coallesc
	kprintf("TEST %d ===================\n",++testno);
	kprintf("TEST %d    --kfree test  AFCFE   free b for 3way coallesc----------------\n",
		testno);
	kprintf("TEST %d    --setting up for test 3\n", testno);
	for(i=0;i<5;i++) addresses[i] = (char*)kmalloc(1);
	kfree(addresses[1]);
	kfree(addresses[3]);
	kprintf("TEST %d    --should now have  AFCFE \n", testno);
	kprintf("TEST %d    -- Allocated User Memory\n", testno); 
	for (i = 0; i< 5; i++){
		ahdr = (struct memHeader*) hdrFromUsrPtr(addresses[i]);
		kprintf("\t%d, user address=0x%x, hdr addres=0x%x, size=0x%x, sanity=0x%x\n",
			i,addresses[i],ahdr,ahdr->size,ahdr->sanityCheck );
	}
	
	kprintf("TEST %d   --test: freeing C in AFCFE   : coalles prev and next\n", testno);
	kprintf("TEST %d   -- before test\n", testno);
	showAllocatedHeader(addresses[2]);
	printFreeList();
	sleep(5);
	
	kfree(addresses[2]);
	kprintf("TEST %d  -- after test\n", testno);
	kprintf("TEST %d    -- verify correct updating of Allocated User Memory blocks\n", testno); 
	for (i = 0; i< 5; i++){
		ahdr = (struct memHeader*) hdrFromUsrPtr(addresses[i]);
		kprintf("\t%d, user address=0x%x, hdr addres=0x%x, size=0x%x, sanity=0x%x\n",
			i,addresses[i],ahdr,ahdr->size,ahdr->sanityCheck );
	}
	printFreeList();
	kprintf("TEST %d    --result should have first memblock at head of freelist, not adjacent to freemem, size 50\n", testno);
	sleep(5);
	// cleanup
	kfree(addresses[0]);
	kfree(addresses[4]);
	
	//free pattern   AFBC   free b for left coallesc
	kprintf("TEST %d ===================\n",++testno);
	kprintf(" TEST %d   --kfree test AFBC   free b for left coallesc----------------\n",
		testno);
	for(i=0;i<5;i++) {
		addresses[i] = (char*)kmalloc(1);
		freeMemListOK();
		memHeaderOK( (struct memHeader*)hdrFromUsrPtr(addresses[i]));
	}
	showAllocatedHeader(addresses[1]);
	kfree(addresses[1]);
	kprintf("TEST %d     -- test setup : Allocated User Memory\n",
		testno); 
	for (i = 0; i< 5; i++){
		ahdr = (struct memHeader*) hdrFromUsrPtr(addresses[i]);
		kprintf("\t%d, user address=0x%x, hdr addres=0x%x, size=0x%x, sanity=0x%x, prev=%x, next= %x\n",
			i,addresses[i],ahdr,ahdr->size,ahdr->sanityCheck, ahdr->prev, ahdr->next );
	}
	kprintf(" TEST %d  --before test\n", testno);
	printFreeList();
	kprintf(" TEST %d   -- test AFBC, freeing B - coallesc left\n", testno);
	showAllocatedHeader(addresses[2]);
	sleep(5);
	kfree(addresses[2]);
	kprintf(" TEST %d   -- after test\n", testno);
	printFreeList();
	freeMemListOK();
	sleep(5);
	
	//free pattern   ABFC   free b for right coallesc
	kprintf("TEST %d ===================\n",++testno);
	kprintf("   -- kfree test ABFC   free b for right coallesc----------------\n");
	for(i=0;i<5;i++) {
		addresses[i] = (char*)kmalloc(1);
		freeMemListOK();
		memHeaderOK( (struct memHeader*)hdrFromUsrPtr(addresses[i]));
	}
	kfree(addresses[2]);
	
	kprintf("   --before test\n");
	printFreeList();
	showAllocatedHeader(addresses[1]);
	sleep(5);
	kfree(addresses[1]);
	kprintf("   --after test\n");
	printFreeList();
	freeMemListOK();
	sleep(5);
	
	//free pattern   AFCDEF free d for all linking
	kprintf("TEST %d ===================\n",++testno);
	kprintf("  -- kfree test AFCDEF free d for all linking----------------\n");
	for(i=0;i<6;i++) {
		addresses[i] = (char*)kmalloc(1);
		freeMemListOK();
		memHeaderOK( (struct memHeader*)hdrFromUsrPtr(addresses[i]));
	}
	kfree(addresses[1]);
	kfree(addresses[5]);
	
	kprintf("   -- before test\n");
	showAllocatedHeader(addresses[3]);
	printFreeList();
	sleep(5);
	kfree(addresses[3]);
	kprintf("   -- after test\n");
	printFreeList();
	freeMemListOK();
	sleep(5);	
	
	kprintf("TEST %d ===================\n",++testno);
	kprintf("   allocate 1000 memory blocks, deallocate 1000\n");
	kprintf("   also run with variants of size, and dealloc every mod 2");
	kprintf("   mod 3, mod 5");
	allocXfreeModY(1000, 3);
	
}


// allocate x memory blocks, then start freeing every mod y of them
// in case we dont see issues because we are too even or multiples of
// even
void allocXfreeModY(int x, int y){
	
	int i;
	struct memHeader* ahdr;
	char* kaddresses[MAXADDR];
	for (i=1; i< x; i++){
		kaddresses[i] = (char*)kmalloc(i*2);
		freeMemListOK();
		memHeaderOK( (struct memHeader*)hdrFromUsrPtr(kaddresses[i]));
		ahdr = (struct memHeader*) hdrFromUsrPtr(kaddresses[i]);
		kprintf("\t%d, user address=0x%x, hdr addres=0x%x, size=0x%x, sanity=0x%x, prev=%x, next= %x\n",
			i,kaddresses[i],ahdr,ahdr->size,ahdr->sanityCheck, ahdr->prev, ahdr->next );
	}
	
	for (i=1; i<x;i++){
		if (i%y)
			kfree(kaddresses[i]);
		freeMemListOK();
	}
	printFreeList();
	sleep(5);

	
}

// ultimate test by randomly allocating and deallocating.
// startup with a min number of allocated blocks and allow
// to accumulate up to about 2000 allocated blocks tnen
// free them back down to smaller size to keep from hitting
// no free memory to allocate (explictly tested seperately)

void randomMemoryTest(int seed){
	char* addresses[2048];
	int request;
	int i=0;
	srand(seed);
	request = rand();
	
	//allocate some random memory
	for (i=0;i<300;i++){
		kprintf("i%d kmalloc(%x)\n",i,request);
		addresses[i] = (char*)kmalloc(request);
		if (addresses[i]==0) break;  // ran out of memory
		freeMemListOK();
		memHeaderOK( (struct memHeader*)hdrFromUsrPtr(addresses[i]));
	}
	
	// now equal chance of alloc or free
	for (;;){
		if (rand()%2){
			request=rand();
			kprintf("kmalloc(%x)\n",request);
			addresses[i] = kmalloc(request);
			freeMemListOK();
			//memory allocation failed, nothing to check.
			if (addresses[i] !=0 )
				memHeaderOK( (struct memHeader*)hdrFromUsrPtr(addresses[i++]));
		}else{
			// we may free free addresses which kfree should handle correctly
			// but we may try to free a coallesced mem block which will not
			// have sanity, so wipe out addresses that are freed
			int index = rand()%i;
			kprintf("kfree(%x)\n", addresses[index]);
			// do not try to free empty slots, already freed previously
			// they may have been coallesced and no longer valid memory headers
			if (addresses[index]!=0) kfree(addresses[index]);
			addresses[index]=0x0;
			freeMemListOK();
		}
		// array of addresses getting too big, let a bunch go
		if (i>=2040){
			i--;
			while (i>300){
				 if (addresses[i]!=0) {
					kfree(addresses[i]);
					addresses[i]=0x0;
				}
				i--;	  
			 }
			 
		}
		freeMemListOK();
		
	}
	
	
}
	

//consistency check for either free or allocated memory block
	//are we on a page boundary
	//are we outside of HOLE
	//are we larger than freemem
	//are we less than maxaddr
	//do we have a correct sanityCheck for memHeader
	//is our size > 0 - we dont allocate headers for zero memory space
	//if we point to another memHeader, does it point back to us (next/prev)
	//if we point to next, is our address+size less than next
int memHeaderOK(struct memHeader* hdr){
	struct memHeader* listHead;
	listHead = hdr;
	
	char* ptr = (char*)hdr;
	kassert ((((unsigned long int)ptr)&0xf) == 0);
	kassert ( ((char*)ptr<(char*)HOLESTART)||((char*)ptr>=(char*)HOLEEND) );
	kassert ( (int)ptr >= (int)freemem );
	kassert ( ((int)hdr->dataStart + hdr->size -1) <= (int)maxaddr );
	
	kassert(hdr->sanityCheck==(char*)SANITY);
	
	//get the head of the list that this memory block is in
	while (listHead->prev){
		kassert(listHead->prev < listHead);
		listHead=listHead->prev;
	}
	
	// we do not allow zero size memory block allocations
	kassert(hdr->size > 0);
	
	// if we point to another memory block, they need to point back to us
	if(hdr->prev)
		kassert(hdr->prev->next == hdr);
	if(hdr->next){
		kassert(hdr->next->prev == hdr);
		kassert((int)hdr->next > ((int)hdr->dataStart + hdr->size -1));
	}
	//if this is a free memory block
	if (hdr->prev) kassert(hdr > hdr->prev);
	if (hdr->next) kassert (hdr->next > hdr);

	if (listHead!=freeMemList){

		//this is a memory block allocated to user space and should not point to 
		// any other memory blocks
		kassert(hdr->next == NULL);
		kassert(hdr->prev == NULL);
	}
	return 1;
}

// make sure all the headers in freeMemList are sane and consitent
int freeMemListOK(){
	struct memHeader* lasthdr=NULL;
	struct memHeader* hdr = freeMemList;
	
	//are we at the head?
	kassert(freeMemList->prev == 0x0);
	
	// walk down freeMemList and check each memory block
	while (hdr->next){
		memHeaderOK(hdr);
		hdr=hdr->next;
		//put the tail of freememlist into lasthdr
		if ((hdr)&&(hdr->next==NULL)&&(lasthdr==NULL))
			lasthdr=hdr;
	}

	// walk back up the freeMemList
	while (lasthdr->prev!=NULL){
		//memHeaderOK(lasthdr);
		lasthdr=lasthdr->prev;
	}

	// down and back the freeMemList, we should be back at start
	kassert(lasthdr==freeMemList);
	
	return 1;
}

void showAllocatedHeader(char* ptr){
	struct memHeader *hdr = (struct memHeader*)hdrFromUsrPtr(ptr);
	kprintf("%x  sz=%x prev=%x next=%x sanity=%x\n",
		(char*)hdr, hdr->size, (char*)hdr->prev, (char*)hdr->next, hdr->sanityCheck);
}

