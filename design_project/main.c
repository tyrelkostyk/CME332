#include <stdio.h>
#include <stdlib.h>
#include "includes.h"

/* Definition of Global Vars and Mem-Mapped Pointers */

#define KEY_ptr								(((volatile unsigned long *)0xFF200050))		// Mem Addr for KEY3-0 Pushbuttons
#define BYTE_SIZE							8             // Byte size, in bits (for offsetting HEX display inputs)
#define LCD_SIZE							40

#define KEY0									0x01
#define KEY1									0x02
#define KEY2									0x04
#define KEY3									0x08

#define MAX_LOCATIONS 				100
#define VGA_TEXT_MAX_SIZE			200

#define MSG_BASE_X						1
#define MSG_BASE_Y						5

#define QUESTION_BASE_X				1
#define QUESTION_BASE_Y				20

// TODO: Update values
#define NORTH_MSG_BASE_X			230
#define NORTH_MSG_BASE_Y			140

#define EAST_MSG_BASE_X				230
#define EAST_MSG_BASE_Y				140

#define SOUTH_MSG_BASE_X			230
#define SOUTH_MSG_BASE_Y			140

#define WEST_MSG_BASE_X				70
#define WEST_MSG_BASE_Y				140

int KEY_val, SW_val;
int KEY0_flag, KEY1_flag, KEY2_flag, KEY3_flag;

int location, step_count;

int time_250ms;
int step_time_rem_SS = 30;
int tot_time_rem_SS, tot_time_rem_MM;
int tot_time_SS, tot_time_MM;


/* Definitions of LUTs for unique properties of each labyrinth location */

// lut for what buttons can be pressed (and do something) for each location
const char LUT_location_permissions [MAX_LOCATIONS] = {
  KEY0,                 		// Loc 0: Press KEY0 to Start (East)
  KEY0,                 		// Loc 1: East
  KEY0,                 		// Loc 2: East
  KEY0,                 		// Loc 3: East
  KEY0,                 		// Loc 4: East
  KEY0,                 		// Loc 5: East
  KEY0+KEY1,            		// Loc 6: East, South
	KEY0+KEY3,								// Loc 7: East, West
	KEY1+KEY3,								// Loc 8: West, South
	0,												// Loc 9: Not a valid spot
	KEY1,											// Loc 10: South
	KEY0+KEY1,								// Loc 11: East, South
	KEY1+KEY3,								// Loc 12: South, West
	KEY0+KEY1,								// Loc 13: East, South
	KEY1+KEY3,								// Loc 14: South, West
	KEY1+KEY2,								// Loc 15: North, South
	0,												// Loc 16: Not a valid spot
	KEY1+KEY2,								// Loc 17: North, South
	KEY0+KEY1,								// Loc 18: East, South
	KEY0+KEY1+KEY2+KEY3,			// Loc 19: All directions
	KEY2+KEY3,								// Loc 20: North, West
	KEY0+KEY2,								// Loc 21: North, East
	KEY2+KEY3,								// Loc 22: North, West
	KEY0+KEY2,								// Loc 23: North, East
	KEY0+KEY1+KEY2+KEY3,			// Loc 24: All directions
	KEY0+KEY3,								// Loc 25: East, West
	KEY2+KEY3,								// Loc 26: North, West
	KEY1+KEY2,								// Loc 27: North, South
	KEY0+KEY2,								// Loc 28: North, East
	KEY0+KEY3,								// Loc 29: East, West
	KEY0+KEY3,								// Loc 30: East, West
	KEY0+KEY3,								// Loc 31: East, West
	KEY3,											// Loc 32: West
	KEY1+KEY2,								// Loc 33: North, South
	0,												// Loc 34: Not a valid spot
	KEY1,											// Loc 35: South
	KEY1+KEY2,								// Loc 36: North, South
	KEY0+KEY1,								// Loc 37: East, South
	KEY0+KEY3,								// Loc 38: East, West
	KEY0+KEY3,								// Loc 39: East, West
	KEY0+KEY3,								// Loc 40: East, West
	KEY0,											// Loc 41: East
	KEY0+KEY1+KEY2++KEY3,			// Loc 42: All directions
	KEY0+KEY3,								// Loc 43: East, West
	KEY1+KEY2+KEY3,						// Loc 44: North, South, West
	KEY1+KEY2,								// Loc 45: North, South
	KEY0+KEY1+KEY2,						// Loc 46: North, East, South
	KEY0+KEY3,								// Loc 47: East, West
	KEY0+KEY3,								// Loc 48: East, West
	KEY1+KEY3,								// Loc 49: South, West
	KEY1,											// Loc 50: South
	KEY1+KEY2,								// Loc 51: North, South
	KEY1,											// Loc 52: South
	KEY1+KEY2,								// Loc 53: North, South
	KEY1+KEY2,								// Loc 54: North, South
	KEY2,											// Loc 55: North
	0,												// Loc 56: Not a valid spot
	0,												// Loc 57: Not a valid spot
	KEY1+KEY2,								// Loc 58: North, South
	KEY1+KEY2,								// Loc 59: North, South
	KEY1+KEY2,								// Loc 60: North, South
	KEY0+KEY2,								// Loc 61: North, East
	KEY2+KEY3,								// Loc 62: North, West
	KEY0+KEY2,								// Loc 63: North, East
	KEY0+KEY3,								// Loc 64: East, West
	KEY0+KEY3,								// Loc 65: East, West
	KEY0+KEY3,								// Loc 66: East, West
	KEY0+KEY2+KEY3,						// Loc 67: North, East, West
	KEY0+KEY1+KEY2+KEY3,			// Loc 68: All directions
	KEY0+KEY2+KEY3,						// Loc 69: North, East, West
	KEY0+KEY3,								// Loc 70: East, West
	KEY1+KEY3,								// Loc 71: South, West
	KEY0,											// Loc 72: East
	KEY0+KEY3,								// Loc 73: East, West
	KEY0+KEY3,								// Loc 74: East, West
	KEY0+KEY3,								// Loc 75: East, West
	KEY0+KEY3,								// Loc 76: East, West
	KEY0+KEY2+KEY3,						// Loc 77: North, East, West
	KEY3,											// Loc 78: West
	0,												// Loc 79: Not a valid spot
	KEY2,											// Loc 80: North
			// ACTION SPOTS
	,											// Loc 81:
	,											// Loc 82:
	,											// Loc 83:
	,											// Loc 84:
	,											// Loc 85:
	,											// Loc 86:
	,											// Loc 87:
	,											// Loc 88:
	,											// Loc 89:
	,											// Loc 90:
	,											// Loc 91:
	,											// Loc 92:
	,											// Loc 93:
	,											// Loc 94:
	,											// Loc 95:
	,											// Loc 96:
	,											// Loc 97:
	,											// Loc 98:
	,											// Loc 99:


};

// lut for what message to display at each location
const char LUT_location_msg [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
  "You awake in a pitch black room of cold, hard stone. You hear an oddly         familiar growl in the distance...",  // loc 0: Start Screen

};

// lut for the question to ask at each location
const char LUT_location_question [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
  "You fumble around in the dark and find a candle, and some matchsticks. Will    you light the canlde & begin your journey?"  // Loc 0: Start screen
};

// lut for the text associated with the North option (if applicable)
const char LUT_loc_north_option [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
	" ",			// Loc 0: Start screen (no north option)
};

// lut for the text associated with the East option (if applicable)
const char LUT_loc_east_option [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
	"KEY0: Start Journey...",			// Loc 0: Start screen
};

// lut for the text associated with the South option (if applicable)
const char LUT_loc_south_option [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
	" ",			// Loc 0: Start screen (no south option)
};

// lut for the text associated with the West option (if applicable)
const char LUT_loc_west_option [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
	" ",			// Loc 0: Start screen (no west option)
};

/* Definition of Semaphores & Mailboxes */

OS_EVENT	*LocSem;
OS_EVENT	*MBoxRemTime;
INT8U 		err;

/* Definition of Event Flags */

OS_FLAG_GRP *GameStatus;
OS_FLAGS value;

#define GAME_ACTIVE 				0x01  // 0 = IDLE, 1 = ACTIVE
#define GAME_NEW_LOCATION		0x02  // 0 = No new location to render, 1 = New location to render (within ACTIVE state)
#define GAME_FINISHED				0x04  // 0 = GAME ONGOING (or IDLE), 1 = GAME COMPLETED
#define GAME_LOST						0x08	// 0 = GAME ONGONG (or IDLE), 1 = GAME LOST
#define GAME_RESET					0x10  // 0 = GAME ONGOING (or IDLE), 1 = GAME RESET


/* Definition of Task Stacks */

#define   TASK_STACKSIZE       2048
OS_STK    TaskStartScreen_Stk[TASK_STACKSIZE];
OS_STK    TaskMakeChoice_stk[TASK_STACKSIZE];
OS_STK    TaskStopwatch_stk[TASK_STACKSIZE];
OS_STK    TaskDispNewLocation_stk[TASK_STACKSIZE];
OS_STK    TaskDispRemTime_stk[TASK_STACKSIZE];
OS_STK    TaskDispResults_stk[TASK_STACKSIZE];


/* Definition of Task Priorities */

#define TASKMAKECHOICE_PRIORITY				5
#define TASKSTOPWATCH_PRIORITY				6
#define TASKDISPNEWLOCATION_PRIORITY	10
#define TASKDISPREMTIME_PRIORITY    	11
#define TASKDISPRESULTS_PRIORITY    	12
#define TASKSTARTSCREEN_PRIORITY			13


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

	for (x = 0; x < 320; x++) {
		for (y = 0; y < 240; y++) {
			VGA_pixel(x, y, 0);
		}
	}
}

void VGA_text(int x, int y, char * text_ptr) {
	// Subroutine to send a string of text to the VGA monitor
	int offset = (y << 7) + x;
	volatile char *character_buffer = (char *) 0x09000000;   // VGA character buffer

	int char_count = 0;
	while ( *(text_ptr) ) {
		if (char_count >= 79) {
			y += 2;
			offset = (y << 7) + x;
			char_count = 0;
		}
		*(character_buffer + offset) = *(text_ptr);   // write to the character buffer
		++text_ptr;
		++offset;
		++char_count;
	}
}

void VGA_text_clear() {
	// Subroutine to clear the text on the VGA monitor
	int x, y, offset;
	volatile char *character_buffer = (char *) 0x09000000;   // VGA character buffer

	for (x = 0; x < 80; x++) {
		for (y = 0; y < 60; y++) {
			*(character_buffer + offset) = ' ';   // write to the character buffer
			offset = (y << 7) + x;
		}
	}
}

void VGA_clear_timers() {
	// Clear chunk of VGA screen dedicated to displaying the time remaining
	// Chunk: bottom-right corner of VGA character buffer (8x4)
	int x, y, offset;
	volatile char *character_buffer = (char *) 0x09000000;   // VGA character buffer

	for (x = 72; x < 80; x++) {
		for (y = 56; y < 60; y++) {
			*(character_buffer + offset) = ' ';   // write to the character buffer
			offset = (y << 7) + x;
		}
	}
}


void VGA_north_arrow() {
	// draws a north (upwards) facing arrow
	int x_base = 150;
	int y_base = 120;
	short pixel_color = 0xFFFF;		// white

	// draw the center line
	for (int x = x_base; x < (x_base+4); x++) {
		for (int y = y_base; y < (y_base+20); y++) {
			VGA_pixel(x, y, pixel_color);
		}
	}

	// Draw the triangle
	for (int y = y_base-1; y < (y_base+8); y++) {
		for (int x = (x_base-(y-y_base)); x < (x_base+(y-y_base))+4; x++) {
			VGA_pixel(x, y, pixel_color);
		}
	}
}

void VGA_south_arrow() {
	// draws a south (downwards) facing arrow
	int x_base = 150;
	int y_base = 200;
	short pixel_color = 0xFFFF;		// white

	// draw the center line
	for (int x = x_base; x < (x_base+4); x++) {
		for (int y = y_base; y < (y_base+20); y++) {
			VGA_pixel(x, y, pixel_color);
		}
	}

	// Draw the triangle
	for (int y = y_base+19; y > (y_base+12); y--) {
		for (int x = (x_base-(y-y_base)); x < (x_base+(y-y_base))+4; x++) {
			VGA_pixel(x, y, pixel_color);
		}
	}
}

void VGA_east_arrow() {
	// draws a east (right-wards) facing arrow
	int x_base = 230;
	int y_base = 168;
	short pixel_color = 0xFFFF;		// white

	// draw the center line
	for (int y = y_base; y < (y_base+4); y++) {
		for (int x = x_base; x < (x_base+20); x++) {
			VGA_pixel(x, y, pixel_color);
		}
	}

	// Draw the triangle
	for (int x = x_base+21; x > (x_base+12); x--) {
		for (int y = (y_base-(x-x_base)); y < (y_base+(x-x_base))+4; y++) {
			VGA_pixel(x, y, pixel_color);
		}
	}
}

void VGA_west_arrow() {
	// draws a west (left-wards) facing arrow
	int x_base = 70;
	int y_base = 168;
	short pixel_color = 0xFFFF;		// white

	// draw the center line
	for (int y = y_base; y < (y_base+4); y++) {
		for (int x = x_base; x < (x_base+20); x++) {
			VGA_pixel(x, y, pixel_color);
		}
	}

	// Draw the triangle
	for (int x = x_base-1; x < (x_base+8); x++) {
		for (int y = (y_base-(x-x_base)); y < (y_base+(x-x_base))+4; y++) {
			VGA_pixel(x, y, pixel_color);
		}
	}
}

void VGA_disp_options(int loc) {
	// Draws arrows & prints messages next to those arrows based on current location

	// Go through each direction and, if it's allowed, display the arrow & option msg
	if (LUT_location_permissions[loc] & KEY2) {
		// North
		VGA_north_arrow();
		VGA_text(NORTH_MSG_BASE_X, NORTH_MSG_BASE_Y, LUT_loc_north_option[loc]);
	}

	if (LUT_location_permissions[loc] & KEY0) {
		// East
		VGA_east_arrow();
		VGA_text(EAST_MSG_BASE_X, EAST_MSG_BASE_Y, LUT_loc_east_option[loc]);
	}

	if (LUT_location_permissions[loc] & KEY1) {
		// South
		VGA_south_arrow();
		VGA_text(SOUTH_MSG_BASE_X, SOUTH_MSG_BASE_Y, LUT_loc_south_option[loc]);
	}

	if (LUT_location_permissions[loc] & KEY3) {
		// West
		VGA_west_arrow();
		VGA_text(WEST_MSG_BASE_X, WEST_MSG_BASE_Y, LUT_loc_west_option[loc]);
	}

}

/* Definition of Tasks */

void TaskStartScreen(void* pdata) {
	// Initial task
	// Needs to stay until KEY0 is pressed
	// Can only run in idle state; then once started, pends until GAME_RESET is high

	int seed_val = OSTimeGet(); // for random number generation

	while(1) {

		// Don't run until active / finished states are completed/inactive
		value = OSFlagPend(GameStatus, GAME_ACTIVE + GAME_FINISHED, OS_FLAG_WAIT_CLR_ALL, 0, &err);

		// increment seed_val, reseed program
		seed_val = OSTimeGet();
		srand(seed_val);

		// Reset Global Game Vars
		location = 0;
		step_count = 0;
		time_250ms = 0;
    step_time_rem_SS = 30;
    tot_time_rem_SS = 9;
    tot_time_rem_MM = 59;
		tot_time_SS = 0;
		tot_time_MM = 0;

		// display initial message
		VGA_text(MSG_BASE_X, MSG_BASE_Y, LUT_location_msg[location]);

		// display initial question
		VGA_text(QUESTION_BASE_X, QUESTION_BASE_Y, LUT_location_question[location]);

		// TODO: TEST DISPLAYING ARROWS
		VGA_north_arrow();
		VGA_south_arrow();
		VGA_east_arrow();
		VGA_west_arrow();

		// Wait until game is reset until reseting & idling again
		value = OSFlagPend(GameStatus, GAME_RESET, OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 0, &err);

		OSTimeDly(4);

	}
}

void TaskMakeChoice(void* pdata) {
	// Needed in all states - never pend
	// Driver for most states / event flags

	OS_FLAGS flags;

	while(1) {

		// Protect Key Inputs with KeySem
		OSSemPend(LocSem, 0, &err);

		// Read Key Values
		KEY_val = *(KEY_ptr) & 0xF;
		if (KEY_val == KEY0) {				        // KEY0
			KEY0_flag = 1;
			KEY1_flag = 0;
			KEY2_flag = 0;
			KEY3_flag = 0;
		} else if (KEY_val == KEY1) {				// KEY1
			KEY0_flag = 0;
			KEY1_flag = 1;
			KEY2_flag = 0;
			KEY3_flag = 0;
		} else if (KEY_val == KEY2) {				// KEY2
			KEY0_flag = 0;
			KEY1_flag = 0;
			KEY2_flag = 1;
			KEY3_flag = 0;
		} else if (KEY_val == KEY3) {				// KEY2
			KEY0_flag = 0;
			KEY1_flag = 0;
			KEY2_flag = 0;
			KEY3_flag = 1;
		}


		// Read Status of GameStatus Flags
		flags = OSFlagQuery(GameStatus, &err);

		// State driver - Dependent on what state we're currently in
		if (!(flags & GAME_ACTIVE)) {
			// IDLE state

			if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
				KEY0_flag = 0;
				value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_NEW_LOCATION, OS_FLAG_SET, &err);
			}
		}

		if ( (flags & GAME_ACTIVE) && (!(flags & GAME_NEW_LOCATION)) ) {
			// ACTIVE state (NOT rendering new location, i.e. taking input)

			// TODO - add edge cases for "action spots"
				// Before checking for regular locations? Probably
			if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
				// KEY0 press - Typically means go East (rightwards)
				KEY0_flag = 0;
				if (LUT_location_permissions[location] & KEY0) {
					location += 1;		// increment place in map by 1
				}

			} else if ( !(KEY_val & KEY1) && (KEY1_flag) ) {
				// KEY1 press - Typically means go South (downwards)
				KEY1_flag = 0;
				if (LUT_location_permissions[location] & KEY1) {
					location += 9;		// increment place in map by 9
				}

			} else if ( !(KEY_val & KEY2) && (KEY2_flag) ) {
				// KEY2 press - Typically means go North (upwards)
				KEY2_flag = 0;
				if (LUT_location_permissions[location] & KEY2) {
					location -= 9;		// decrement place in map by 9
				}

			} else if ( !(KEY_val & KEY3) && (KEY3_flag) ) {
				// KEY3 press - Typically means go West (leftwards)
				KEY3_flag = 0;
				if (LUT_location_permissions[location] & KEY3) {
					location -= 1;		// decrement place in map by 1
				}
			}
		}

		if (flags & GAME_FINISHED) {
			// GAME FINISHED state

			// disable active game state flags
			value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_NEW_LOCATION, OS_FLAG_CLR, &err);

			if  ( !(KEY_val & KEY0) && (KEY0_flag) ) {
				// Reset game
				KEY0_flag = 0;
				value = OSFlagPost(GameStatus, GAME_RESET, OS_FLAG_SET, &err);
			}
		}

		if (flags & GAME_LOST) {
			// GAME LOST state

			// disable active game state flags
			value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_NEW_LOCATION, OS_FLAG_CLR, &err);

			if  ( !(KEY_val & KEY0) && (KEY0_flag) ) {
				// Reset game
				KEY0_flag = 0;
				value = OSFlagPost(GameStatus, GAME_RESET, OS_FLAG_SET, &err);
			}
		}

		if (flags & GAME_RESET) {
			// GAME RESET state

			// Reset event flags to default settings
			value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_NEW_LOCATION + GAME_FINISHED + GAME_LOST, OS_FLAG_CLR, &err);
		}

		OSSemPost(LocSem);

		OSTimeDly(1);
	}
}


void TaskStopwatch(void* pdata) {
	// Needs ACTIVE state & PLAY state to run

	OS_FLAGS flags;

	while(1) {

		// pend until game is activated (to prevent useless processing in IDLE state)
		value = OSFlagPend(GameStatus, GAME_ACTIVE, OS_FLAG_WAIT_SET_ALL, 0, &err);

		// pend if any non-active states (LOST, RESET, FINISHED) are high
		value = OSFlagPend(GameStatus, GAME_LOST+GAME_RESET+GAME_FINISHED, OS_FLAG_WAIT_CLR_ALL, 0, &err);

		time_250ms++;
		if (time_250ms >= 4) {
			time_250ms = 0;

			// counter for total time elapsed
			if (tot_time_SS >= 59) {
				tot_time_SS = 0;

				if (tot_time_MM >= 59) {
					tot_time_MM = 0;
				} else {
					tot_time_MM++;
				}

			} else {
				tot_time_SS++;
			}

			// counter for time remaining each step
			if (step_time_rem_SS > 0) {
				step_time_rem_SS--;
			} else {
				// SS are zero -> game over!
				value = OSFlagPost(GameStatus, GAME_LOST, OS_FLAG_SET, &err);
			}

			// counter for total game time remaining
			if (tot_time_rem_SS > 0) {
				tot_time_rem_SS--;

			} else {
				tot_time_rem_SS = 59;

				if (tot_time_rem_MM > 0) {
					tot_time_rem_MM--;

				} else {
					// MM & SS are zero -> game over!
					value = OSFlagPost(GameStatus, GAME_LOST, OS_FLAG_SET, &err);
				}
			}

		}

		// Send step time remaining to taskDispRemTime
		char step_time_rem_SS_char[VGA_TEXT_MAX_SIZE];
		sprintf(step_time_rem_SS_char, "%.2ds", step_time_rem_SS);

		OSMboxPost(MBoxRemTime, (void *)&step_time_rem_SS_char[0]);

		OSTimeDly(2);

	}
}


void TaskDispNewLocation(void* pdata) {
	// Displays options & relevant text based on the current location

	while(1) {

		// Wait for game to be active, and for a new location to render. consumer new location flag
		value = OSFlagPend(GameStatus, GAME_ACTIVE, OS_FLAG_WAIT_SET_ALL, 0, &err);
		value = OSFlagPend(GameStatus, GAME_NEW_LOCATION, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);

		step_count++;

		OSSemPend(LocSem, 0, &err);

		// Clear VGA before displaying new location
	  VGA_clear();
	  VGA_text_clear();

		// display message based on current (new) location
		VGA_text(MSG_BASE_X, MSG_BASE_Y, LUT_location_msg[location]);

		// display question based on current (new) location
		VGA_text(QUESTION_BASE_X, QUESTION_BASE_Y, LUT_location_question[location]);

		// display options based on current (new) location
		VGA_disp_options(location);

		OSSemPost(LocSem);

		OSTimeDly(4);

	}
}

void TaskDispRemTime(void* pdata) {
	// displays time remaining (for entire game, and each step)
	char *time_msg; // to receive MBoxRemTime from stopwatch

	while(1) {

		// pend until game is activated (to prevent useless processing in IDLE state)
		value = OSFlagPend(GameStatus, GAME_ACTIVE, OS_FLAG_WAIT_SET_ALL, 0, &err);

		// pend if any non-active states (LOST, RESET, FINISHED) are high
		value = OSFlagPend(GameStatus, GAME_LOST+GAME_RESET+GAME_FINISHED, OS_FLAG_WAIT_CLR_ALL, 0, &err);

		// receive msg from MBoxRemTime (from taskStopwatch) - Pend until it's ready
		time_msg = (char *)OSMboxPend(MBoxRemTime, 0 , &err);

		if (err == OS_ERR_NONE) {
			// No error; display remaining time
			// TODO: Display Remaining Step Time

		} else {
			// Error on receiving msg
			int current_time = OSTimeGet();
			printf("%d: Error on Receiving MBoxRemTime via OSMboxPend in TaskDispRemTime\n", current_time);
		}


		OSTimeDly(4);

	}
}


void TaskDispResults(void* pdata) {
	// Can only run in GAME_FINISHED state

	while(1) {

		// blocking delay until game is finished (to prevent useless processing in IDLE state)
		value = OSFlagPend(GameStatus, GAME_FINISHED, OS_FLAG_WAIT_SET_ALL, 0, &err);

		// Receive the total elapsed time value from TaskStopwatch, display on top row of LCD

		// Display total score & elapsed time global vars
		char tot_time_MMSS_msg[VGA_TEXT_MAX_SIZE];
		sprintf(tot_time_MMSS_msg, "%.2d:%.2d", tot_time_MM, tot_time_SS);


		// after running once, wait until a new game is started (and finished)
		value = OSFlagPend(GameStatus, GAME_RESET, OS_FLAG_WAIT_SET_ALL, 0, &err);

		OSTimeDly(4);

	}
}


/* The main function creates the tasks and starts the program */
int main(void)
{

	// Clear VGA before operation
  VGA_clear();
  VGA_text_clear();

  OSTaskCreateExt(TaskStartScreen,
                  NULL,
                  (void *)&TaskStartScreen_Stk[TASK_STACKSIZE-1],
				          TASKSTARTSCREEN_PRIORITY,
				          TASKSTARTSCREEN_PRIORITY,
								  TaskStartScreen_Stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(TaskMakeChoice,
                  NULL,
                  (void *)&TaskMakeChoice_stk[TASK_STACKSIZE-1],
				          TASKMAKECHOICE_PRIORITY,
				          TASKMAKECHOICE_PRIORITY,
				          TaskMakeChoice_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(TaskStopwatch,
                  NULL,
                  (void *)&TaskStopwatch_stk[TASK_STACKSIZE-1],
  			          TASKSTOPWATCH_PRIORITY,
  			          TASKSTOPWATCH_PRIORITY,
  			          TaskStopwatch_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(TaskDispRemTime,
                  NULL,
                  (void *)&TaskDispRemTime_stk[TASK_STACKSIZE-1],
  			          TASKDISPREMTIME_PRIORITY,
  			          TASKDISPREMTIME_PRIORITY,
  			          TaskDispRemTime_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(TaskDispResults,
                  NULL,
                  (void *)&TaskDispResults_stk[TASK_STACKSIZE-1],
  			          TASKDISPRESULTS_PRIORITY,
  			          TASKDISPRESULTS_PRIORITY,
  			          TaskDispResults_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(TaskDispNewLocation,
                  NULL,
                  (void *)&TaskDispNewLocation_stk[TASK_STACKSIZE-1],
  			          TASKDISPNEWLOCATION_PRIORITY,
  			          TASKDISPNEWLOCATION_PRIORITY,
  			          TaskDispNewLocation_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);


  LocSem = OSSemCreate(1);

  MBoxRemTime = OSMboxCreate((void *)0);

  GameStatus = OSFlagCreate(0x00, &err);

  OSStart();

  return 0;
}
