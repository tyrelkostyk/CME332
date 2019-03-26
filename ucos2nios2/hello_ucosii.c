/*************************************************************************
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.      *
* All rights reserved. All use of this software and documentation is     *
* subject to the License Agreement located at the end of this file below.*
**************************************************************************
* Description:                                                           *
* The following is a simple hello world program running MicroC/OS-II.The *
* purpose of the design is to be a very simple application that just     *
* demonstrates MicroC/OS-II running on NIOS II.The design doesn't account*
* for issues such as checking system call return codes. etc.             *
*                                                                        *
* Requirements:                                                          *
*   -Supported Example Hardware Platforms                                *
*     Standard                                                           *
*     Full Featured                                                      *
*     Low Cost                                                           *
*   -Supported Development Boards                                        *
*     Nios II Development Board, Stratix II Edition                      *
*     Nios Development Board, Stratix Professional Edition               *
*     Nios Development Board, Stratix Edition                            *
*     Nios Development Board, Cyclone Edition                            *
*   -System Library Settings                                             *
*     RTOS Type - MicroC/OS-II                                           *
*     Periodic System Timer                                              *
*   -Know Issues                                                         *
*     If this design is run on the ISS, terminal output will take several*
*     minutes per iteration.                                             *
**************************************************************************/

#include <stdio.h>
#include "includes.h"

/* Definition of Global Vars and Mem-Mapped Pointers */

#define KEY_ptr			          (((volatile unsigned long *)0xFF200050))		// Mem Addr for KEY3-0 Pushbuttons
#define BYTE_SIZE             8             // Byte size, in bits (for offsetting HEX display inputs)
#define LCD_SIZE 40

int KEY_val;
int KEY0_flag, KEY1_flag, KEY2_flag, KEY3_clear_counter_flag, KEY3_clear_stopwatch_flag;
int RUN_flag;  // 0 = STOP, 1 = RUN

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

/* Supporting Functions */

//////* LCD DISPLAY FUNCTIONS *//////
void LCD_clear(void)
{
  	volatile char * LCD_display_ptr = (char *) 0xFF203050;	// 16x2 character display
	*(LCD_display_ptr) = 0x01;											// clear the LCD
}

void LCD_cursor(int x, int y)
{
  	volatile char * LCD_display_ptr = (char *) 0xFF203050;	// 16x2 character display
	char instruction;

	instruction = x;
	if (y != 0) instruction |= 0x40;				// set bit 6 for bottom row
	instruction |= 0x80;								// need to set bit 7 to set the cursor location
	*(LCD_display_ptr) = instruction;			// write to the LCD instruction register
}

void LCD_text(char * text_ptr)
{
  volatile char * LCD_display_ptr = (char *) 0xFF203050;	// 16x2 character display

	while ( *(text_ptr) )
	{
		*(LCD_display_ptr + 1) = *(text_ptr);	// write to the LCD data register
		++text_ptr;
	}
}


/* Definition of Semaphores & Mailboxes */

OS_EVENT	*KeySem;
OS_EVENT	*DispSem;
OS_EVENT	*MBoxTime;
OS_EVENT	*MBoxCounter;
INT8U 		err;

/* Definition of Task Stacks */

#define   TASK_STACKSIZE       2048
OS_STK    TaskScanKey_stk[TASK_STACKSIZE];
OS_STK    TaskCounter_stk[TASK_STACKSIZE];
OS_STK    TaskStopwatch_stk[TASK_STACKSIZE];
OS_STK    TaskDispTime_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */

#define TASKSCANKEY_PRIORITY 	7
#define TASKSTOPWATCH_PRIORITY  8
#define TASKCOUNTER_PRIORITY  	9
#define TASKDISPTIME_PRIORITY   10

/* Definition of Tasks */

void TaskScanKey(void* pdata) {
	while(1) {
		OSSemPend(KeySem, 0, &err);

		KEY_val = *(KEY_ptr) & 0xF;
		if (KEY_val & 0x01) {				        // KEY0
		KEY0_flag = 1;
		}	else if (KEY_val & 0x02) {				// KEY1
		KEY1_flag = 1;
		} else if (KEY_val & 0x04) {				// KEY2
		KEY2_flag = 1;
		} else if (KEY_val & 0x08) {				// KEY3 - Two flags, because needed in 2 tasks
		KEY3_clear_counter_flag = 1;
		if (!RUN_flag) {KEY3_clear_stopwatch_flag = 1;}
		}

		OSSemPost(KeySem);

		OSTimeDly(1);
	}
}

void TaskCounter(void* pdata) {

	int counter;

	while(1) {
    OSSemPend(KeySem, 0, &err);

		if ( !(KEY_val & 0x02) && (KEY1_flag) ) {				// KEY1
			if (counter < 9) {counter++;}
			KEY1_flag = 0;
		}
		if ( !(KEY_val & 0x04) && (KEY2_flag) ) {				// KEY2
			if (counter > 0) {counter--;}
			KEY2_flag = 0;
		}
		if ( !(KEY_val & 0x08) && (KEY3_clear_counter_flag) ) {				// KEY3
			counter = 0;
			KEY3_clear_counter_flag = 0;
		}

		char counter_char[LCD_SIZE];
		sprintf(counter_char, "%d", counter);

		OSMboxPost(MBoxCounter, (void *)&counter_char[0]);

		OSSemPost(KeySem);

		OSTimeDly(1);
	}
}

void TaskStopwatch(void* pdata) {

  int time_125ms, time_SS, time_MM;

  while(1) {
    if ( !(KEY_val & 0x01) && (KEY0_flag) ) {       // KEY0
      RUN_flag = !RUN_flag;
      KEY0_flag = 0;
    }

    if (RUN_flag) {				// RUN

      time_125ms++;
      if (time_125ms >= 8) {
        time_125ms = 0;

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

    if ( (!RUN_flag) && !(KEY_val & 0x08) && (KEY3_clear_stopwatch_flag) ) {  // STOP & KEY3
      time_125ms = 0;
      time_SS = 0;
      time_MM = 0;
      KEY3_clear_stopwatch_flag = 0;
    }

    char time_MMSS_char[LCD_SIZE];
    sprintf(time_MMSS_char, "%.2d:%.2d", time_MM, time_SS);

    OSMboxPost(MBoxTime, (void *)&time_MMSS_char[0]);

    OSTimeDly(1);

  }
}

void TaskDispTime(void* pdata) {
	while(1) {

    OSSemPend(DispSem, 0, &err);

    // Receive the time value from TaskStopwatch, display on top row of LCD
    char *time_msg;
    time_msg = (char *)OSMboxPend(MBoxTime, 0, &err);
    if (err == OS_ERR_NONE) {	// Success
      LCD_cursor(0,0);
      LCD_text(time_msg);

    } else {	// Error
      int time = OSTimeGet();
      printf("%d: Error on Receiving MBoxTime via OSMboxPend in TaskDispTime\n", time);
    }

    // Receive the counter value from TaskCounter, display on bottom row of LCD
    char *counter_msg;
    counter_msg = (char *)OSMboxPend(MBoxCounter, 0, &err);
    if (err == OS_ERR_NONE) {	// Success
      LCD_cursor(0,1);
      LCD_text(counter_msg);

    } else {	// Error
      int time = OSTimeGet();
      printf("%d: Error on Receiving MBoxCounter via OSMboxPend in TaskDispTime\n", time);
    }

    OSSemPost(DispSem);

    OSTimeDly(1);

	}
}

/* The main function creates two task and starts multi-tasking */
int main(void)
{

  printf("MicroC/OS-II Licensing Terms\n");
  printf("============================\n");
  printf("Micrium\'s uC/OS-II is a real-time operating system (RTOS) available in source code.\n");
  printf("This is not open-source software.\n");
  printf("This RTOS can be used free of charge only for non-commercial purposes and academic projects,\n");
  printf("any other use of the code is subject to the terms of an end-user license agreement\n");
  printf("for more information please see the license files included in the BSP project or contact Micrium.\n");
  printf("Anyone planning to use a Micrium RTOS in a commercial product must purchase a commercial license\n");
  printf("from the owner of the software, Silicon Laboratories Inc.\n");
  printf("Licensing information is available at:\n");
  printf("Phone: +1 954-217-2036\n");
  printf("Email: sales@micrium.com\n");
  printf("URL: www.micrium.com\n\n\n");

  LCD_clear();  // Clear LCD before operation

  OSTaskCreateExt(TaskScanKey,
                  NULL,
                  (void *)&TaskScanKey_stk[TASK_STACKSIZE-1],
				          TASKSCANKEY_PRIORITY,
				          TASKSCANKEY_PRIORITY,
				          TaskScanKey_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  OSTaskCreateExt(TaskCounter,
                  NULL,
                  (void *)&TaskCounter_stk[TASK_STACKSIZE-1],
				          TASKCOUNTER_PRIORITY,
				          TASKCOUNTER_PRIORITY,
				          TaskCounter_stk,
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

  OSTaskCreateExt(TaskDispTime,
                  NULL,
                  (void *)&TaskDispTime_stk[TASK_STACKSIZE-1],
  			          TASKDISPTIME_PRIORITY,
  			          TASKDISPTIME_PRIORITY,
  			          TaskDispTime_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  KeySem = OSSemCreate(1);
  DispSem = OSSemCreate(1);

  MBoxTime = OSMboxCreate((void *)0);
  MBoxCounter = OSMboxCreate((void *)0);

  OSStart();

  return 0;
}

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/
