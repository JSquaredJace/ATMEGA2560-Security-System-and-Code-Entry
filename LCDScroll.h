/*
 * LCDScroll.h
 *
 * Header file to allow scrolling messages on the LCD screen.
 * Uses timer 3.
 * Author : Jace Johnson
 * Rev 1
 * Designed to work with LCD.h (Rev 1) by Jace Johnson
 */

#ifndef LCDSCROLL_H_
#define LCDSCROLL_H_

#include <string.h>
#include "LCD.h"

void initScrollStr();
void updateScrollStr();
void startScrollStr(char str[]);
void stopScrollStr();


char scrollStr[38];		//string for scrolling text
int scrollCounter = 0;	//counter for scrolling text

/*
 * ISR(TIMER3_OVF_vect)
 *  ISR to call a function to update the LCD with the next segment of the 
 *	scrolling text and update the scroll counter. Sets timer 3 so the ISR 
 *	repeats in 500 ms.
 *
 *  returns:    none
 */
ISR(TIMER3_OVF_vect){
	//update scrolling text and increment scroll counter
	updateScrollStr();
	
	//set timer 3 to overflow in 500 ms
	TCNT3 = 65536 - (int)((500 * 16000000.0) / 256.0);
}

/*
 * Function:  initScrollStr
 *  Sets up timer 3 in normal mode and stops the timer. Enables timer 3 
 *	overflow interrupt
 *
 *  returns:    none
 */
void initScrollStr(){
	TCCR3A = 0x00;									//normal timer mode
	TCCR3B &= !((1<<CS32)|(1<<CS31)|(1<<CS30));		//stop timer 3
	
	TIMSK3 = (1 << TOIE3);		//enable timer3 overflow interrupt
	return;
}

/*
 * Function:  updateScrollStr
 *  Updates the LCD with the next segment of the scrolling text and updates
 *	the scroll counter.
 *
 *  returns:    none
 */
void updateScrollStr(){
	char message[17];
	int line;
	line = 0;		//top line of LCD
	int count = 0;
	
	//restart string count at end of string
	if(scrollCounter == strlen(scrollStr)){
		scrollCounter = 0;
	}
	//start count at scrollCounter value
	count = scrollCounter;
	
	//loop until message string is full
	for(int i = 0; i < 16; i++){
		
		if(scrollStr[count] == '\0'){	//if scrollStr is less than 16 chars,
			count = 0;					//start copying from the beginning of
		}								//scrollStr
		
		message[i] = scrollStr[count];	//copy character to display message
		count++;
	}
	message[16] = '\0';					//add null terminator
	
	LCD_write_str(message, &line);		//print to top line of LCD
	scrollCounter++;					//update scroll counter
	return;
}

/*
 * Function:  startScrollStr
 *  Sets the string to be scrolled on the LCD screen, starts timer 3 and makes
 *	the timer overflow, triggering the ISR to control the scrolling. Uses the
 *	input string to set the scrolling message.
 *
 *  returns:    none
 */
void startScrollStr(char str[]){
	strcpy(scrollStr, str);	//set scrolling text
	
	TCCR3B = (1<<CS32);		//start timer 3 with 256 prescaler
	
	//set timer 3 to overflow in 1 clock cycle
	TCNT3 = 65536 - 1;
	return;
}

/*
 * Function:  stopScrollStr
 *  Sets the string to be scrolled on the LCD screen, starts timer 3 and makes
 *	the timer overflow, triggering the ISR to control the scrolling. Uses the
 *	input string to set the scrolling message.
 *
 *  returns:    none
 */
void stopScrollStr(){
	//Stop Timer 3
	TCCR3B &= !((1<<CS32)|(1<<CS31)|(1<<CS30));
	
	scrollCounter = 0;		//reset counter
	_delay_ms(10);			//short delay
	return;
}

#endif