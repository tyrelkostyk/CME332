#include "address_map_nios2.h"
#include "globals.h" // defines global values

extern volatile int count;
/*******************************************************************************
 * Interval timer interrupt service routine
 ******************************************************************************/
void interval_timer_ISR() {

	volatile int * interval_timer_ptr = (int *)TIMER_BASE;
	
	*(interval_timer_ptr) = 0; // clear the interrupt

	if (count >= 9) {
		count = 0;
	} else {
		count += 1;
	}

	return;
}

