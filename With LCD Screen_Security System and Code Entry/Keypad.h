/*
 * Keypad.h
 *
 * Header file for taking 4x4 keypad input using external interrupts.
 * Uses external interrupts 0 - 3.
 * Author : Jace Johnson
 * Rev 1
 * Hardware:	ATMega 2560 operating at 16 MHz
 *				4x4 keypad module
 *				jumper wires
 * Configuration shown below
 *
 *  ATMega 2560
 *   PORT  pin			 Keypad
 *	---------- 		   ----------
 *	| D3   18|---------|1		|
 *	| D2   19|---------|2		|
 *	| D1   20|---------|3		|
 *	| D0   21|---------|4		|
 *	|		 |		   |		|
 *	| C0   37|---------|5		|
 *	| C1   36|---------|6		|
 *	| C2   35|---------|7		|
 *	| C3   34|---------|8		|
 *	----------		   ----------
 */ 


#ifndef KEYPAD_H_
#define KEYPAD_H_

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

void initKeypad();
void initExInterrupts();
void readNumPad(int readCol);
void waitForKeypadClear();
void getNewKey();


int newKeyInput = 0;	//flag for if the input in pressedKey is new
int pressedKey = -1;	//var for key that is currently pressed

/*
 * ISR(INT0_vect)
 *  Finds which key was pressed in the leftmost row (pin 4 of the keypad, 
 *	PORTD0). Sets pressedKey to this value and sets newKeyInput flag.
 *
 *  returns:    none
 */
ISR(INT0_vect){
	if(newKeyInput == 0){
		_delay_ms(100);		//debounce delay
		
		int readCol = 0;	//read column 0
		readNumPad(readCol);
		
		newKeyInput = 1;	//set new input flag
	}
}

/*
 * ISR(INT1_vect)
 *  Finds which key was pressed in the middle-left row (pin 3 of the keypad,
 *	PORTD1). Sets pressedKey to this value and sets newKeyInput flag.
 *
 *  returns:    none
 */
ISR(INT1_vect){
	if(newKeyInput == 0){
		_delay_ms(100);		//debounce delay
		
		int readCol = 1;	//read column 1
		readNumPad(readCol);
		
		newKeyInput = 1;	//set new input flag
	}
}

/*
 * ISR(INT2_vect)
 *  Finds which key was pressed in the middle-right row (pin 2 of the keypad,
 *	PORTD2). Sets pressedKey to this value and sets newKeyInput flag.
 *
 *  returns:    none
 */
ISR(INT2_vect){
	if(newKeyInput == 0){
		_delay_ms(100);		//debounce delay
		
		int readCol = 2;	//read column 2
		readNumPad(readCol);
		
		newKeyInput = 1;	//set new input flag
	}
}

/*
 * ISR(INT3_vect)
 *  Finds which key was pressed in the rightmost row (pin 1 of the keypad,
 *	PORTD3). Sets pressedKey to this value and sets newKeyInput flag.
 *
 *  returns:    none
 */
ISR(INT3_vect){
	if(newKeyInput == 0){
		_delay_ms(100);		//debounce delay
		
		int readCol = 3;	//read column 3
		readNumPad(readCol);
		
		newKeyInput = 1;	//set new input flag
	}
}

/*
 * Function:  initKeypad
 *  Calls functions to initialize external interrupt 0 - 3.
 *
 *  returns:    none
 */
void initKeypad(){
	initExInterrupts();	//initialize external interrupts
	sei();				//enable global interrupts
	return;
}

/*
 * Function:  initExInterrupts
 *  Sets PORTC 0 - 3 as outputs and PORTD 0 - 3 as inputs. enables external 
 *	interrupts 0 - 3 (PORTD 0 - 3)and sets them to trigger on falling edge.
 *
 *  returns:    none
 */
void initExInterrupts(){
	DDRC |= 0x0F;	//set low nybble of PORTC as output
	DDRD &= 0xF0;	//set low nybble of PORTD as input
	
	PORTC &= 0xF0;	//set outputs low
	PORTD |= 0x0F;	//enable pull up resistors for inputs
	
	//set external interrupt to trigger on falling edge (pin state change from
	//5V to GND) for external interrupt pins 0 - 3
	EICRA |= (1<<ISC01)|(1<<ISC11)|(1<<ISC21)|(1<<ISC31);
	EICRA &= !((1<<ISC00)|(1<<ISC10)|(1<<ISC20)|(1<<ISC30));
	//enable external interrupts 0 - 3
	EIMSK |= (1<<INT0)|(1<<INT1)|(1<<INT2)|(1<<INT3);
	return;
}

/*
 * Function:  readNumPad
 *  Checks each row and sets pressedKey (global var) to the value of key 
 *	pressed based on input column. pressedKey is set to the following based on
 *	the pressed key:	0 - 9		corresponding number key is pressed 0 - 9
 *						0xA - 0xD	corresponding letter key is pressed A - D
 *						0xE			# key is pressed
 *						0xF			* key is pressed
 *						-1			invalid input or no input
 *
 *	readCol	int		keypad column to get input from
 *
 *  returns:    none
 */
void readNumPad(int readCol){
	if((readCol == -1)||(readCol > 3)){	//if column is invalid, return
		return;
	}
	
	int keyPressed[4][4] = {	//2D array of keypad button values
		{0X1, 0x2, 0x3, 0xA},
		{0x4, 0x5, 0x6, 0xB},
		{0x7, 0x8, 0x9, 0xC},
		{0xF, 0x0, 0xE, 0xD}
	};
	
	//row mask for different rows of keypad
	int keyRowMask[4] = {0xF7, 0xFB, 0xFD, 0xFE};
	
	//loop to check each row
	for(int row = 0; row < 4; row++){
		PORTC |= 0x0F;				//set high (clear) output pins of PORTC
		PORTC &= keyRowMask[row];	//set low current row of keypad
		asm("nop");					//short delay
		asm("nop");
		asm("nop");
			
		//check if row is low in column
		if(!(PIND & (1 << readCol))){
			//if key is pressed, get value of the pressed key and set newKeyInput
			//flag
			pressedKey = keyPressed[row][readCol];
			newKeyInput = 1;
			
			PORTC &= 0xF0;				//set low output pins of PORTC
			return;
		}
	}
	
	PORTC &= 0xF0;				//set low output pins of PORTC
	
	//pressedKey is -1 for invalid input or no input and newKeyInput is reset
	pressedKey = -1;
	newKeyInput = 0;
	return;
}

/*
 * Function:  waitForKeypadClear
 *  Delays until no keys are being pressed on the keypad. Clears the global 
 *	variable pressedKey and global flag newKeyInput.
 *
 *  returns:    none
 */
void waitForKeypadClear(){
	while((PIND & 0x0F) != 0x0F){}	//wait for kaypad buttons to be unpressed
	pressedKey = -1;				//clear input variable
	newKeyInput = 0;				//reset new input flag
	return;
}

/*
 * Function:  getNewKey
 *  Delays until the buttons on the keypad have been unpressed then a new 
 *	button is pressed.
 *
 *  returns:    none
 */
void getNewKey(){
	waitForKeypadClear();		//wait for keypad to clear
	
	while(pressedKey == -1){	//wait for new key to be pressed
		_delay_us(1);			//short delay
		}
	return;
}

#endif
