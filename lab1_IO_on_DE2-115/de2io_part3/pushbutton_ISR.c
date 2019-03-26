#include "address_map_nios2.h"
#include "globals.h" // defines global values

extern volatile int count;
/*******************************************************************************
 * Pushbutton - Interrupt Service Routine
 ******************************************************************************/
void pushbutton_ISR(void) {

	volatile int * KEY_ptr           = (int *)KEY_BASE;

	int press = *(KEY_ptr + 3);
	*(KEY_ptr + 3) = press;		// Clear the interrupt

	count = 0;			// Clear the count

	return;

}
