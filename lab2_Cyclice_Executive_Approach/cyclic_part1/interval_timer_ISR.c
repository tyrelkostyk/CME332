#include "address_map_nios2.h"
#include "globals.h" // defines global values

extern volatile int time_elapsed;
/*******************************************************************************
 * Interval timer interrupt service routine
 *
 * Counts every 0.25s for accurate time keeping of Game Program
 ******************************************************************************/
void interval_timer_ISR() {
    volatile int * interval_timer_ptr = (int *)TIMER_BASE;
    volatile int * LEDG_ptr           = (int *)LED_BASE; // LED address

    *(interval_timer_ptr) = 0; // clear the interrupt

    time_elapsed += 1;

    return;
}
