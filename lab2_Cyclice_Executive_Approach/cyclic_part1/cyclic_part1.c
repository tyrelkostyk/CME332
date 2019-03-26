/*
CME332 Lab 2 Part 2 - Tuesday, Febuary 5th 2019
Tyrel Kostyk, tck290, 11216033
*/

#include <rand.h>
#include "nios2_ctrl_reg_macros.h"  // for activating hardware interrupts


/////* Define Global Constants and Mem-Mapped Pointers */////
#define interval_timer_ptr    (((volatile unsigned long *)0xFF202000))		// Mem Addr for Interval Timer
#define HEX3_HEX0_ptr         (((volatile unsigned long *)0xFF200020))		// Mem Addr for HEX3 - HEX0 Displays
#define	HEX7_HEX4_ptr         (((volatile unsigned long *)0xFF200030))		// Mem Addr for HEX7 - HEX4 Displays
#define	SW_ptr		          	(((volatile unsigned long *)0xFF200040))		// Mem Addr for SW17 - SW0
#define KEY_ptr			          (((volatile unsigned long *)0xFF200050))		// Mem Addr for KEY3-0 Pushbuttons

#define BYTE_SIZE             8             // Byte size, in bits (for offsetting HEX display inputs)
#define INT_TIMER_MAX_COUNT   0xFFFFFFFF    // Interval Timer max value (32-bit)
#define INT_TIMER_PERIOD      0x00BEBC20    // (1 / 50MHz) x 12,500,000 = 0.25s
#define INT_TIMER_CONFIG      0x07          // Config for Int Timer; STOP = 0, START = 1, CONT = 1, ITO = 1

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



/////* PERIODIC TASKS */////
int game_task(void) {

  /* spuedo code: */

  // generate random number, display it on HEX3-0 (call display task)
  // use global (external) var to count down from 60, on every 4 interval_timer interrupt
    // Save timer val with get_int_timer_val(), then THAT minus subsequent get_int_timer_val
    // will tell us how long each round has been 
  // display_task to display that value on HEX7-6
  return 0;
}

void timing_task(void) {
  /* start interval timer based on 4-bit config settings; pattern: STOP.START.CONT.ITO */

  int_timer_run(INT_TIMER_CONFIG);

  return;
}

void display_task(int hex_choice, int input_1, int input_2, int input_3, int input_4 ) {

  // choice - 0 means HEX3-HEX0, 1 means HEX7-HEX4
  // use premade display_HEX7_HEX4 & display_HEX3_HEX0 functions

  return;
}


/////* MAIN LOOP */////
int main(void) {
  // clear all 8 HEX displays before operation
  *HEX3_HEX0_ptr &= ~0xFFFFFFFF;
  *HEX7_HEX4_ptr &= ~0xFFFFFFFF;

  // Load the interval timer
  int_timer_load(INTERVAL_TIMER_PERIOD);

  *(KEY_ptr + 2) = 0x3; // enable interrupts for all pushbuttons

  NIOS2_WRITE_IENABLE(0x3);
  NIOS2_WRITE_STATUS(1); // enable Nios II interrupts


  int round_number, score;

	while(1) {

    // would use hardware interrupts to check for KEY1 press, to reset game & score



    if (round_number >= 10) {
      // store final score & time elapsed
      // external global var + KEY3 interrupt controls which
      // reset game
    } else {
      score += game_task();
      round_number += 1;
    }






	}
}
