/* sleep.c : sleep device (assignment 2)
 */

#include <xeroskernel.h>
#include <sleep.h>

/* spin to allow sleep -- approx 1 second per cycle */
// useful to pause output to allow capture of a screen snapshot
void sleep(int cycles){
	int i;
	int j;
	for (i=0;i<cycles;i++){
		for(j=0;j<1000000;j++){
		}
	}
}
