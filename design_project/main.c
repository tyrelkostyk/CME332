#include <stdio.h>
#include <stdlib.h>
#include "includes.h"

/* Definition of Global Vars and Mem-Mapped Pointers */

#define KEY_ptr					(((volatile unsigned long *)0xFF200050))		// Mem Addr for KEY3-0 Pushbuttons
#define BYTE_SIZE				8             // Byte size, in bits (for offsetting HEX display inputs)
#define LCD_SIZE				40

#define KEY0						0x01
#define KEY1						0x02
#define KEY2						0x04
#define KEY3						0x08

#define MAX_LOCATIONS 	85
#define VGA_TEXT_MAX  	80

int KEY_val, SW_val;
int KEY0_flag, KEY1_flag, KEY2_flag, KEY3_flag;

int step_count;

int time_250ms;
int time_rem_SS = 30;
int tot_time_rem_SS, tot_time_rem_MM;
int tot_time_SS, tot_time_MM;


/* Definitions of LUTs for unique properties of each labyrinth location */

// lut for what buttons can be pressed (and do something) for each location
const char LUT_permissions [MAX_LOCATIONS] = {
  KEY0,                 // Loc 0: Press KEY0 to Start (East)
  KEY0,                 // Loc 1: East
  KEY0,                 // Loc 2: East
  KEY0,                 // Loc 3: East
  KEY0,                 // Loc 4: East
  KEY0,                 // Loc 5: East
  KEY0+KEY1,            // Loc 6: All directions


};

// lut for what message to display at each location
const char LUT_location_msg [MAX_LOCATIONS][VGA_TEXT_MAX] = {
  "You awake in a pitch black room of hard stone. Something growls in the distance",  // loc 0: Start Screen

};

// lut for the question to ask at each location
const char LUT_location_question [MAX_LOCATIONS][VGA_TEXT_MAX] = {
  "You find a candle & matchsticks. Will you light the canlde & begin your journey?"  // Loc 0: Start screen
};


/* Definition of Semaphores & Mailboxes */

OS_EVENT	*KeySem;
OS_EVENT	*MBoxRemTime;
INT8U 		err;

/* Definition of Event Flags */

OS_FLAG_GRP *GameStatus;
OS_FLAGS value;

#define GAME_ACTIVE 				0x01  // 0 = IDLE, 1 = ACTIVE
#define GAME_NEW_LOCATION		0x02  // 0 = No new location to render, 1 = New location to render (within ACTIVE state)
#define GAME_FINISHED				0x04  // 0 = GAME ONGOING (or IDLE), 1 = GAME COMPLETED
#define GAME_RESET					0x08  // 0 = GAME ONGOING (or IDLE), 1 = GAME RESET


/* Definition of Task Stacks */

#define   TASK_STACKSIZE       2048
OS_STK    TaskStartScreen_Stk[TASK_STACKSIZE];
OS_STK    TaskMakeChoice_stk[TASK_STACKSIZE];
OS_STK    TaskStopwatch_stk[TASK_STACKSIZE];
OS_STK    TaskDispOptions_stk[TASK_STACKSIZE];
OS_STK    TaskDispRemTime_stk[TASK_STACKSIZE];
OS_STK    TaskDispResults_stk[TASK_STACKSIZE];


/* Definition of Task Priorities */

#define TASKMAKECHOICE_PRIORITY			5
#define TASKSTOPWATCH_PRIORITY			6
#define TASKDISPOPTIONS_PRIORITY		10
#define TASKDISPREMTIME_PRIORITY    11
#define TASKDISPRESULTS_PRIORITY    12
#define TASKSTARTSCREEN_PRIORITY		13


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

void TaskIdle(void* pdata) {
	// Initial task
	// Needs to stay until KEY1 is pressed
	// Can only run in idle state; then once started, pends until GAME_RESET is high

	int seed_val = OSTimeGet(); // for random number generation

	char *idle_msg = "KEY1 to start...";

	while(1) {

		// Don't run until active / finished states are completed/inactive
		value = OSFlagPend(GameStatus, GAME_ACTIVE + GAME_FINISHED, OS_FLAG_WAIT_CLR_ALL, 0, &err);

		// increment seed_val, reseed program
		seed_val = OSTimeGet();
		srand(seed_val);

		// Clear LCD Display
		LCD_clear();

		// Reset Global Game Vars
		step_count = 0;
		time_250ms = 0;
    time_rem_SS = 30;
    tot_time_rem_SS = 0;
    tot_time_rem_MM = 0;
		tot_time_SS = 0;
		tot_time_MM = 0;

		// Print "Press KEY1 to start game..." on top row of LCD Display
		LCD_cursor(0,1);
		LCD_text(idle_msg);

		// Wait until game is reset until idling again
		value = OSFlagPend(GameStatus, GAME_RESET, OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 0, &err);

		OSTimeDly(4);

	}
}

void TaskScanKey(void* pdata) {
	// Needed in all states - never pend
	// Driver for most (all?) states / event flags; for sure the ones driven by KEY presses

	OS_FLAGS flags;

	while(1) {

		// Protect Key Inputs with KeySem
		OSSemPend(KeySem, 0, &err);

		// Read Key Values
		KEY_val = *(KEY_ptr) & 0xF;
		if (KEY_val == KEY0) {				        // KEY0
			KEY0_flag = 1;
			KEY1_flag = 0;
			KEY2_flag = 0;
		} else if (KEY_val == KEY1) {				// KEY1
			KEY0_flag = 0;
			KEY1_flag = 1;
			KEY2_flag = 0;
		} else if (KEY_val == KEY2) {				// KEY2
			KEY0_flag = 0;
			KEY1_flag = 0;
			KEY2_flag = 1;
		}


		// Read Status of GameStatus Flags
		flags = OSFlagQuery(GameStatus, &err);

		// State driver - Dependent on what state we're currently in
		if (!(flags & GAME_ACTIVE) && (flags < 0x08) ) {
			// IDLE state

			if ( !(KEY_val & KEY1) && (KEY1_flag) ) {
				// Start the game - Enable Active State
				KEY1_flag = 0;
				value = OSFlagPost(GameStatus, GAME_ACTIVE, OS_FLAG_SET, &err);
			}

		} else if ( (flags & GAME_ACTIVE) && (flags < 0x08) ) {
			// ACTIVE state; NOT in finished or reset state

			if (flags & GAME_PAUSED) {
				// PAUSED state

				if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
					// Reset game, enter GAME_RESET state
					KEY0_flag = 0;
					value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_PAUSED + GAME_FINISHED + GAME_WAITING_FOR_ANSWER, OS_FLAG_CLR, &err);
					value = OSFlagPost(GameStatus, GAME_RESET, OS_FLAG_SET, &err);
				} else if ( !(KEY_val & KEY1) && (KEY1_flag) ) {
					// Resume game
					KEY1_flag = 0;
					value = OSFlagPost(GameStatus, GAME_PAUSED, OS_FLAG_CLR, &err);
				}

			} else if (!(flags & GAME_PAUSED)) {
				// PLAY state

				if  ( !(KEY_val & KEY1) && (KEY1_flag) ) {
					// Pause game
					KEY1_flag = 0;
					value = OSFlagPost(GameStatus, GAME_PAUSED, OS_FLAG_SET, &err);

				} else if (flags & GAME_WAITING_FOR_ANSWER) {
					// Playing - WAITING_FOR_INPUT State

					if  ( !(KEY_val & KEY2) && (KEY2_flag) ) {
						// Submit answer
						KEY2_flag = 0;
						value = OSFlagPost(GameStatus, GAME_WAITING_FOR_ANSWER, OS_FLAG_CLR, &err);
					}
				}
			}
		}

		if (flags & GAME_FINISHED) {
			// GAME_FINISHED state

			// disable active game state flags
			value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_PAUSED + GAME_WAITING_FOR_ANSWER, OS_FLAG_CLR, &err);

			if  ( !(KEY_val & KEY0) && (KEY0_flag) ) {
				// Reset game
				KEY0_flag = 0;
				value = OSFlagPost(GameStatus, GAME_RESET, OS_FLAG_SET, &err);
			}
		}

		if (flags & GAME_RESET) {
			// GAME_RESET state

			// Reset event flags to default settings
			value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_PAUSED + GAME_FINISHED + GAME_WAITING_FOR_ANSWER, OS_FLAG_CLR, &err);
		}

		OSSemPost(KeySem);

		OSTimeDly(1);
	}
}


void TaskStopwatch(void* pdata) {
	// Needs ACTIVE state & PLAY state to run

	OS_FLAGS flags;

	while(1) {

		// blocking delay until game is activated (to prevent useless processing in IDLE state)
		value = OSFlagPend(GameStatus, GAME_ACTIVE, OS_FLAG_WAIT_SET_ALL, 0, &err);

		// non-blocking check of GameStatus
		// to run counter: GAME_ACTIVE = 1, GAME_PAUSED = 0
		// to send total time elapsed to taskDispResults: GAME_FINISHED = 1
		flags = OSFlagQuery(GameStatus, &err);

		if ( (flags & GAME_ACTIVE) && (!(flags & GAME_PAUSED)) ) {
			// Game is ACTIVE, and NOT PAUSED

			time_250ms++;
			if (time_250ms >= 4) {
				time_250ms = 0;

				// counter for total time
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

				// counter for time remaining
				if (time_rem_SS > 0) {
					time_rem_SS--;
				} else {
					value = OSFlagPost(GameStatus, GAME_RESET, OS_FLAG_SET, &err);
				}
			}

			// Send time remaining to taskDispRemTime
			char time_rem_SS_char[LCD_SIZE];
			sprintf(time_rem_SS_char, "%.2ds             ", time_rem_SS);

			OSMboxPost(MBoxRemTime, (void *)&time_rem_SS_char[0]);

		}

		OSTimeDly(1);

	}
}


void TaskGame(void* pdata) {

	int rand_num;

	while(1) {

		// blocking delay until game is activated (to prevent useless processing in IDLE state)
		value = OSFlagPend(GameStatus, GAME_ACTIVE, OS_FLAG_WAIT_SET_ALL, 0, &err);

		round_num++;

		// generate new random number, between 1 & 255
		rand_num = ( (rand() % 255) + 1 );

		// display rand number
		char rand_msg[LCD_SIZE];
		sprintf(rand_msg, "%d             ", rand_num);

		LCD_cursor(0,1);
		LCD_text(rand_msg);

		// wait for user input answer
		value = OSFlagPost(GameStatus, GAME_WAITING_FOR_ANSWER, OS_FLAG_SET, &err);
		value = OSFlagPend(GameStatus, GAME_WAITING_FOR_ANSWER, OS_FLAG_WAIT_CLR_ALL, 0, &err);

		if (binary_answer == rand_num) {
			score++;
		}

		// once 10 rounds have been completed, clear active game flags & enter GAME_FINISHED state
		if (round_num >= 10) {
			value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_PAUSED + GAME_WAITING_FOR_ANSWER, OS_FLAG_CLR, &err);
			value = OSFlagPost(GameStatus, GAME_FINISHED, OS_FLAG_SET, &err);
		}

		time_rem_SS = 30;

		OSTimeDly(2);

	}
}

void TaskDispRemTime(void* pdata) {
	char *time_msg; // to receive MBoxRemTime from stopwatch

	OS_FLAGS flags;

	while(1) {

		// blocking delay until game is activated (to prevent useless processing in IDLE state)
		value = OSFlagPend(GameStatus, GAME_ACTIVE, OS_FLAG_WAIT_SET_ALL, 0, &err);

		// non-blocking delay ensuring game is active and in play mode
		flags = OSFlagQuery(GameStatus, &err);

		if ( (flags & GAME_ACTIVE) && (!(flags & GAME_PAUSED)) && (!(flags & GAME_FINISHED))) {

			// receive msg from MBoxRemTime (from taskStopwatch) - Pend until it's ready
			time_msg = (char *)OSMboxPend(MBoxRemTime, 0 , &err);

			if (err == OS_ERR_NONE) {
				// No error; display remaining time on top row of LCD
				LCD_cursor(0,0);
				LCD_text(time_msg);

			} else {
				// Error on receiving msg
				int current_time = OSTimeGet();
				printf("%d: Error on Receiving MBoxRemTime via OSMboxPend in TaskDispRemTime\n", current_time);
			}

			char score_msg[LCD_SIZE];
			sprintf(score_msg, "%d", score);

			LCD_cursor(6,0);
			LCD_text(score_msg);
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

		// Clear LCD upon entering state
		LCD_clear();

		// Display total score & elapsed time global vars
		char tot_time_MMSS_msg[LCD_SIZE];
		sprintf(tot_time_MMSS_msg, "%.2d:%.2d", tot_time_MM, tot_time_SS);

		LCD_cursor(0,0);
		LCD_text(tot_time_MMSS_msg);

		char score_msg[LCD_SIZE];
		sprintf(score_msg, "%d         ", score);

		LCD_cursor(6,0);
		LCD_text(score_msg);

		// after running once, wait until a new game is started (and finished)
		value = OSFlagPend(GameStatus, GAME_RESET, OS_FLAG_WAIT_SET_ALL, 0, &err);

		OSTimeDly(4);

	}
}


/* The main function creates the tasks and starts the program */
int main(void)
{

  LCD_clear();  		// Clear LCD before operation
  LCD_cursor_off();  	// turn off blinking LCD Cursor

  OSTaskCreateExt(TaskIdle,
                  NULL,
                  (void *)&TaskIdle_Stk[TASK_STACKSIZE-1],
				          TASKIDLE_PRIORITY,
				          TASKIDLE_PRIORITY,
						  TaskIdle_Stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(TaskScanKey,
                  NULL,
                  (void *)&TaskScanKey_stk[TASK_STACKSIZE-1],
				          TASKSCANKEY_PRIORITY,
				          TASKSCANKEY_PRIORITY,
				          TaskScanKey_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(TaskScanSW,
                  NULL,
                  (void *)&TaskScanSW_stk[TASK_STACKSIZE-1],
				          TASKSCANSW_PRIORITY,
				          TASKSCANSW_PRIORITY,
				          TaskScanSW_stk,
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

  OSTaskCreateExt(TaskGame,
                  NULL,
                  (void *)&TaskGame_stk[TASK_STACKSIZE-1],
  			          TASKGAME_PRIORITY,
  			          TASKGAME_PRIORITY,
  			          TaskGame_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  KeySem = OSSemCreate(1);

  MBoxRemTime = OSMboxCreate((void *)0);
  MBoxTotTime = OSMboxCreate((void *)0);

  GameStatus = OSFlagCreate(0x00, &err);

  OSStart();

  return 0;
}
