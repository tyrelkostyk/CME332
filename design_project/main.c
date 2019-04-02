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

#define NORTH_MSG_BASE_X			31
#define NORTH_MSG_BASE_Y			33

#define EAST_MSG_BASE_X				44
#define EAST_MSG_BASE_Y				41

#define SOUTH_MSG_BASE_X			31
#define SOUTH_MSG_BASE_Y			57

#define WEST_MSG_BASE_X				19
#define WEST_MSG_BASE_Y				41

int KEY_val, SW_val;
int KEY0_flag, KEY1_flag, KEY2_flag, KEY3_flag;

int location, step_count;

int time_250ms;
int step_time_rem_SS, max_step_time_rem;
int tot_time_rem_SS, tot_time_rem_MM;
int tot_time_SS, tot_time_MM;

// Gameplay Global Vars
int sword_flag, key_flag, minotaur_gone_flag, minotaur_slain_flag; // inventory


/* Definition of Semaphores & Mailboxes */

OS_EVENT	*LocSem;
OS_EVENT	*MBoxRemStepTime;
OS_EVENT	*MBoxRemTotTime;
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
OS_STK    TaskDispGameOver_stk[TASK_STACKSIZE];


/* Definition of Task Priorities */

#define TASKMAKECHOICE_PRIORITY				5
#define TASKSTOPWATCH_PRIORITY				6
#define TASKDISPNEWLOCATION_PRIORITY	10
#define TASKDISPREMTIME_PRIORITY    	11
#define TASKDISPRESULTS_PRIORITY    	12
#define TASKDISPGAMEOVER_PRIORITY    	13
#define TASKSTARTSCREEN_PRIORITY			14


/* Definitions of LUTs for unique properties of each labyrinth location */

// lut for what buttons can be pressed (and do something) for each location
const char LUT_location_permissions [MAX_LOCATIONS] = {
  KEY0,                 		// Loc 0: Press KEY0 to Start (East)
  KEY0,                 		// Loc 1: East
  KEY0,                 		// Loc 2: East
  KEY0,                 		// Loc 3: East
  KEY0,                 		// Loc 4: East
  KEY0,                 		// Loc 5: East
  KEY0+KEY1,            		// Loc 6: East, South (after first time)
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
	KEY3,											// Loc 32: West (after investigating)
	KEY1+KEY2,								// Loc 33: North, South
	0,												// Loc 34: Not a valid spot
	KEY1,											// Loc 35: South (after getting sword)
	KEY1+KEY2,								// Loc 36: North, South
	KEY0+KEY1,								// Loc 37: East, South
	KEY0+KEY3,								// Loc 38: East, West
	KEY0+KEY3,								// Loc 39: East, West (unless running from Minotaur)
	KEY0+KEY3,								// Loc 40: East, West (only after Minotaur leaves & player investigates)
	KEY0,											// Loc 41: East
	KEY0+KEY1+KEY2+KEY3,			// Loc 42: All directions
	KEY0+KEY3,								// Loc 43: East, West
	KEY1+KEY2+KEY3,						// Loc 44: North, South, West
	KEY1+KEY2,								// Loc 45: North, South
	KEY0+KEY1+KEY2,						// Loc 46: North, East, South
	KEY0+KEY3,								// Loc 47: East, West
	KEY0+KEY3,								// Loc 48: East, West
	KEY1+KEY3,								// Loc 49: South, West
	KEY1,											// Loc 50: South (unless taking secret Minotaur tunnel)
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
	KEY0,											// Loc 72: East (unless Key has been retrieved)
	KEY0+KEY3,								// Loc 73: East, West
	KEY0+KEY3,								// Loc 74: East, West
	KEY0+KEY3,								// Loc 75: East, West
	KEY0+KEY3,								// Loc 76: East, West
	KEY0+KEY2+KEY3,						// Loc 77: North, East, West
	KEY3,											// Loc 78: West
	0,												// Loc 79: Not a valid spot
	KEY2,											// Loc 80: North
			// ACTION SPOTS
	KEY0+KEY1,								// 81; Action spot for Loc 6 (first time only): East, South
	KEY1+KEY2,								// 82; 1st Action spot for Loc 35 (first time only): North (pick up), South (leave)
	KEY1,											// 83; 2nd Action spot for Loc 35 (after pick up only): South (leave)
	KEY2+KEY3,								// 84; 1st Action spot for Loc 40 (first time only): North (fight), west (run)
	KEY0,											// 85; 2nd Action spot for Loc 40 (fight & win): East (grab key & leave)
	KEY0,											// 86; 3rd Action spot for Loc 40 (run & return): East (grab key & leave)
	KEY3,											// 87; Action spot for Loc 39 (after running only, once): West (keep running!)
	KEY1,											// 88; Action spot for Loc 50 (only after fighting & taking secret exit): South
	KEY1,											// 89; Action spot for Loc 72 (Only after getting key): South (to win!)
};

// lut for what message to display at each location
const char LUT_location_msg [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
  "You awake in a pitch black room of cold, hard stone. You hear an oddly         familiar growl in the distance...",  			// Loc 0: Start Screen
	"You continue down a dark hallway.                                              The only way to go is forward.",  				// Loc 1
	"The hallway continues, no end in sight. You see writing on the wall ahead...",																						// Loc 2
	"You continue, and the footsteps stop abruptly. On the wall reads               'None shall escape the Labyrinth'.",				// Loc 3
	"The hallway continues.",																																																	// loc 4
	"Finally, the hallway ends. There's a simple wooden door, with a latch on it.",			// Loc 5
	"You're back at the locked door from which you came... still locked.",							// Loc 6
	"You're north of a rotten pillar... god does it stink.",														// Loc 7
	"You're in a corner, behind the moldy pillar.",																			// Loc 8
	"Error: Loc 9 is an invalid location",																							// Loc 9 (not a valid spot)
	"The Shrine appears to depict a fierce golden bull creature, wrapped in chains.",		// Loc 10
	"The corridor continues to twist & turn.",																					// Loc 11
	"You continue along another corner of the corridor.",																// Loc 12
	"To the North, a painting depicting nothing but a dreadful pair of eyes...",				// Loc 13
	"You're in a twisting corridor.",																										// Loc 14
	"You're in a short hallway. There's a dank pillar to the East.",										// Loc 15
	"Error: Loc 16 is an invalid location",																							// Loc 16 (not a valid spot)
	"You're behind the pillar. To the West, you see 'Only a sword will Slay         the Beast' inscribed on the wall... that's definitely not red paint...",				// Loc 17
	"A long dark hallway is to the South, and multiple corridors to the West.",					// Loc 18
	"The corridor splits, opening in all directions. But to the North, you spot     what looks like a weird glowing Shrine...",					// Loc 19
	"You're at the end of the corridor, which opens up North of you.",									// Loc 20
	"The corridor continues.",																													// Loc 21
	"You make another turn within the corridor. More rattling chains.",									// Loc 22
	"You're at the start of a twisting corridor, which opens up North of you.",					// Loc 23
	"The Labyrinth opens up in all directions.",																				// Loc 24
	"You're in a tight passage way between a wall, and the pillar North of you.",				// Loc 25
	"You're in a corner, behind the pillar.",																						// Loc 26
	"You're in the long dark hallway.",																									// Loc 27
	"A long dark hallway continues East of you, but Jesus does it stink.",							// Loc 28
	"The stench lingers, and the hallway continues.",																		// Loc 29
	"Christ, what could that smell possibly be?!",																			// Loc 30
	"The corridor continues West of you, but ends to the East.",												// Loc 31
	"You stumble upon what looks like the Triceratops poop from Jurassic Park...    You hear Jeff Goldblum's 'That's one big pile of shit' in your head.",		// Loc 32
	"You continue through a narrow hallway, but jesus does something smell awful.",			// Loc 33
	"Error: Loc 34 is an invalid location",																							// Loc 34
	"Something must have ripped the Knight's head off... ",															// Loc 35
	"As the hallway continues, a painting of what looks like the apocalypse         is hung on the Eastern wall. ",		// Loc 36
	"You're at a corner which opens to the East & South.",															// Loc 37
	"You're in a hallway, and there are bones riddled everywhere...",										// Loc 38
	"There's some sort of Den to the East, and a corridor to the West.",									// Loc 39
	"The Minotaur's Den reeks of death.",																								// Loc 40
	"It's a dead end.",		// Loc 41
	"The Labyrinth opens in all directions again.",		// Loc 42
	"You walk along a short corridor.",		// Loc 43
	"There's a dead body on the floor North of you...",		// Loc 44
	"The corridor continues running North & South.",		// Loc 45
	"Corridors open to the North & West. A nook opens up to the South.",		// Loc 46
	"You step in something sticky... gross.",		// Loc 47
	"You hear heavy breathing North of you, but see nothing in the corridor...",		// Loc 48
	"The Labyrinth bends 90 degrees.",		// Loc 49
	"It's a dead end, with nothing but dirt and mold.",		// Loc 50
	"This hallway stinks. It continues North & South",		// Loc 51
	"You come across a heaping pile of rotten bones... some of them look fresh...",		// Loc 52
	"The labyrinth opens to the North, and curves around a corner to the South.",		// Loc 53
	"The corridor continues.",		// Loc 54
	"It's a dead end, but there's some mysterious green slime on the walls...       Is it moving?!",		// Loc 55
	"Error: Loc 56 is an invalid location",																							// Loc 56
	"Error: Loc 57 is an invalid location",																							// Loc 57
	"A corridor continues to the North, and the labyrinth opens in all directions   to the South.",		// Loc 58
	"A dead end lingers to the North.",																									// Loc 59
	"The hallway continues to the North, and opens up to the South",		// Loc 60
	"Something smells seriously dreadful...",		// Loc 61
	"Okay, who farted? It reeks around this corner.",		// Loc 62
	"You find yourself in a corner, With dark, endless corridors spilling out to    the North & East.",		// Loc 63
	"There corridor ends to the West, but continues without an end in sight         to the East.",		// Loc 64
	"You slash your way through some cobwebs, continuing down the corridor.",		// Loc 65
	"The corridor continues.",		// Loc 66
	"The Labyrinth opens to the North, and continues running East/West.",		// Loc 67
	"Corridor opens, all directions are available.",		// Loc 68
	"A long, endless corridor opens to the North. Can also go East / West",		// Loc 69
	"The corridor continues to the East, around a corner.",		// Loc 70
	"To the South, you spot some cracks in the wall. It's bright on the other side, and super hot.",		// Loc 71
	"There's a door! You can hear birds chirping on the other side!                 But damn, it's locked!",		// Loc 72
	"There's a door to the West!",																											// Loc 73
	"There appears to be a light source up ahead!",																			// Loc 74
	"This hallway smells like all the rest... shit.",																		// Loc 75
	"The corridor continues.",		// Loc 76
	"The corridor continues to the West, opens to the North, and has a dead-end     to the East.",		// Loc 77
	"It's a dead-end.",		// Loc 78
	"Error: Loc 34 is an invalid location",		// Loc 79
	"You look through the cracks and see nothing but fire, as if peering into a     furnance. You swear you hear screams from within...",		// Loc 80
	// Action Locations
	"You close the door behind you, and immediately hear it be aggresively locked.  You try to open it, but it's barred from the other side...",		// 81; Action spot for loc 6
	"This guy must've been some sort of Knight, before he lost his head...          He's got something strapped to his belt... it looks like a Sword!",		// 82; 1st Action spot for loc 35
	"The Sword fits perfectly in your hand, and has a slight glow to it.            It doesn't have even a drop of blood on it...",									// 83; 2nd Action spot for loc 35
	"You walked right into the Minotaur's Den! It gets up, ready to attack!",						// 84; 1st Action spot for loc 40
	"You have slain the Minotaur!! And there's a Key in the back of his Den!        And it looks like there's some sort of secret passage...",			// 85; 2nd Action spot for loc 40
	"The Minotaur isn't here.. for now. But there's a Key in the back of his Den!   And it looks like there's some sort of secret passage...",			// 86; 3rd Action spot for loc 40
	"The Minotaur is right behind you!! Don't stop!!!",																																															// 87; Action spot for loc 39
	"You exit the secret tunnel at the end of a corridor. South is the only option.",																																// 88; Action spot for loc 50
	"The key looks the right size! Quick, escape!!"																																																	// 89; Action spot for loc 72
};

// lut for the main question to ask at each location
const char LUT_location_question [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
  "You fumble around in the dark and find a compass, a candle, and                some matchsticks. Will you light the canlde & begin your journey?",  // Loc 0: Start screen
	"Continue forward?",  // Loc 1
	"Continue forward?",  // Loc 2
	"Continue forward?",  // Loc 3
	"Continue forward?",  // Loc 4
	"Unlatch the door and continue forward?",  // Loc 5
	"Continue forward?",  // Loc 6
	"Continue forward?",  // Loc 7
	"Which direction will you go?",	 // Loc 8
	" ",								// Loc 9 (invalid location)
	"Go back to exploring the Labyrinth?",		// Loc 10
	"Which direction will you go?",		// Loc 11
	"Which direction will you go?",		// Loc 12
	"Which direction will you go?",		// Loc 13
	"Continue along the corridor? Or turn back?",		// Loc 14
	"North or South?",		// Loc 15
	" ",								// Loc 16 (invalid location)
	"Should take note of that... which way now?",		// Loc 17
	"Which way?",		// Loc 18
	"Which direction will you go?",		// Loc 19
	"North or West?",		// Loc 20
	"North or East?",		// Loc 21
	"Which way?",		// Loc 22
	"Go into the corridor?",		// Loc 23
	"Which direction?",		// Loc 24
	"East or West?",		// Loc 25
	"Go North around the Pillar?",		// Loc 26
	"Go South into the corridor? Or North?",		// Loc 27
	"Go East into the dark, stinky hallway? Or nah?",		// Loc 28
	"Continue through the corridor?",		// Loc 29
	"East or West?",		// Loc 30
	"Continue East to the end of the corridor? Or exit to the West?",		// Loc 31
	"Leave the pile of shit behind?",		// Loc 32
	"North? Or South?",		// Loc 33
	" ",								// Loc 34 (invalid location)
	"Leave the corpse?",		// Loc 35
	"North or South?",		// Loc 36
	"East, or South?",		// Loc 37
	"Continue East, down the hallway? Or go back West?",		// Loc 38
	"East into the Den? Or West?",		// Loc 39
	"Crawl through Secret Tunnel? Or go back West?",		// Loc 40
	"Leave dead-end?",		// Loc 41
	"Which way?",		// Loc 42
	"East or West?",		// Loc 43
	"Investigate the body? Or go another direction?",		// Loc 44
	"Continue along the corridor?",		// Loc 45
	"North, East, or South?",		// Loc 46
	"Continue?",		// Loc 47
	"East around the corner, or West down the corridor?",		// Loc 48
	"South, or West?",		// Loc 49
	"Leave dead-end?",		// Loc 50
	"Continue?",		// Loc 51
	"Leave bones behind?",		// Loc 52
	"Which way?",		// Loc 53
	"North, down the endless hallway? Or South to the corner?",		// Loc 54
	"Go North?",		// Loc 55
	" ",								// Loc 56 (invalid location)
	" ",								// Loc 57 (invalid location)
	"North around corner? Or South into the open area?",		// Loc 58
	"North to dead-end? Or South?",		// Loc 59
	"Which way?",		// Loc 60
	"Examine source of smell to the North?",		// Loc 61
	"North, or West?",		// Loc 62
	"North hallway or East hallway?",		// Loc 63
	"East or West?",		// Loc 64
	"Continue?",		// Loc 65
	"East or West?",		// Loc 66
	"Which way?",		// Loc 67
	"Enter corridor to the South? Or go another direction?",		// Loc 68
	"North to where you began? Or East / West?",		// Loc 69
	"East or West?",		// Loc 70
	"Return West? Or examine cracks in the wall?",		// Loc 71
	"Return East to go find key?",		// Loc 72
	"East along corridor? Or West towards door?",		// Loc 73
	"Continue East? Or West towards light source?",		// Loc 74
	"East or West?",		// Loc 75
	"East or West?",		// Loc 76
	"North to exit corridor? Or another direction?",		// Loc 77
	"Leave dead-end?",		// Loc 78
	" ",		// Loc 79 (invalid location)
	"Stop examining cracks in wall?",		// Loc 80
	// Action Locations
	"You don't seem to have an option of going back... which way will you go now?",		// Loc 81
	"Take Sword off the dead Knight?",		// Loc 82
	"Continue onwards, Sword in hand?",		// Loc 83
	"Will you fight, and risk death? Or run away?",		// Loc 84
	"Take key and escape through secret tunnel?",		// Loc 85
	"Take key and escape through secret tunnel?",		// Loc 86
	"KEEP RUNNING!",		// Loc 87
	"Continue South towards the exit?",		// Loc 88
	"Insert Key and escape the Labyrinth??",		// Loc 89
};

// TODO: modify directional options to be more "fluid" yeno
// lut for the text associated with the North option (if applicable)
const char LUT_loc_north_option [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
	" ",								// Loc 0: Start screen (no north option)
	" ",								// Loc 1: no north option
	" ",								// Loc 2: no north option
	" ",								// Loc 3: no north option
	" ",								// Loc 4: no north option
	" ",								// Loc 5: no north option
	" ",								// Loc 6: no north option
	" ",								// Loc 7: no north option
	" ",								// Loc 8: no north option
	" ",								// Loc 9: no north option (invalid spot)
	" ",								// Loc 10: no north option
	" ",								// Loc 11: no north option
	" ",								// Loc 12: no north option
	" ",								// Loc 13: no north option
	" ",								// Loc 14: no north option
	"KEY2: Go North",		// Loc 15
	" ",								// Loc 16: no north option (invalid spot)
	"KEY2: Go North",		// Loc 17
	" ",								// Loc 18: no north option
	"KEY2: Go North",		// Loc 19
	"KEY2: Go North",		// Loc 20
	"KEY2: Go North",		// Loc 21
	"KEY2: Go North",		// Loc 22
	"KEY2: Go North",		// Loc 23
	"KEY2: Go North",		// Loc 24
	" ",								// Loc 25: no north option
	"KEY2: Go North",		// Loc 26
	"KEY2: Go North",		// Loc 27
	"KEY2: Go North",		// Loc 28
	" ",								// Loc 29: no north option
	" ",								// Loc 30: no north option
	" ",								// Loc 31: no north option
	" ",								// Loc 32: no north option
	"KEY2: Go North",		// Loc 33
	" ",								// Loc 34: no north option (invalid spot)
	" ",								// Loc 35: no north option
	"KEY2: Go North",		// Loc 36
	" ",								// Loc 37: no north option
	" ",								// Loc 38: no north option
	" ",								// Loc 39: no north option
	" ",								// Loc 40: no north option
	" ",								// Loc 41: no north option
	"KEY2: Go North",		// Loc 42
	" ",								// Loc 43: no north option
	"KEY2: Go North to examine body",		// Loc 44
	"KEY2: Go North",		// Loc 45
	"KEY2: Go North",		// Loc 46
	" ",								// Loc 47: no north option
	" ",								// Loc 48: no north option
	" ",								// Loc 49: no north option
	" ",								// Loc 50: no north option
	"KEY2: Go North",		// Loc 51
	" ",								// Loc 52: no north option
	"KEY2: Go North",		// Loc 53
	"KEY2: Go North",		// Loc 54
	"KEY2: Go North",		// Loc 55
	" ",								// Loc 56: no north option (invalid location)
	" ",								// Loc 57: no north option (invalid location)
	"KEY2: Go North",		// Loc 58
	"KEY2: Go North",		// Loc 59
	"KEY2: Go North",		// Loc 60
	"KEY2: Go North",		// Loc 61
	"KEY2: Go North",		// Loc 62
	"KEY2: Go North",		// Loc 63
	" ",								// Loc 64: no north option
	" ",								// Loc 65: no north option
	" ",								// Loc 66: no north option
	"KEY2: Go North",		// Loc 67
	"KEY2: Go North",		// Loc 68
	"KEY2: Go North",		// Loc 69
	" ",								// Loc 70: no north option
	" ",								// Loc 71: no north option
	" ",								// Loc 72: no north option
	" ",								// Loc 73: no north option
	" ",								// Loc 74: no north option
	" ",								// Loc 75: no north option
	" ",								// Loc 76: no north option
	"KEY2: Go North",		// Loc 77
	" ",								// Loc 78: no north option
	" ",								// Loc 79: no north option (invalid location)
	"KEY2: Go North",		// Loc 80
		// ACTION SPOTS
	" ",								// 81: Action for loc 6; no north option
	"KEY2: Pick Up Sword!",		// 82: Action for loc 35
	" ",											// 83: no north option
	"KEY2: Fight!",						// 84: 1st Action for loc 40
	" ",								// 85: 2nd Action for loc 40; no north option
	" ",								// 86: 3rd Action for loc 40; no north option
	" ",								// 87: Action for loc 39; no north option
	" ",								// 88: Action for loc 50; no north option
	" "									// 89: Action for loc 72; no north option
};

// lut for the text associated with the East option (if applicable)
const char LUT_loc_east_option [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
	"KEY0: Start Journey...",			// Loc 0: Start screen
	"KEY0: Continue East",				// Loc 1
	"KEY0: Continue East",				// Loc 2
	"KEY0: Continue East",				// Loc 3
	"KEY0: Continue East",				// Loc 4
	"KEY0: Go East through door",				// Loc 5
	"KEY0: Go East",		// Loc 6
	"KEY0: Go East",		// Loc 7
	" ",								// Loc 8: no east option
	" ",								// Loc 9: no east option (invalid spot)
	" ",								// Loc 10: no east option
	"KEY0: Go East",		// Loc 11
	" ",								// Loc 12: no east option
	"KEY0: Go East",		// Loc 13
	" ",								// Loc 14: no east option
	" ",								// Loc 15: no east option
	" ",								// Loc 16: no east option (invalid spot)
	" ",								// Loc 17: no east option
	"KEY0: Go East",		// Loc 18
	"KEY0: Go East",		// Loc 19
	" ",								// Loc 20: no east option
	"KEY0: Go East",		// Loc 21
	" ",								// Loc 22: no east option
	"KEY0: Go East",		// Loc 23
	"KEY0: Go East",		// Loc 24
	"KEY0: Go East",		// Loc 25
	" ",								// Loc 26: no east option
	" ",								// Loc 27: no east option
	"KEY0: Go East",		// Loc 28
	"KEY0: Go East",		// Loc 29
	"KEY0: Go East",		// Loc 30
	"KEY0: Go East",		// Loc 31
	" ",								// Loc 32: no east option
	" ",								// Loc 33: no east option
	" ",								// Loc 34: no east option (invalid location)
	" ",								// Loc 35: no east option
	" ",								// Loc 36: no east option
	"KEY0: Go East",		// Loc 37
	"KEY0: Go East",		// Loc 38
	"KEY0: Go East",		// Loc 39
	"KEY0: Crawl Through Secret tunnel",		// Loc 40
	"KEY0: Go East",		// Loc 41
	"KEY0: Go East",		// Loc 42
	"KEY0: Go East",		// Loc 43
	" ",								// Loc 44: no east option
	" ",								// Loc 45: no east option
	"KEY0: Go East",		// Loc 46
	"KEY0: Go East",		// Loc 47
	"KEY0: Go East",		// Loc 48
	" ",								// Loc 49: no east option
	" ",								// Loc 50: no east option
	" ",								// Loc 51: no east option
	" ",								// Loc 52: no east option
	" ",								// Loc 53: no east option
	" ",								// Loc 54: no east option
	" ",								// Loc 55: no east option
	" ",								// Loc 56: no east option (invalid location)
	" ",								// Loc 57: no east option (invalid location)
	" ",								// Loc 58: no east option
	" ",								// Loc 59: no east option
	" ",								// Loc 60: no east option
	"KEY0: Go East",		// Loc 61
	" ",								// Loc 62: no east option
	"KEY0: Go East",		// Loc 63
	"KEY0: Go East",		// Loc 64
	"KEY0: Go East",		// Loc 65
	"KEY0: Go East",		// Loc 66
	"KEY0: Go East",		// Loc 67
	"KEY0: Go East",		// Loc 68
	"KEY0: Go East",		// Loc 69
	"KEY0: Go East",		// Loc 70
	" ",								// Loc 71: no east option
	"KEY0: Go East",		// Loc 72
	"KEY0: Go East",		// Loc 73
	"KEY0: Go East",		// Loc 74
	"KEY0: Go East",		// Loc 75
	"KEY0: Go East",		// Loc 76
	"KEY0: Go East",		// Loc 77
	" ",								// Loc 78: no east option
	" ",								// Loc 79: no east option
	" ",								// Loc 80: no east option
		// ACTION SPOTS
	"KEY0: Go East",		// 81: Action for loc 6
	" ",								// 82: 1st Action for loc 35; no east option
	" ",								// 83: 2nd Action for loc 35; no east option
	" ",								// 84: 1st Action for loc 40; no east option
	"KEY0: Grab Key, Crawl into Tunnel",		// 85: 2nd Action for loc 40
	"KEY0: Grab Key, Crawl into Tunnel",		// 86: 3rd Action for loc 40
	" ",								// 87: 1st Action for loc 32; no east option
	" ",								// 88: Action for loc 50; no east option
	" "									// 89: Action for loc 72; no east option
};

// lut for the text associated with the South option (if applicable)
const char LUT_loc_south_option [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
	" ",								// Loc 0: Start screen (no south option)
	" ",								// Loc 1: no south option
	" ",								// Loc 2: no south option
	" ",								// Loc 3: no south option
	" ",								// Loc 4: no south option
	" ",								// Loc 5: no south option
	"KEY1: Go South",		// Loc 6
	" ",								// Loc 7: no south option
	"KEY1: Go South",		// Loc 8
	" ",								// Loc 9: no south option (invalid spot)
	"KEY1: Go South",		// Loc 10
	"KEY1: Go South",		// Loc 11
	"KEY1: Go South",		// Loc 12
	"KEY1: Go South",		// Loc 13
	"KEY1: Go South",		// Loc 14
	"KEY1: Go South",		// Loc 15
	" ",								// Loc 16: no south option (invalid spot)
	"KEY1: Go South",		// Loc 17
	"KEY1: Go South",		// Loc 18
	"KEY1: Go South",		// Loc 19
	" ",								// Loc 20: no south option
	" ",								// Loc 21: no south option
	" ",								// Loc 22: no south option
	" ",								// Loc 23: no south option
	"KEY1: Go South",		// Loc 24
	" ",								// Loc 25: no south option
	" ",								// Loc 26: no south option
	"KEY1: Go South",		// Loc 27
	" ",								// Loc 28: no south option
	" ",								// Loc 29: no south option
	" ",								// Loc 30: no south option
	" ",								// Loc 31: no south option
	" ",								// Loc 32: no south option
	"KEY1: Go South",		// Loc 33
	" ",								// Loc 34: no south option (invalid spot)
	"KEY1: Go South",		// Loc 35
	"KEY1: Go South",		// Loc 36
	"KEY1: Go South",		// Loc 37
	" ",								// Loc 38: no south option
	" ",								// Loc 39: no south option
	" ",								// Loc 40: no south option
	" ",								// Loc 40: no south option
	"KEY1: Go South",		// Loc 42
	" ",								// Loc 43: no south option
	"KEY1: Go South",		// Loc 44
	"KEY1: Go South",		// Loc 45
	"KEY1: Go South",		// Loc 46
	" ",								// Loc 47: no south option
	" ",								// Loc 48: no south option
	"KEY1: Go South",		// Loc 49
	"KEY1: Go South",		// Loc 50
	"KEY1: Go South",		// Loc 51
	"KEY1: Go South",		// Loc 52
	"KEY1: Go South",		// Loc 53
	"KEY1: Go South",		// Loc 54
	" ",								// Loc 55: no south option
	" ",								// Loc 56: no south option (invalid spot)
	" ",								// Loc 57: no south option (invalid spot)
	"KEY1: Go South",		// Loc 58
	"KEY1: Go South",		// Loc 59
	"KEY1: Go South",		// Loc 60
	" ",								// Loc 61: no south option
	" ",								// Loc 62: no south option
	" ",								// Loc 63: no south option
	" ",								// Loc 64: no south option
	" ",								// Loc 65: no south option
	" ",								// Loc 66: no south option
	" ",								// Loc 67: no south option
	"KEY1: Go South",		// Loc 68
	" ",								// Loc 69: no south option
	" ",								// Loc 70: no south option
	"KEY1: Examine Cracks",		// Loc 71
	" ",								// Loc 72: no south option
	" ",								// Loc 73: no south option
	" ",								// Loc 74: no south option
	" ",								// Loc 75: no south option
	" ",								// Loc 76: no south option
	" ",								// Loc 77: no south option
	" ",								// Loc 78: no south option
	" ",								// Loc 79: no south option
	" ",								// Loc 80: no south option
		// ACTION SPOTS
		"KEY1: Go South",		// 81: Action for loc 6
		"KEY1: Leave Sword Behind",		// 82: 1st Action for loc 35
		"KEY1: Go South, Sword in hand",							// 83: 2nd Action for loc 35
		" ",								// 84: 1st Action for loc 40; no south option
		" ",								// 85: 2nd Action for loc 40; no south option
		" ",								// 86: 3rd Action for loc 40; no south option
		" ",								// 87: Action for loc 39; no south option
		"KEY1: Go South",		// 88: Action for loc 50
		"KEY1: ESCAPE THE LABYRINTH"		// 89: Action for loc 72
};

// lut for the text associated with the West option (if applicable)
const char LUT_loc_west_option [MAX_LOCATIONS][VGA_TEXT_MAX_SIZE] = {
	" ",								// Loc 0: Start screen (no west option)
	" ",								// Loc 1: no west option
	" ",								// Loc 2: no west option
	" ",								// Loc 3: no west option
	" ",								// Loc 4: no west option
	" ",								// Loc 5: no west option
	" ",								// Loc 6: no west option
	"KEY3: Go West",		// Loc 7
	"KEY3: Go West",		// Loc 8
	" ",								// Loc 9: no west option (invalid spot)
	" ",								// Loc 10: no west option
	" ",								// Loc 11: no west option
	"KEY3: Go West",		// Loc 12
	" ",								// Loc 13: no west option
	"KEY3: Go West",		// Loc 14
	" ",								// Loc 15: no west option
	" ",								// Loc 16: no west option (invalid spot)
	" ",								// Loc 17: no west option
	" ",								// Loc 18: no west option
	"KEY3: Go West",		// Loc 19
	"KEY3: Go West",		// Loc 20
	" ",								// Loc 21: no west option
	"KEY3: Go West",		// Loc 22
	" ",								// Loc 23: no west option
	"KEY3: Go West",		// Loc 24
	"KEY3: Go West",		// Loc 25
	"KEY3: Go West",		// Loc 26
	" ",								// Loc 27: no west option
	" ",								// Loc 28: no west option
	"KEY3: Go West",		// Loc 29
	"KEY3: Go West",		// Loc 30
	"KEY3: Go West",		// Loc 31
	"KEY3: Go West",		// Loc 32
	" ",								// Loc 33: no west option
	" ",								// Loc 34: no west option (invalid spot)
	" ",								// Loc 35: no west option
	" ",								// Loc 36: no west option
	" ",								// Loc 37: no west option
	"KEY3: Go West",		// Loc 38
	"KEY3: Go West",		// Loc 39
	"KEY3: Go West",		// Loc 40
	" ",								// Loc 41: no west option
	"KEY3: Go West",		// Loc 42
	"KEY3: Go West",		// Loc 43
	"KEY3: Go West",		// Loc 44
	" ",								// Loc 45: no west option
	" ",								// Loc 46: no west option
	"KEY3: Go West",		// Loc 47
	"KEY3: Go West",		// Loc 48
	"KEY3: Go West",		// Loc 49
	" ",								// Loc 50: no west option
	" ",								// Loc 51: no west option
	" ",								// Loc 52: no west option
	" ",								// Loc 53: no west option
	" ",								// Loc 54: no west option
	" ",								// Loc 55: no west option
	" ",								// Loc 56: no west option (invalid spot)
	" ",								// Loc 57: no west option (invalid spot)
	" ",								// Loc 58: no west option
	" ",								// Loc 59: no west option
	" ",								// Loc 60: no west option
	" ",								// Loc 61: no west option
	"KEY3: Go West",		// Loc 62
	" ",								// Loc 63: no west option
	"KEY3: Go West",		// Loc 64
	"KEY3: Go West",		// Loc 65
	"KEY3: Go West",		// Loc 66
	"KEY3: Go West",		// Loc 67
	"KEY3: Go West",		// Loc 68
	"KEY3: Go West",		// Loc 69
	"KEY3: Go West",		// Loc 70
	"KEY3: Go West",		// Loc 71
	" ",								// Loc 72: no west option
	"KEY3: Go West",		// Loc 73
	"KEY3: Go West",		// Loc 74
	"KEY3: Go West",		// Loc 75
	"KEY3: Go West",		// Loc 76
	"KEY3: Go West",		// Loc 77
	"KEY3: Go West",		// Loc 78
	" ",								// Loc 79: no west option (invalid spot)
	" ",								// Loc 80: no west option
		// ACTION SPOTS
		" ",								// 81: Action for loc 32; no west option
		" ",								// 82: 1st Action for loc 35; no west option
		" ",								// 83: 2nd Action for loc 35; no west option
		"KEY3: RUN!",				// 84: 1st Action for loc 40
		" ",								// 85: 2nd Action for loc 40; no west option
		"KEY3: Go West",		// 86: 3rd Action for loc 40
		"KEY3: KEEP RUNNING",		// 87: Action for loc 39
		" ",								// 88: Action for loc 50; no west option
		" ",								// 89: Action for loc 72; no west option
};


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
	int y_base = 144;
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

	int row = 0;
	// Draw the triangle
	for (int y = (y_base+21); y > (y_base+12); y--) {
		for (int x = (x_base-(row))+1; x < (x_base+(row))+3; x++) {
			VGA_pixel(x, y, pixel_color);
		}
		row++;
	}
}

void VGA_east_arrow() {
	// draws a east (right-wards) facing arrow
	int x_base = 192;
	int y_base = 181;
	short pixel_color = 0xFFFF;		// white

	// draw the center line
	for (int y = y_base; y < (y_base+4); y++) {
		for (int x = x_base; x < (x_base+20); x++) {
			VGA_pixel(x, y, pixel_color);
		}
	}

	int row = 0;
	// Draw the triangle
	for (int x = x_base+21; x > (x_base+12); x--) {
		for (int y = (y_base-row)+1; y < (y_base+row)+3; y++) {
			VGA_pixel(x, y, pixel_color);
		}
		row++;
	}
}

void VGA_west_arrow() {
	// draws a west (left-wards) facing arrow
	int x_base = 92;
	int y_base = 181;
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
	if (loc < 0) {
		// Game is over (either won or lost)
		VGA_south_arrow();
		VGA_text(SOUTH_MSG_BASE_X, SOUTH_MSG_BASE_Y, "KEY1: Reset Game");

	} else {
		// Go through each direction and, if it's allowed, display the arrow & option msg
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

		if (LUT_location_permissions[loc] & KEY2) {
			// North
			VGA_north_arrow();
			VGA_text(NORTH_MSG_BASE_X, NORTH_MSG_BASE_Y, LUT_loc_north_option[loc]);
		}

		if (LUT_location_permissions[loc] & KEY3) {
			// West
			VGA_west_arrow();
			VGA_text(WEST_MSG_BASE_X, WEST_MSG_BASE_Y, LUT_loc_west_option[loc]);
		}
	}

}


/* Definition of Tasks */

void TaskStartScreen(void* pdata) {
	// Initial (Idle) task
	// Needs to stay until KEY0 is pressed
	// Can only run in idle state; then after running once, pends until GAME_RESET is high

	int seed_val = OSTimeGet(); // for random number generation

	while(1) {

		// Don't run until active / finished states are completed/inactive
		value = OSFlagPend(GameStatus, GAME_ACTIVE + GAME_FINISHED, OS_FLAG_WAIT_CLR_ALL, 0, &err);
		value = OSFlagPost(GameStatus, GAME_RESET, OS_FLAG_CLR, &err); // disable RESET flag

		// Clear VGA before displaying new location
		VGA_clear();
		VGA_text_clear();

		// increment seed_val, reseed program for rand var
		seed_val = OSTimeGet();
		srand(seed_val);

		// Reset Global Game Vars
		location = 43; // TODO:
		step_count = 0;
		time_250ms = 0;
    step_time_rem_SS = 40;		// TODO: change back
		max_step_time_rem = step_time_rem_SS;
    tot_time_rem_SS = 59;
    tot_time_rem_MM = 9;
		tot_time_SS = 0;
		tot_time_MM = 0;

		// display initial message
		VGA_text(MSG_BASE_X, MSG_BASE_Y, LUT_location_msg[location]);

		// display initial question
		VGA_text(QUESTION_BASE_X, QUESTION_BASE_Y, LUT_location_question[location]);

		// display start game msg & options
		VGA_disp_options(location);

		// Wait until game is reset until reseting & idling again
		value = OSFlagPend(GameStatus, GAME_RESET, OS_FLAG_WAIT_SET_ANY, 0, &err);

		OSTimeDly(2);

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
		} else if (KEY_val == KEY3) {				// KEY3
			KEY0_flag = 0;
			KEY1_flag = 0;
			KEY2_flag = 0;
			KEY3_flag = 1;
		}


		// Read Status of GameStatus Flags
		flags = OSFlagQuery(GameStatus, &err);

		// State driver - Dependent on what state we're currently in
		if (!(flags & GAME_ACTIVE) && !(flags && GAME_LOST) && !(flags && GAME_FINISHED) && !(flags && GAME_RESET)) {
			// IDLE state (no active GameStatus flags)

			if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
				KEY0_flag = 0;
				location += 1;
				step_count++;
				value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_NEW_LOCATION, OS_FLAG_SET, &err);
			}


		} else if (flags & GAME_RESET) {
			// GAME RESET state

			// Reset event flags to default settings
			value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_NEW_LOCATION + GAME_FINISHED + GAME_LOST, OS_FLAG_CLR, &err);


		} else if (flags & GAME_FINISHED) {
			// GAME FINISHED state

			// disable active game state flags
			value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_NEW_LOCATION, OS_FLAG_CLR, &err);

			if  ( !(KEY_val & KEY1) && (KEY1_flag) ) {
				// Reset game
				KEY1_flag = 0;
				value = OSFlagPost(GameStatus, GAME_RESET, OS_FLAG_SET, &err);
			}


		} else if (flags & GAME_LOST) {
			// GAME LOST state

			// disable active game state flags
			value = OSFlagPost(GameStatus, GAME_ACTIVE + GAME_NEW_LOCATION, OS_FLAG_CLR, &err);

			if  ( !(KEY_val & KEY1) && (KEY1_flag) ) {
				// Reset game
				KEY1_flag = 0;
				value = OSFlagPost(GameStatus, GAME_RESET, OS_FLAG_SET, &err);
			}


		} else if ( (flags & GAME_ACTIVE) && (!(flags & GAME_NEW_LOCATION)) ) {
			// ACTIVE state (NOT rendering new location, i.e. taking input)

			if (location == 5) {
					if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
						// KEY0 press - Go East to loc 81 (action spot preceeding loc 6)
						KEY0_flag = 0;
						location = 81;
						step_time_rem_SS = max_step_time_rem;
						time_250ms = 0;
						step_count++;
						value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
					}

			} else if (location == 39) {
				if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
					// KEY0 press - go East into Minotaur's Den
					KEY0_flag = 0;
					if (minotaur_gone_flag && key_flag) {
						location = 40;
					} else if (minotaur_gone_flag && (!key_flag)) {
						location = 86;
					} else {
						location = 84;	// encounter Minotaur for first time
						max_step_time_rem = (max_step_time_rem / 4);
					}
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				} else if ( !(KEY_val & KEY3) && (KEY3_flag) ) {
					// KEY3 press - Typically means go West (leftwards)
					KEY3_flag = 0;
					location = 38;		// increment place in map by 9
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 40) {
				if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
					// KEY0 press - Travel through secret tunnel to location 50
					KEY0_flag = 0;
					location = 88;
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				} else if ( !(KEY_val & KEY3) && (KEY3_flag) ) {
					// KEY3 press - Typically means go West (leftwards)
					KEY3_flag = 0;
					location = 39;		// increment place in map by 9
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 44) {
				if ( !(KEY_val & KEY1) && (KEY1_flag) ) {
					// KEY1 press - Typically means go South (downwards)
					KEY1_flag = 0;
					location = 53;		// increment place in map by 9
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				} else if ( !(KEY_val & KEY2) && (KEY2_flag) ) {
					// KEY2 press - go to location of sword
					KEY2_flag = 0;
					if (sword_flag) {
						location = 35;		// go to location where sword used to be (already picked it up)
					} else {
						location = 82;		// go to location w/ sword on floor
					}
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				} else if ( !(KEY_val & KEY3) && (KEY3_flag) ) {
					// KEY3 press - Typically means go West (leftwards)
					KEY3_flag = 0;
					location = 43;		// increment place in map by 9
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 73) {
				if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
					// KEY0 press - Typically means go East (rightwards)
					KEY0_flag = 0;
					location = 74;
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				} else if ( !(KEY_val & KEY3) && (KEY3_flag) ) {
					// KEY3 press - Typically means go West (leftwards)
					KEY3_flag = 0;
					if (key_flag) {
						location = 89;
					} else {
						location = 72;
					}
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 81) {
				if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
					// KEY0 press - Typically means go East (rightwards)
					KEY0_flag = 0;
					location = 7;		// increment place in map by 1 (81 is 6's action spot)
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				} else if ( !(KEY_val & KEY1) && (KEY1_flag) ) {
					// KEY1 press - Typically means go South (downwards)
					KEY1_flag = 0;
					location = 15;		// increment place in map by 9
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 82) {
				if ( !(KEY_val & KEY1) && (KEY1_flag) ) {
					// KEY1 press - Typically means go South (downwards)
					KEY1_flag = 0;
					location = 44;		// increment place in map by 9
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				} else if ( !(KEY_val & KEY2) && (KEY2_flag) ) {
					// KEY2 press - take sword
					KEY1_flag = 0;
					sword_flag = 1;		// sword is taken
					location = 83;
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 83) {
				if ( !(KEY_val & KEY1) && (KEY1_flag) ) {
					// KEY1 press - Typically means go South (downwards)
					KEY1_flag = 0;
					location = 44;		// increment place in map by 9
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 84) {
				if ( !(KEY_val & KEY2) && (KEY2_flag) ) {
					// KEY2 press - fight Minotaur!!
					KEY2_flag = 0;
					if (sword_flag) {
						minotaur_gone_flag = 1;
						minotaur_slain_flag = 1;
						location = 85;		// Minotaur is defeated!
						max_step_time_rem = (max_step_time_rem * 4);
						step_time_rem_SS = max_step_time_rem;
						time_250ms = 0;
						step_count++;
						value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
					} else {
						value = OSFlagPost(GameStatus, GAME_LOST, OS_FLAG_SET, &err);
					}
				} else if ( !(KEY_val & KEY3) && (KEY3_flag) ) {
					// KEY3 press - Run from the Minotaur!
					KEY3_flag = 0;
					minotaur_gone_flag = 1;
					location = 87;	// run from minotaur
					if (tot_time_rem_MM >= 1) {
						tot_time_rem_MM = 0;
						tot_time_rem_SS = 59;
					}
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 85) {
				if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
					// KEY0 press - grab key & escape!
					KEY0_flag = 0;
					key_flag = 1;
					location = 88;		// Minotaur is defeated!
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 86) {
				if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
					// KEY0 press - grab key & escape!
					KEY0_flag = 0;
					key_flag = 1;
					location = 88;		// Minotaur is chasing you!
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 87) {
				if ( !(KEY_val & KEY3) && (KEY3_flag) ) {
					// KEY3 press - KEEP RUNNING!
					KEY3_flag = 0;
					location = 38;		// Minotaur is CHASING YOU!
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 88) {
				if ( !(KEY_val & KEY1) && (KEY1_flag) ) {
					// KEY1 press - go South towards the exit!
					KEY1_flag = 0;
					location = 59;
					step_time_rem_SS = max_step_time_rem;
					time_250ms = 0;
					step_count++;
					value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
				}

			} else if (location == 89) {
				if ( !(KEY_val & KEY1) && (KEY1_flag) ) {
					// KEY1 press - Escape to Vicotry!!
					KEY1_flag = 0;
					value = OSFlagPost(GameStatus, GAME_FINISHED, OS_FLAG_SET, &err);
				}

			} else {
				// Regular (< 80) locations
				if ( !(KEY_val & KEY0) && (KEY0_flag) ) {
					// KEY0 press - Typically means go East (rightwards)
					KEY0_flag = 0;
					if (LUT_location_permissions[location] & KEY0) {
						location += 1;		// increment place in map by 1
						step_time_rem_SS = max_step_time_rem;
						time_250ms = 0;
						step_count++;
						value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
					}

				} else if ( !(KEY_val & KEY1) && (KEY1_flag) ) {
					// KEY1 press - Typically means go South (downwards)
					KEY1_flag = 0;
					if (LUT_location_permissions[location] & KEY1) {
						location += 9;		// increment place in map by 9
						step_time_rem_SS = max_step_time_rem;
						time_250ms = 0;
						step_count++;
						value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
					}

				} else if ( !(KEY_val & KEY2) && (KEY2_flag) ) {
					// KEY2 press - Typically means go North (upwards)
					KEY2_flag = 0;
					if (LUT_location_permissions[location] & KEY2) {
						location -= 9;		// decrement place in map by 9
						step_time_rem_SS = max_step_time_rem;
						time_250ms = 0;
						step_count++;
						value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
					}

				} else if ( !(KEY_val & KEY3) && (KEY3_flag) ) {
					// KEY3 press - Typically means go West (leftwards)
					KEY3_flag = 0;
					if (LUT_location_permissions[location] & KEY3) {
						location -= 1;		// decrement place in map by 1
						step_time_rem_SS = max_step_time_rem;
						time_250ms = 0;
						step_count++;
						value = OSFlagPost(GameStatus, GAME_NEW_LOCATION, OS_FLAG_SET, &err);
					}
				}
			}
		}

		OSSemPost(LocSem);

		OSTimeDly(1);
	}
}


void TaskStopwatch(void* pdata) {
	// Needs ACTIVE state to run

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

		OSMboxPost(MBoxRemStepTime, (void *)&step_time_rem_SS_char[0]);

		// Send total time remaining to taskDispRemTime (in MM:SS format)
		char tot_time_rem_MMSS_char[VGA_TEXT_MAX_SIZE];
		sprintf(tot_time_rem_MMSS_char, "%.2d:%.2d", tot_time_rem_MM, tot_time_rem_SS);

		OSMboxPost(MBoxRemTotTime, (void *)&tot_time_rem_MMSS_char[0]);

		OSTimeDly(2);

	}
}


void TaskDispNewLocation(void* pdata) {
	// Displays options & relevant text based on the current location

	while(1) {

		// Wait for game to be active, and for a new location to render. consumer new location flag
		value = OSFlagPend(GameStatus, GAME_ACTIVE, OS_FLAG_WAIT_SET_ALL, 0, &err);
		value = OSFlagPend(GameStatus, GAME_NEW_LOCATION, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);

		// pend if any non-active states (LOST, RESET, FINISHED) are high
		value = OSFlagPend(GameStatus, GAME_LOST+GAME_RESET+GAME_FINISHED, OS_FLAG_WAIT_CLR_ALL, 0, &err);

		OSSemPend(LocSem, 0, &err);

		int current_time = OSTimeGet();
		printf("%d: TaskDispRemTime || Location: %d || Step Count: %d\n", current_time, location, step_count);

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
	char *step_time_msg; 	// to receive MBoxRemStepTime from stopwatch
	char *tot_time_msg; 	// to receive MBoxRemTotTime from stopwatch

	while(1) {

		// pend until game is activated (to prevent useless processing in IDLE state)
		value = OSFlagPend(GameStatus, GAME_ACTIVE, OS_FLAG_WAIT_SET_ALL, 0, &err);

		// pend if any non-active states (LOST, RESET, FINISHED) are high
		value = OSFlagPend(GameStatus, GAME_LOST+GAME_RESET+GAME_FINISHED, OS_FLAG_WAIT_CLR_ALL, 0, &err);

		// receive msg from MBoxRemStepTime (from taskStopwatch) - Pend until it's ready
		step_time_msg = (char *)OSMboxPend(MBoxRemStepTime, 0 , &err);
		if (err == OS_ERR_NONE) {
			// No error; display remaining step time
			VGA_text(3, 52, "Step Time Remaining");
			VGA_text(3, 54, step_time_msg);

		} else {
			// Error on receiving msg
			int current_time = OSTimeGet();
			printf("%d: Error on Receiving MBoxRemStepTime via OSMboxPend in TaskDispRemTime\n", current_time);
		}


		// receive msg from MBoxRemTotTime (from taskStopwatch) - Pend until it's ready
		tot_time_msg = (char *)OSMboxPend(MBoxRemTotTime, 0 , &err);
		if (err == OS_ERR_NONE) {
			// No error; display remaining total time
			VGA_text(55, 52, "Total Time Remaining");
			VGA_text(55, 54, tot_time_msg);

		} else {
			// Error on receiving msg
			int current_time = OSTimeGet();
			printf("%d: Error on Receiving MBoxRemTotTime via OSMboxPend in TaskDispRemTime\n", current_time);
		}

		OSTimeDly(4);

	}
}


void TaskDispResults(void* pdata) {
	// Can only run in GAME_FINISHED state

	while(1) {

		// blocking delay until game is finished (to prevent useless processing in IDLE state)
		value = OSFlagPend(GameStatus, GAME_FINISHED, OS_FLAG_WAIT_SET_ALL, 0, &err);

		// Clear VGA before displaying winning results
	  VGA_clear();
	  VGA_text_clear();

		// disp winning message
		VGA_text(17, 20, "Congratulations! You escaped The Labyrinth!");
		VGA_text(35, 22, ":D");

		// display total time elapsed
		char tot_time_MMSS_msg[VGA_TEXT_MAX_SIZE];
		sprintf(tot_time_MMSS_msg, "Total Time: %.2d:%.2d", tot_time_MM, tot_time_SS);

		VGA_text(25, 30, tot_time_MMSS_msg);

		// display total step count
		char step_count_msg[VGA_TEXT_MAX_SIZE];
		sprintf(step_count_msg, "Total Steps Taken: %d", step_count);

		VGA_text(25, 33, step_count_msg);

		// display if sword was taken
		if (sword_flag) {
			VGA_text(25, 36, "Sword Taken? Yes");
		} else {
			VGA_text(25, 36, "Sword Taken? No");
		}

		// display if Minotaur was slain
		if (minotaur_slain_flag) {
			VGA_text(25, 39, "Minotaur Slain? Yes");
		} else {
			VGA_text(25, 39, "Minotaur Slain? No");
		}

		// display option to reset game (KEY1)
		VGA_disp_options(-1);

		// after running once, wait until a new game is started (and finished)
		value = OSFlagPend(GameStatus, GAME_RESET, OS_FLAG_WAIT_SET_ALL, 0, &err);

		OSTimeDly(4);

	}
}


void TaskDispGameOver(void* pdata) {
	// Can only run in GAME_LOST state

	while(1) {

		// blocking delay until game is lost (to prevent useless processing in IDLE state)
		value = OSFlagPend(GameStatus, GAME_LOST, OS_FLAG_WAIT_SET_ALL, 0, &err);

		// Clear VGA before displaying losing results
	  VGA_clear();
	  VGA_text_clear();

		// disp losing message
		VGA_text(27, 20, "The Minotaur got ya...");
		VGA_text(35, 22, ">:(");

		// display total time elapsed
		char tot_time_MMSS_msg[VGA_TEXT_MAX_SIZE];
		sprintf(tot_time_MMSS_msg, "Total Time: %.2d:%.2d", tot_time_MM, tot_time_SS);

		VGA_text(25, 30, tot_time_MMSS_msg);

		// display total step count
		char step_count_msg[VGA_TEXT_MAX_SIZE];
		sprintf(step_count_msg, "Total Steps Taken: %d", step_count);

		VGA_text(25, 33, step_count_msg);

		// display if sword was taken
		if (sword_flag) {
			VGA_text(25, 36, "Sword Taken? Yes");
		} else {
			VGA_text(25, 36, "Sword Taken? No");
		}

		// display if Minotaur was slain
		if (minotaur_slain_flag) {
			VGA_text(25, 39, "Minotaur Slain? Yes");
		} else {
			VGA_text(25, 39, "Minotaur Slain? No");
		}

		// display option to reset game
		VGA_disp_options(-1);

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

	OSTaskCreateExt(TaskDispGameOver,
                  NULL,
                  (void *)&TaskDispGameOver_stk[TASK_STACKSIZE-1],
  			          TASKDISPGAMEOVER_PRIORITY,
  			          TASKDISPGAMEOVER_PRIORITY,
  			          TaskDispGameOver_stk,
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

  MBoxRemStepTime = OSMboxCreate((void *)0);	// Mbox for remaining time each step
	MBoxRemTotTime = OSMboxCreate((void *)0);		// Mbox for total remaining game time

  GameStatus = OSFlagCreate(0x00, &err);

  OSStart();

  return 0;
}
