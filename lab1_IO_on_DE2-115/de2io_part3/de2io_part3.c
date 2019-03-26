/*
CME332 Lab 1 Part 3 - Tuesday, January 22nd 2019
Tyrel Kostyk, tck290, 11216033
*/


// Declarations
#include "address_map_nios2.h"
#include "nios2_ctrl_reg_macros.h"
#include "globals.h" // defines global values

#define HEX3_HEX0_ptr		(((volatile unsigned long *)0xFF200020))		// Mem Addr for HEX4 - HEX7 Displays
#define	HEX7_HEX4_ptr		(((volatile unsigned long *)0xFF200030))		// Mem Addr for HEX4 - HEX7 Displays
#define KEY_ptr			(((volatile unsigned long *)0xFF200050))		// Mem Addr for KEY3-0 Pushbuttons
#define interval_timer_ptr	(((volatile unsigned long *)0xFF202000))		// Mem Addr for DE2-115's Interval Timer


// Main program

const char lut_num [11] =   {
                                 0x3F,  // 0  011.1111
                                 0x06,  // 1  000.0110
                                 0x5B,  // 2  101.1011
                                 0x4F,  // 3  100.1111
                                 0x66,  // 4  110.0110
                                 0x6D,  // 5  110.1101
                                 0x7D,  // 6  111.1101
                                 0x07,  // 7  000.0111
                                 0x7F,  // 8  111.1111
                                 0x6F,  // 9  110.1111
                                 0x04   // -  000.0100 (error)

};


void display_HEX0 (int four_bit_val) {
/* Display 4bit number (in decimal; 0-9) onto the HEX0 7-Segment Display */

	// clear HEX0
	*HEX3_HEX0_ptr &= ~0xFF;	

	switch (four_bit_val) {
	case (0): 
		*HEX3_HEX0_ptr |= lut_num[0];
		break;
	case (1): 
		*HEX3_HEX0_ptr |= lut_num[1];
		break;
	case (2): 
		*HEX3_HEX0_ptr |= lut_num[2];
		break;
	case (3): 
		*HEX3_HEX0_ptr |= lut_num[3];
		break;
	case (4): 
		*HEX3_HEX0_ptr |= lut_num[4];
		break;
	case (5): 
		*HEX3_HEX0_ptr |= lut_num[5];
		break;
	case (6):
		*HEX3_HEX0_ptr |= lut_num[6];
		break;
	case (7): 
		*HEX3_HEX0_ptr |= lut_num[7];
		break;
	case (8): 
		*HEX3_HEX0_ptr |= lut_num[8];
		break;
	case (9): 
		*HEX3_HEX0_ptr |= lut_num[9];
		break;
	default: 
		*HEX3_HEX0_ptr |= lut_num[10];
		break;
	}

	return;
}


int KEY_poll_and_count(int count) {

	if (((*KEY_ptr) & 0x02)) {			// KEY1; Increment Count by 1
		while(((*KEY_ptr) & 0x02));
		if (count == 9) {
			count = 0;
		} else {
			count += 1;
		}

	} else if (((*KEY_ptr) & 0x04)) {		// KEY2; Decrement Count by 1
		while(((*KEY_ptr) & 0x04));
		if (count == 0) {
			count = 9;
		} else {
			count -= 1;
		}

	} else if (((*KEY_ptr) & 0x08)) {		// KEY3; Clear count to 0
		while(((*KEY_ptr) & 0x08));
		count = 0;

	} else {				// Neither KEY3, KEY2, or KEY1 is pressed; return count unchanged
		count = count;
	}

	return count;

}



int main (void) {

	// clear all HEX displays before operation
	*HEX3_HEX0_ptr &= ~0xFFFFFFFF;	
	*HEX7_HEX4_ptr &= ~0xFFFFFFFF;	

	/* set the interval timer period */
	int counter = 50000000;    // (50,000,000 / 50MHz) = 1 Second delay 
    	*(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
    	*(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;

	/* start interval timer, enable its interrupts */
	*(interval_timer_ptr + 1) = 0x7; // STOP = 0, START = 1, CONT = 1, ITO = 1
	*(KEY_ptr + 2) = 0x3; // enable interrupts for all pushbuttons

    	/* set interrupt mask bits for levels 0 (interval timer) and 1 (pushbuttons) */
	NIOS2_WRITE_IENABLE(0x3);

	NIOS2_WRITE_STATUS(1); // enable Nios II interrupts

	while(1) {


		// Read the KEY pushbutton values, and increment/decrement/clear accordingly
		//count = KEY_poll_and_count(count);
		
		// Display count on HEX0
		//display_HEX0(count);
		
	}
	
}













