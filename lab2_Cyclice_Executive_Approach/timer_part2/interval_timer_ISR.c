#include "address_map_nios2.h"
#include "globals.h" // defines global values

extern volatile int RUN_flag, time_SS, time_MM, time_QS;
/*******************************************************************************
 * Interval timer interrupt service routine
 *
 * Shifts a PATTERN being displayed on the LED lights. The shift direction
 * is determined by the external variable key_dir.
 ******************************************************************************/
void interval_timer_ISR() {
  volatile int * interval_timer_ptr = (int *)TIMER_BASE;

  *(interval_timer_ptr) = 0; // clear the interrupt

  time_QS++;
  if (RUN_flag) {
    if (time_QS >= 4) {
      time_QS = 0;

      if (time_SS >= 59) {
        time_SS = 0;

        if (time_MM >= 59) {
          time_MM = 0;
        } else {
          time_MM++;
        }
      } else {
        time_SS++;
      }
    }
  }

  return;
}
