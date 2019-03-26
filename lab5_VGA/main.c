#include <stdio.h>
#include "includes.h"

/* Definition of Global Vars and Mem-Mapped Pointers */

#define KEY_ptr					(((volatile unsigned long *)0xFF200050))		// Mem Addr for KEY3-0 Pushbuttons

#define KEY0					0x01
#define KEY1					0x02
#define KEY2					0x04
#define KEY3					0x08

#define DISP_BLUE 				0x187F

int KEY_val;
int KEY0_flag, KEY1_flag, KEY2_flag, KEY3_flag;


/* Definition of Semaphores & Mailboxes */

OS_EVENT	*KeySem;
INT8U 		err;

/* Definition of Event Flags */



OS_FLAGS value;


/* Definition of Task Stacks */

#define   TASK_STACKSIZE       		2048

OS_STK    TaskScanKey_stk[TASK_STACKSIZE];
OS_STK    TaskDrawBox_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */

#define TASKSCANKEY_PRIORITY        5
#define TASKDRAWBOX_PRIORITY        6


/* Supporting Functions */

//////* VGA DISPLAY FUNCTIONS *//////

void VGA_pixel(int x, int y, short pixel_color){
	int offset;
	volatile short * pixel_buffer = (short *) 0x08000000;	// VGA pixel buffer
	offset = (y << 9) + x;
	*(pixel_buffer + offset) = pixel_color;
}

void VGA_clear() {
	// Set entire VGA screen to black
	int x, y;

	for (x = 0; x < 160; x++) {
		for (y = 0; y < 240; y++) {
			VGA_pixel(x, y, 0);
		}
	}
}

void VGA_text(int x, int y, char * text_ptr) {
	// Subroutine to send a string of text to the VGA monitor

	int offset;
	volatile char *character_buffer = (char *) 0x09000000;   // VGA character buffer

	/* assume that the text string fits on one line */
	offset = (y << 7) + x;

	while ( *(text_ptr) ) {
		*(character_buffer + offset) = *(text_ptr);   // write to the character buffer
		++text_ptr;
		++offset;
	}
}

void VGA_box(int x1, int y1, int x2, int y2, short pixel_color) {
	// Draw a filled rectangle on the VGA monitor

	int offset, row, col;
	volatile short * pixel_buffer = (short *) 0x08000000;	// VGA pixel buffer

	/* assume that the box coordinates are valid */
	for (row = y1; row <= y2; row++) {
		col = x1;
		while (col <= x2) {
			offset = (row << 9) + col;
			*(pixel_buffer + offset) = pixel_color; // compute halfword address, set pixel
			++col;
		}
	}
}


/* Definition of Tasks */

void TaskScanKey(void* pdata) {

	VGA_clear();

	while(1) {

		// Protect Key Inputs with KeySem
		OSSemPend(KeySem, 0, &err);

		// Read Key Values
		KEY_val = *(KEY_ptr) & 0xF;
		if (KEY_val == KEY0) {				        // KEY0 - right
			KEY0_flag = 1;
		} else if (KEY_val == KEY1) {				// KEY1 - down
			KEY1_flag = 1;
		} else if (KEY_val == KEY2) {				// KEY2 - up
			KEY2_flag = 1;
		} else if (KEY_val == KEY3) {
			KEY3_flag = 1;
		}

		OSSemPost(KeySem);

		OSTimeDly(1);
	}
}

void TaskDrawBox(void *pdata) {
	// Task that draws the box

	VGA_clear();

	// starting values for 20x20 box in center of screen
	int x1 = 150;
	int y1 = 110;
	int x2 = 170;
	int y2 = 130;

	VGA_box (x1, y1, x2, y2, DISP_BLUE);

	while(1) {

		OSSemPend(KeySem, 0, &err);

		if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
			// move box to the right
			KEY0_flag = 0;
			VGA_box (x1, y1, x2, y2, 0);	// remove current box
			x1 += 5;
			x2 += 5;
			VGA_box (x1, y1, x2, y2, DISP_BLUE);
		} else if ( !(KEY_val & KEY1) && (KEY1_flag) ) {
			// move box down
			KEY1_flag = 0;
			VGA_box (x1, y1, x2, y2, 0);	// remove current box
			y1 += 5;
			y2 += 5;
			VGA_box (x1, y1, x2, y2, DISP_BLUE);
		} else if ( !(KEY_val & KEY2) && (KEY2_flag) ) {
			// move box up
			KEY2_flag = 0;
			VGA_box (x1, y1, x2, y2, 0);	// remove current box
			y1 -= 5;
			y2 -= 5;
			VGA_box (x1, y1, x2, y2, DISP_BLUE);
		} else if ( !(KEY_val & KEY3) && (KEY3_flag) ) {
			// move box to the left
			KEY3_flag = 0;
			VGA_box (x1, y1, x2, y2, 0);	// remove current box
			x1 -= 5;
			x2 -= 5;
			VGA_box (x1, y1, x2, y2, DISP_BLUE);
		}

		OSSemPost(KeySem);

		OSTimeDly(1);
	}
}


/* The main function creates the tasks and starts the program */
int main(void)
{

  OSTaskCreateExt(TaskScanKey,
                  NULL,
                  (void *)&TaskScanKey_stk[TASK_STACKSIZE-1],
				  TASKSCANKEY_PRIORITY,
				  TASKSCANKEY_PRIORITY,
				  TaskScanKey_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(TaskDrawBox,
                  NULL,
                  (void *)&TaskDrawBox_stk[TASK_STACKSIZE-1],
				  TASKDRAWBOX_PRIORITY,
				  TASKDRAWBOX_PRIORITY,
				  TaskDrawBox_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  VGA_clear();

  KeySem = OSSemCreate(1);

  OSStart();

  return 0;
}
