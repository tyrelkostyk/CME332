/*
CME332 Lab 2 Part 1 - Tuesday, Febuary 5th 2019
Tyrel Kostyk, tck290, 11216033
*/
#include <stdio.h>

/////* Define Global Constants and Mem-Mapped Pointers */////
#define interval_timer_ptr    (((volatile unsigned long *)0xFF202000))		// Mem Addr for Interval Timer
#define HEX3_HEX0_ptr         (((volatile unsigned long *)0xFF200020))		// Mem Addr for HEX3 - HEX0 Displays
#define	HEX7_HEX4_ptr         (((volatile unsigned long *)0xFF200030))		// Mem Addr for HEX7 - HEX4 Displays
#define	SW_ptr		          	(((volatile unsigned long *)0xFF200040))		// Mem Addr for SW17 - SW0
#define KEY_ptr			          (((volatile unsigned long *)0xFF200050))		// Mem Addr for KEY3-0 Pushbuttons

#define BYTE_SIZE             8             // Byte size, in bits (for offsetting HEX display inputs)
#define INT_TIMER_MAX_COUNT   0xFFFFFFFF    // Interval Timer max value (32-bit)
#define INT_TIMER_CONFIG      0x06          // Config for Int Timer; STOP = 0, START = 1, CONT = 1, ITO = 0

const char lut_num [11] =   {              // gfe.dcba
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
                                 0x40   // -  100.0000 (error)
};

int KEY2_flag = 0;  // 0 = display execution_time on HEX7-0
                    // 1 = display approx_sqrt_val on HEX7-0


/////* Conversions & Mathematical Function Definitions */////
int calc_pow(int base, int power) {
  /* simple function to compute exponent calculations */
  int result = 1;
  for (int i = 0; i < power; i++) {
    result *= base;
  }

  return result;
}

int DigitAtIndex(int index, int num) {
  /* returns single digit value at the corresponding index (0=1's place) */
  return (( num / calc_pow(10,index) ) % 10);
}

int intDigitToHex(int digit) {
  /* return the hex value equal to given digit; must be in range 0-9 */
  if (digit > 9) {
    return lut_num[10];    // digit value out of range; show '-' (error)
  }

  return lut_num[digit];
}


/////* HEX Display Function Definitions */////
void display_HEX7_HEX4(int hex7, int hex6, int hex5, int hex4) {
  /* Update HEX7-HEX4 7-Segment Hex Displays. All inputs are 0-9, in hex */
  *HEX7_HEX4_ptr = ((hex7 << BYTE_SIZE*3) + (hex6 << BYTE_SIZE*2)
      + (hex5 << BYTE_SIZE*1) + (hex4));

  return;
}

void display_HEX3_HEX0(int hex3, int hex2, int hex1, int hex0) {
  /* Update HEX3-HEX0 7-Segment Hex Displays. All inputs are 0-9, in hex */
  *HEX3_HEX0_ptr = ((hex3 << BYTE_SIZE*3) + (hex2 << BYTE_SIZE*2)
      + (hex1 << BYTE_SIZE*1) + (hex0));

  return;
}


/////* Interval Timer Function Definitions */////
void int_timer_load(int count) {
  /* (re)loads the interval timer count value to increment down from */
  *(interval_timer_ptr + 2) = (count & 0xFFFF);          // Load in the lower 16 bits of count val
  *(interval_timer_ptr + 3) = (count >> 16) & 0xFFFF;    // Load in the higher 16 bits of count val

  return;
}

void int_timer_run(int int_timer_config) {
  /* start interval timer based on 4-bit config settings; pattern: STOP.START.CONT.ITO */
  *(interval_timer_ptr + 1) = int_timer_config;

  return;
}

void int_timer_stop(void) {
  /* stop the interval timer; pattern: STOP.START.CONT.ITO */
  *(interval_timer_ptr + 1) = 0x8;

  return;
}

unsigned int get_int_timer_val(void) {
  /* Return the current value of the Interval Timer */
  *(interval_timer_ptr + 4) |= 0x00;  // write any val to Counter snapshot (low)

  // Add the low & high counter snapshots (both 16-bit wide)
  return ( (*(interval_timer_ptr + 5) << BYTE_SIZE*2) + *(interval_timer_ptr + 4) );
}


/////* Dummy Function Definition */////
#define BIGNUM (int *) 0x20000
int LIST[7] = {4, 5, 3, 6, 1, 8, 2};
void test_program(void) {
  int_timer_load(INT_TIMER_MAX_COUNT);  // reload timer with max count value
  int_timer_run(INT_TIMER_CONFIG);      // start timer! (no ints, don't loop)

  int big, i;
  big = LIST[0];
  for ( i = 1; i <= 6; i++) {
    if ( LIST[i] > big ) {
      big = LIST[i];
    }
  }
  *BIGNUM = big;

  int_timer_stop();     // stop the interval counter before returning
  return;
}


long int sqrt_approx(int val) {
  // Approximate the square root of signed integer input "val"
  int_timer_load(INT_TIMER_MAX_COUNT);  // reload timer with max count value
  int_timer_run(INT_TIMER_CONFIG);      // start timer! (no ints, don't loop)

  long int approx_sqrt_val = 0;

  for (int i = 14; i >= 0; i-- ) {
    if ( (val - (approx_sqrt_val * approx_sqrt_val)) == 0 ) {
      int_timer_stop();     // stop the interval counter before returning
      return approx_sqrt_val;

    } else if ( (val - (approx_sqrt_val * approx_sqrt_val)) > 0 ) {
      approx_sqrt_val += calc_pow(2, i);

    } else {
      approx_sqrt_val -= calc_pow(2, i);
    }
  }

  int_timer_stop();     // stop the interval counter before returning
  return approx_sqrt_val;
}


/////* MAIN LOOP */////
int main(void) {
  // clear all 8 HEX displays before operation
  *HEX3_HEX0_ptr &= ~0xFFFFFFFF;
  *HEX7_HEX4_ptr &= ~0xFFFFFFFF;

  unsigned int execution_time;  // time (in processing steps) to execute
  long int SW_val;
  long int approx_sqrt_val;

	while(1) {

    if ((*KEY_ptr) & 0x02) {            // KEY 1; Read new SW values
      while((*KEY_ptr) & 0x02);
      SW_val = *(SW_ptr);

      // run square root approximation function
      approx_sqrt_val = sqrt_approx(SW_val);
      // measure execution time
      execution_time = ( INT_TIMER_MAX_COUNT - get_int_timer_val() );

    }

    if ((*KEY_ptr) & 0x04) {            // KEY 2; toggle KEY2_flag for display
      while((*KEY_ptr) & 0x04);
      KEY2_flag = !KEY2_flag;
    }


    if (KEY2_flag) {
      // Display the 10,000's, 100,000's, 1,000,000's, and 10,000,000's digits of approx_sqrt_val on HEX7-HEX4
  		display_HEX7_HEX4(intDigitToHex(DigitAtIndex(7, approx_sqrt_val)),
        intDigitToHex(DigitAtIndex(6, approx_sqrt_val)),
        intDigitToHex(DigitAtIndex(5, approx_sqrt_val)),
        intDigitToHex(DigitAtIndex(4, approx_sqrt_val))
        );

  		// Display the 1's, 10's, 100's, and 1000's digits of approx_sqrt_val on HEX3-HEX0
  		display_HEX3_HEX0(intDigitToHex(DigitAtIndex(3, approx_sqrt_val)),
        intDigitToHex(DigitAtIndex(2, approx_sqrt_val)),
        intDigitToHex(DigitAtIndex(1, approx_sqrt_val)),
        intDigitToHex(DigitAtIndex(0, approx_sqrt_val))
        );
    } else {
      // Display the 10,000's, 100,000's, 1,000,000's, and 10,000,000's digits of execution_time on HEX7-HEX4
  		display_HEX7_HEX4(intDigitToHex(DigitAtIndex(7, execution_time)),
        intDigitToHex(DigitAtIndex(5, execution_time)),
        intDigitToHex(DigitAtIndex(6, execution_time)),
        intDigitToHex(DigitAtIndex(4, execution_time))
        );

  		// Display the 1's, 10's, 100's, and 1000's digits of execution_time on HEX3-HEX0
  		display_HEX3_HEX0(intDigitToHex(DigitAtIndex(3, execution_time)),
        intDigitToHex(DigitAtIndex(2, execution_time)),
        intDigitToHex(DigitAtIndex(1, execution_time)),
        intDigitToHex(DigitAtIndex(0, execution_time))
        );
    }

	}
}
