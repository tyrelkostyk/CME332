#include "address_map_nios2.h"

extern volatile int RUN_flag;
/*******************************************************************************
 * Pushbutton - Interrupt Service Routine
 *
 * This routine checks which KEY has been pressed and updates the global
 * variables as required.
 ******************************************************************************/
void pushbutton_ISR(void) {
    volatile int * KEY_ptr           = (int *)KEY_BASE;
    int            press;

    press          = *(KEY_ptr + 3); // read the pushbutton interrupt register
    *(KEY_ptr + 3) = press;          // Clear the interrupt


    RUN_flag = !RUN_flag; // Disable / Enable Stopwatch counting

    return;
}
