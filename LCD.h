/*
 * LCD.h
 *
 * header file to control LCD screen
 * Author : Jace Johnson, Dr. Randy Hoover
 * Hardware:	ATMega 2560 operating at 16 MHz
 *				LCD screen
 *				10 KOhm potentiometer (POT)
 *				jumper wires
 * Configuration shown below
 *
 * ATMega 2560
 *  PORT   pin		   LCD Screen
 * -----------         ----------
 * |		 |	  GND--|K		|
 * |		 |	   5V--|A		|
 * | A7    29|---------|D7		|
 * | A6    28|---------|D6		|
 * | A5    27|---------|D5		|
 * | A4    26|---------|D4		|
 * |      	 |         |		|
 * | C1    52|---------|E		|
 * |		 |	  GND--|RW		|	10KOhm Potentiometer
 * | C0    53|---------|RS		|			POT
 * |      	 |         |		|		-----------
 * |		 |		   |	  V0|-------|W		5V|--5V
 * |	     |    5V---|VDD		|		|	   GND|--GND
 * |	  GND|---GND---|VSS		|		-----------
 * -----------		   ----------
 */

#ifndef LCD_H
#define LCD_H

//dependencies
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

//prototypes for functions provided by Dr. Randy Hoover
void LCD_init(void);
void LCD_E_RS_init(void);
void LCD_write_4bits(uint8_t Data);
void LCD_write_instruction(uint8_t Instruction);
void LCD_EnablePulse(void);
void LCD_write_char(char Data);

#define MAX_INPUT 40

//  Helpful LCD control defines  //
#define LCD_Reset              0b00110000          // reset the LCD to put in 4-bit mode //
#define LCD_4bit_enable        0b00100000          // 4-bit data - can't set the line display or fonts until this is set  //
#define LCD_4bit_mode          0b00101000          // 2-line display, 5 x 8 font  //
#define LCD_4bit_displayOFF    0b00001000          // set display off  //
#define LCD_4bit_displayON     0b00001100         // set display on - no blink //
#define LCD_4bit_displayON_Bl  0b00001101         // set display on - with blink //
#define LCD_4bit_displayCLEAR  0b00000001          // replace all chars with "space"  //
#define LCD_4bit_entryMODE     0b00000110          // set curser to write/read from left -> right  //
#define LCD_4bit_cursorSET     0b10000000          // set cursor position


//  For two line mode  //
#define LineOneStart 0x00
#define LineTwoStart 0x40 //  must set DDRAM address in LCD controller for line two  //

//  Pin definitions for PORTC control lines  //
#define LCD_EnablePin 1
#define LCD_RegisterSelectPin 0

//Prototypes for functions provided by Jace Johnson
void LCD_write_str(char arr[MAX_INPUT], int* LCDLine);
void LCD_clear_line(int* line);
int checkInput(char input[MAX_INPUT]);
int checkClearInput(char input[MAX_INPUT]);
int checkInputLen(char input[MAX_INPUT]);
void outputLine(char input[MAX_INPUT], int* LCDLine, int changeLine);
void printErr(int* LCDLine);



//  Important notes in sequence from page 26 in the KS0066U datasheet - initialize the LCD in 4-bit two line mode //
//  LCD is initially set to 8-bit mode - we need to reset the LCD controller to 4-bit mode before we can set anyting else //
void LCD_init(void)
{
	DDRC |= 0x23;	//setup pins in ports A and C as outputs for LCD screen
	DDRA |= 0xF0;
	
    //  Wait for power up - more than 30ms for vdd to rise to 4.5V //
    _delay_ms(100);
    
    //  Note that we need to reset the controller to enable 4-bit mode //
    LCD_E_RS_init();  //  Set the E and RS pins active low for each LCD reset  //
    
    //  Reset and wait for activation  //
    LCD_write_4bits(LCD_Reset);
    _delay_ms(10);
    
    //  Now we can set the LCD to 4-bit mode  //
    LCD_write_4bits(LCD_4bit_enable);
    _delay_us(80);  //  delay must be > 39us  //
    
    
    
    ////////////////  system reset is complete - set up LCD modes  ////////////////////
    //  At this point we are operating in 4-bit mode
    //  (which means we have to send the high-nibble and low-nibble separate)
    //  and can now set the line numbers and font size
    //  Notice:  we use the "LCD_wirte_4bits() when in 8-bit mode and the LCD_instruction() (this just
    //  makes use of two calls to the LCD_write_4bits() function )
    //  once we're in 4-bit mode.  The set of instructions are found in Table 7 of the datasheet.  //
    LCD_write_instruction(LCD_4bit_mode);
    _delay_us(80);  //  delay must be > 39us  //
    
    //  From page 26 (and Table 7) in the datasheet we need to:
    //  display = off, display = clear, and entry mode = set //
    LCD_write_instruction(LCD_4bit_displayOFF);
    _delay_us(80);  //  delay must be > 39us  //
    
    LCD_write_instruction(LCD_4bit_displayCLEAR);
    _delay_ms(80);  //  delay must be > 1.53ms  //
    
    LCD_write_instruction(LCD_4bit_entryMODE);
    _delay_us(80);  //  delay must be > 39us  //
    
    //  The LCD should now be initialized to operate in 4-bit mode, 2 lines, 5 x 8 dot fonstsize  //
    //  Need to turn the display back on for use  //
    LCD_write_instruction(LCD_4bit_displayON);
    _delay_us(80);  //  delay must be > 39us  //

}

void LCD_E_RS_init(void)
{
    //  Set up the E and RS lines to active low for the reset function  //
    PORTC &= ~(1<<LCD_EnablePin);
    PORTC &= ~(1<<LCD_RegisterSelectPin);
}

//  Send a byte of Data to the LCD module  //
void LCD_write_4bits(uint8_t Data)
{
    //  We are only interested in sending the data to the upper 4 bits of PORTA //
    PORTA &= 0b00001111;  //  Ensure the upper nybble of PORTA is cleared  //
    PORTA |= Data;  // Write the data to the data lines on PORTA  //
    
    //  The data is now sitting on the upper nybble of PORTA - need to pulse enable to send it //
    LCD_EnablePulse();  //  Pulse the enable to write/read the data  //
}

//  Write an instruction in 4-bit mode - need to send the upper nybble first and then the lower nybble  //
void LCD_write_instruction(uint8_t Instruction)
{
    //  ensure RS is low  //
    //PORTC &= ~(1<<LCD_RegisterSelectPin);
    LCD_E_RS_init();  //  Set the E and RS pins active low for each LCD reset  //
    
    LCD_write_4bits(Instruction);  //  write the high nybble first  //
    LCD_write_4bits(Instruction<<4);  //  write the low nybble  //
    
    
}

//  Pulse the Enable pin on the LCD controller to write/read the data lines - should be at least 230ns pulse width //
void LCD_EnablePulse(void)
{
    //  Set the enable bit low -> high -> low  //
    //PORTC &= ~(1<<LCD_EnablePin); // Set enable low //
    //_delay_us(1);  //  wait to ensure the pin is low  //
    PORTC |= (1<<LCD_EnablePin);  //  Set enable high  //
    _delay_us(1);  //  wait to ensure the pin is high  //
    PORTC &= ~(1<<LCD_EnablePin); // Set enable low //
    _delay_us(1);  //  wait to ensure the pin is low  //
}

//  write a character to the display  //
void LCD_write_char(char Data)
{
    //  Set up the E and RS lines for data writing  //
    PORTC |= (1<<LCD_RegisterSelectPin);  //  Ensure RS pin is set high //
    PORTC &= ~(1<<LCD_EnablePin);  //  Ensure the enable pin is low  //
    LCD_write_4bits(Data);  //  write the upper nybble  //
    LCD_write_4bits(Data<<4);  //  write the lower nybble  //
    _delay_us(80);  //  need to wait > 43us  //
    
}

/*
 * Function:	LCD_write_str
 *  Writes the input string to the input line on the LCD screen. wraps line if
 *	it is too large for one line. Does not check if the string is too large 
 *	for two lines and will continue line wrapping. Checking if a string is too
 *	large for two LCD lines will be handled outside of this function.
 *
 *	arr		char[]	string to be written to LCD screen
 *	line	int*	LCD screen line to be cleared
 *
 *  returns:  none
 */
void LCD_write_str(char arr[MAX_INPUT], int* LCDLine){
	int i = 0;		//array index counter
	int count = 0;	//LCD line wrapping counter
	
	//set line to write
	if(*LCDLine == 0){	//write to line 1
		LCD_write_instruction(LCD_4bit_cursorSET | LineOneStart);
	}
	else{				//write to line 2
		LCD_write_instruction(LCD_4bit_cursorSET | LineTwoStart);
	}
	_delay_us(80);		//short delay for LCD to update
	
	//loop to write chars to line until null terminator is encountered
	while(arr[i] != '\0'){
		LCD_write_char(arr[i]);	//write current char
		i++;					//increment index counter for array
		count++;				//increment line wrapping counter
		
		if(arr[i] != '\0'){
			//check if line needs to be wrapped
			if(count > 15){
				count = 0;			//reset line wrapping counter
				*LCDLine ^= 0x01;	//update current LCD line
				
									//change LCD line
				if(*LCDLine == 0){	//select LCD line 1
					LCD_write_instruction(LCD_4bit_cursorSET | LineOneStart);
				}
				else{				//select LDC line 2
					LCD_write_instruction(LCD_4bit_cursorSET | LineTwoStart);
				}
				_delay_us(80);		//short delay for LCD to update
			}
		}
	}
	*LCDLine ^= 0x01;				//change current LCD line
	return;
}

/*
 * Function:	LCD_clear_line
 *  Clears a line of the LCD screen based on the input pointer, then corrects 
 *	the value the pointer is pointing at to be the line that was cleared.
 *
 *	line	int*	LCD screen line to be cleared
 *
 *  returns:  none
 */
void LCD_clear_line(int* line){
	LCD_write_str("                ", line);	//write clear line to LCD
	*line ^= 0x01;								//go to line that was cleared
	return;
}

/*
 * Function:	checkInput
 *	Checks input string to see if Ctrl+C was pressed or if input is too long.
 *	Returns a status value depending on if either of these are true
 *
 *	input	char[]	string to be checked
 *
 *  returns:	0	status is normal
 *				1	status is clear screen (Ctrl+C is entered)
 *				2	status is input line is too long
 */
int checkInput(char input[MAX_INPUT]){
	int status = 0;
	
	status = checkClearInput(input);	//check if Ctrl+C is entered
	if(status != 0){					//return 1 if true
		return status;
	}
	
	status = checkInputLen(input);		//check if input line is too long
										//set status to 2 if true
	
	return status;						//return status;
}

/*
 * Function:	checkClearInput
 *	Checks input string. If input string contains only Ctrl+C, then the LCD 
 *	screen is cleared. A status value is returned.
 *
 *	input	char[]	string to be checked
 *
 *  returns:	0	screen was not cleared (Ctrl+C is not entered)
 *				1	screen is cleared (Ctrl+C is entered)
 */
int checkClearInput(char input[MAX_INPUT]){
	int temp = 0;
	
	if(strcmp(input, "\x03") == 0){	//if Ctrl+C is entered, clear the screen
		LCD_clear_line(&temp);	//clear line 1
		temp = 1;
		LCD_clear_line(&temp);	//clear line 2
		return 1;				//return clear line status
	}
	return 0;					//Ctrl+C was not entered
}

/*
 * Function:	checkInputLen
 *	Checks input string. If input string is too long to be outputted to both 
 *	lines of the LCD screen, 2 is returned, else 0 is returned
 *
 *	input	char[]	string to be checked
 *
 *  returns:	0	string will fit on LCD screen
 *				2	string will not fit on LCD screen
 */
int checkInputLen(char input[MAX_INPUT]){
	for(int i = 0; i < 33; i++){	//for loop to check for end of string
		if(input[i] == '\0'){	//if end of string is encountered, return 0
			return 0;
		}
	}
	//if string is longer than two lines of the LCD screen, return 2
	return 2;
}

/*
 * Function:	outputLine
 *	Check the status of the input line and output accordingly. If status is 0
 *	(normal status), write the input string to the input line of the LCD 
 *	screen. If status is 1 (clear screen), output nothing. If status is 2 
 *	(string is too long), then output an error message. Also checks if return
 *	was pressed without any other input (input string is empty) and changes the
 *	LCD line without clearing if true.
 *
 *	input			char[]	string to be written to the LCD screen
 *	LCDLine			int*	line of the LCD screen to write the string to 
 *							(will wrap to the other line if too long for one 
 *							line)
 *	returnStatus	int		number of chars in input string (without null 
 *							terminator)
 *
 *  returns:	none
 */
void outputLine(char input[MAX_INPUT], int* LCDLine, int changeLine){
	int status = 0;
	
	if(changeLine == 1){	//change LCD line if flag is set
		strcpy(input, "");
		LCD_write_str(input, LCDLine);
		return;				//return from function
	}
	
	status = checkInput(input);	//get the status of the input string
	
	if(status == 0){				//if string is normal, output to LCD
		LCD_clear_line(LCDLine);	//clear LCD line and write string to line
		LCD_write_str(input, LCDLine);
	}
	if(status == 2){			//if string is too long, print error message
		printErr(LCDLine);		//print error message and set LCDline to 0
	}
	return;
}

/*
 * Function:	printErr
 *	Write error message to LCD screen and set LCDLine to 0
 *
 *	LCDLine			int*	line of the LCD screen to write the string to
 *
 *  returns:	none
 */
void printErr(int* LCDLine){
	char errMsg[MAX_INPUT] = "Error:";
	
	*LCDLine = 0;					//write to LCD line 1
	LCD_clear_line(LCDLine);		//clear line
	LCD_write_str(errMsg, LCDLine);	//print top half of error message
	
	strcpy(errMsg, "Line Too Long");
	*LCDLine = 1;					//write to LCD line 2
	LCD_clear_line(LCDLine);		//clear line
	LCD_write_str(errMsg, LCDLine);	//write bottom half of error message
	
	return;
}

#endif