/*
CME332 Lab 1 Part 3 - Tuesday, January 22nd 2019
Tyrel Kostyk, tck290, 11216033
*/

#include "address_map_nios2.h"
#include "globals.h" // defines global values
#include "nios2_ctrl_reg_macros.h"

#define HEX3_HEX0_ptr		(((volatile unsigned long *)0xFF200020))		// Mem Addr for HEX4 - HEX7 Displays
#define	HEX7_HEX4_ptr		(((volatile unsigned long *)0xFF200030))		// Mem Addr for HEX4 - HEX7 Displays

volatile int count = 0; 	// count from 0-9

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


int main(void) {
	volatile int * interval_timer_ptr = (int *)TIMER_BASE;          // interal timer base address
	volatile int * KEY_ptr = (int *)KEY_BASE; 			// pushbutton KEY address

	/* set the interval timer period for incrementing count */
	int counter                 = 50000000;                  // 1/(50 MHz) x (50000000) = 1 sec
	*(interval_timer_ptr + 0x2) = (counter & 0xFFFF);        // Load in the lower 16 bits of count val
	*(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;  // Load in the higher 16 bits of count val

	/* start interval timer, enable its interrupts */
	*(interval_timer_ptr + 1) = 0x7; // STOP = 0, START = 1, CONT = 1, ITO = 1

	*(KEY_ptr + 2) = 0xF; // enable interrupts for ALL pushbuttons

	/* set interrupt mask bits for level 0 (interval timer) and level 1 (pushbuttons) */
	NIOS2_WRITE_IENABLE(0x3);

	NIOS2_WRITE_STATUS(1); // enable Nios II interrupts

	while(1) {

		// Display count on HEX0
		display_HEX0(count);

	}

}
