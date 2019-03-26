/*
CME332 Lab 1 Part 1 - Tuesday, January 22nd 2019
Tyrel Kostyk, tck290, 11216033
*/


// Declarations

#define HEX3_HEX0_ptr		(((volatile unsigned long *)0xFF200020))		// Mem Addr for HEX4 - HEX7 Displays
#define	HEX7_HEX4_ptr		(((volatile unsigned long *)0xFF200030))		// Mem Addr for HEX4 - HEX7 Displays
#define	SW_ptr			(((volatile unsigned long *)0xFF200040))		// Mem Addr for SW0 - SW17

// Main program


const char lut_num [11] =   {
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
                                 0x04   // -  000.0100 (error)

};


void display_HEX5 (int four_bit_val) {
/* Display 4bit number (in decimal) onto HEX5 7-Segment Display */

	// clear HEX5
	*HEX7_HEX4_ptr &= ~0xFF;	

	switch (four_bit_val) {
	case 0: *HEX7_HEX4_ptr |= lut_num[0];
		break;
	case 1: *HEX7_HEX4_ptr |= lut_num[1];
		break;
	case 2: *HEX7_HEX4_ptr |= lut_num[2];
		break;
	case 3: *HEX7_HEX4_ptr |= lut_num[3];
		break;
	case 4: *HEX7_HEX4_ptr |= lut_num[4];
		break;
	case 5: *HEX7_HEX4_ptr |= lut_num[5];
		break;
	case 6: *HEX7_HEX4_ptr |= lut_num[6];
		break;
	case 7: *HEX7_HEX4_ptr |= lut_num[7];
		break;
	case 8: *HEX7_HEX4_ptr |= lut_num[8];
		break;
	case 9: *HEX7_HEX4_ptr |= lut_num[9];
		break;
	default:	// Error Case
		*HEX7_HEX4_ptr |= lut_num[10];
		break;
	}

	return;
}


void display_HEX2 (int four_bit_val) {
/* Display 4bit number (in decimal) onto HEX2 7-Segment Display */

	// clear HEX2
	*HEX3_HEX0_ptr &= ~0xFF0000;	

	switch (four_bit_val) {
	case (0): 
		*HEX3_HEX0_ptr |= lut_num[0] << 16;
		break;
	case (1):
		*HEX3_HEX0_ptr |= lut_num[1] << 16;
		break;
	case (2):
		*HEX3_HEX0_ptr |= lut_num[2] << 16;
		break;
	case (3): 
		*HEX3_HEX0_ptr |= lut_num[3] << 16;
		break;
	case (4): 
		*HEX3_HEX0_ptr |= lut_num[4] << 16;
		break;
	case (5): 
		*HEX3_HEX0_ptr |= lut_num[5] << 16;
		break;
	case (6):
		*HEX3_HEX0_ptr |= lut_num[6] << 16;
		break;
	case (7): 
		*HEX3_HEX0_ptr |= lut_num[7] << 16;
		break;
	case (8): 
		*HEX3_HEX0_ptr |= lut_num[8] << 16;
		break;
	case (9): 
		*HEX3_HEX0_ptr |= lut_num[9] << 16;
		break;
	default:
		*HEX3_HEX0_ptr |= lut_num[10] << 16;
		break;
	}

	return;
}


void display_HEX1 (int four_bit_val) {
/* Display 4bit number (in decimal) onto HEX1 7-Segment Display */

	// clear HEX1
	*HEX3_HEX0_ptr &= ~0xFF00;	

	switch (four_bit_val) {
	case (0): 
		*HEX3_HEX0_ptr |= lut_num[0] << 8;
		break;
	case (1): 
		*HEX3_HEX0_ptr |= lut_num[1] << 8;
		break;
	case (2): 
		*HEX3_HEX0_ptr |= lut_num[2] << 8;
		break;
	case (3): 
		*HEX3_HEX0_ptr |= lut_num[3] << 8;
		break;
	case (4): 
		*HEX3_HEX0_ptr |= lut_num[4] << 8;
		break;
	case (5): 
		*HEX3_HEX0_ptr |= lut_num[5] << 8;
		break;
	case (6):
		*HEX3_HEX0_ptr |= lut_num[6] << 8;
		break;
	case (7): 
		*HEX3_HEX0_ptr |= lut_num[7] << 8;
		break;
	case (8): 
		*HEX3_HEX0_ptr |= lut_num[8] << 8;
		break;
	case (9): 
		*HEX3_HEX0_ptr |= lut_num[9] << 8;
		break;
	default: 
		*HEX3_HEX0_ptr |= lut_num[10] << 8;
		break;
	}

	return;
}


void display_HEX0 (int four_bit_val) {
/* Display 4bit number (in decimal) onto HEX0 7-Segment Display */

	// clear HEX0
	*HEX3_HEX0_ptr &= ~0xFF;	

	switch (four_bit_val) {
	case (0): 
		*HEX3_HEX0_ptr |= lut_num[0];
		break;
	case (1): 
		*HEX3_HEX0_ptr |= lut_num[1];
		break;
	case (2): 
		*HEX3_HEX0_ptr |= lut_num[2];
		break;
	case (3): 
		*HEX3_HEX0_ptr |= lut_num[3];
		break;
	case (4): 
		*HEX3_HEX0_ptr |= lut_num[4];
		break;
	case (5): 
		*HEX3_HEX0_ptr |= lut_num[5];
		break;
	case (6):
		*HEX3_HEX0_ptr |= lut_num[6];
		break;
	case (7): 
		*HEX3_HEX0_ptr |= lut_num[7];
		break;
	case (8): 
		*HEX3_HEX0_ptr |= lut_num[8];
		break;
	case (9): 
		*HEX3_HEX0_ptr |= lut_num[9];
		break;
	default: 
		*HEX3_HEX0_ptr |= lut_num[10];
		break;
	}

	return;
}



int main (void) {

// clear all 8 HEX displays before operation
*HEX3_HEX0_ptr &= ~0xFFFFFFFF;	
*HEX7_HEX4_ptr &= ~0xFFFFFFFF;	

	while(1) {

		int SW_value, SW74_val, SW158_val;

		// read the SW Values (17-0)
		SW_value = *(SW_ptr);			// grab value from Switches
		SW74_val = (SW_value & 0xF0) >> 4;	// grab int value from binary SW7-SW4
		SW158_val = (SW_value & 0xFF00) >> 8;	// grab int value from binary SW15-SW8

		// Display 4bit decimal val from binary input SW7-SW4 on 7-Segment display HEX5
		display_HEX5(SW74_val);

		// Calculate each decimal value of each digit from binary input SW15-SW8 
		int SW158_100s, SW158_10s, SW158_1s;
		SW158_100s = SW158_val / 100;
		SW158_10s = (SW158_val - (100*SW158_100s)) / 10;
		SW158_1s = (SW158_val - (100*SW158_100s) - (10*SW158_10s));
	
		// Display on HEX2
		display_HEX2(SW158_100s);

		// Display on HEX1
		display_HEX1(SW158_10s);

		// Display on HEX0
		display_HEX0(SW158_1s);
	
	}
	
}





