/*
 * Keypad.h
 *
 * Created: 7/25/2020 10:58:30 PM
 *  Author: 7409254
 */ 


#ifndef KEYPAD_H_
#define KEYPAD_H_

/*
 * ISR:  TIMER0_OVF_vect
 *  Interrupt for timer0 overflow. Calls checkNumPad function every 50 ms, 
 *	then resets timer0 to overflow 50 ms later. 50 ms delay used to 
 *
 *  returns:    none
 */
ISR(TIMER0_OVF_vect) {
	checkNumPad();	//check key pad and take input if a button is pressed
	TCNT0 = 65536 - (int)(0.1 * 16000000.0 / 256.0); //reset timer for 100 ms
	return;
}

void initKeypad(){
	TCCR0A = 0x00;
	TCCR0B |= (1<<CS12);	//turn timer0 on with 256 prescaler
	
	sei();		//Enable global interrupts by setting global interrupt enable
	//bit in SREG
	
	TIMSK0 = (1 << TOIE0);		//enable timer0 overflow interrupt(TOIE0)
	TCNT0 = 65536 - 1;			//timer0 overflow in one clock cycle
	
	DDRD &= 0xF0;	//set PORTD as input
	DDRC |= 0x0F;	//set low nybble as output and high nybble as input
	
	PORTC |= 0x0F;	//set PORTC outputs high and enable pull up resistors for
	PORTD |= 0x0F;	//inputs
	
	return;
}

/*
 * Function:  checkNumPad
 *  checks if any key has been pressed and calls readNumPad function if key is
 *	pressed. Exits if key has not been released to help with debouncing.
 *
 *  returns:    none
 */
void checkNumPad(){
	int temp = 0x0000;
	PORTC &= 0xF0;		//Enable all switches
	
	temp = PINC & 0xF0;	//temp value for input pins of PORTC
	
	if(keypadClear == 0){	//check if any keypad button is still pressed
		if(temp == 0x00){	//exit if any button has not been released
			return;
		}
	}
			
	keypadClear = 1;			//set keypadClear of keypad buttons are not 
								//pressed
	
	if(temp != 0xF0){			//if new key is pressed, read the key
		keypadClear = 0;		//keypadClear is set to 0 because key is 
								//pressed
		input = readNumPad();	//read pressed button and store in input var
	}
	return;
}

/*
 * Function:  checkNumPad
 *  checks each row and column and returns value of key pressed
 *
 *  returns:    int		value of key pressed in keypad
 */
int readNumPad(){
	int keyPressed[4][4] = {	//2D array of keypad button values
		{0x1, 0x2, 0x3, 0xA},
		{0x4, 0x5, 0x6, 0xB},
		{0x7, 0x8, 0x9, 0xC},
		{0xF, 0x10, 0xE, 0xD}
	};
	
	//row mask to ground different rows of keypad
	int keyRowMask[4] = {0x07, 0x0B, 0x0D, 0x0E};
	//column mask to check the column of the pressed button	
	int keyColMask[4] = {0x80, 0x40, 0x20, 0x10};
	
	
	for(int i = 0; i < 4; i++){	//loop to check each row
		PORTC &= 0xF0;			//set high (clear) output pins of PORTC
		PORTC |= keyRowMask[i];	//check each row of keypad
		asm("nop");				//short delay
		
		for(int j = 0; j < 4; j++){			//loop to check each column in 
											//selected row
			if (!(PINC & keyColMask[j])){	//check each column for pressed 
											//button
				return keyPressed[i][j];	//if key is pressed, return value 
											//of that key
			}
		}
	}
	
	return -1;	//return -1 if no button is pressed
}





#endif /* KEYPAD_H_ */