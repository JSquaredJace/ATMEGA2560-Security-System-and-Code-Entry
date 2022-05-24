/*
 * Lab06 - Security System and Code Entry with LCD Screen
 * Author : Jace Johnson
 * Created: 3/28/2020 11:26:17 AM
 *	  Rev 1	3/28/2020
 * Description:	ATMEGA2560 emulates a home security keypad with a screen. It
 *				does this by taking in information from a keypad matrix and
 *				outputting prompts and other information to the LCD screen.
 * 
 * Hardware:	ATMega 2560 operating at 16 MHz
 *				LCD screen
 *				10KOhm Potentiometer (POT)
 *				4x4 keypad module
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
 * | B1    52|---------|E		|
 * |		 |	  GND--|RW		|	10KOhm Potentiometer
 * | B0    53|---------|RS		|			POT
 * |      	 |         |		|		-----------
 * |		 |		   |	  V0|-------|W		5V|--5V
 * |	     |    5V---|VDD		|		|	   GND|--GND
 * |	  GND|---GND---|VSS		|		-----------
 * |		 |		   ----------
 * |		 |
 * |		 |			 Keypad
 * |		 | 		   ----------
 * | D3    18|---------|1		|
 * | D2    19|---------|2		|
 * | D1    20|---------|3		|
 * | D0    21|---------|4		|
 * |		 |		   |		|
 * | C0    37|---------|5		|
 * | C1    36|---------|6		|
 * | C2    35|---------|7		|
 * | C3    34|---------|8		|
 * -----------		   ----------
 *
 */ 

#define F_CPU 16000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "LCD.h"
#include "LCDScroll.h"
#include "Keypad.h"

void blinkMsg(char str[40]);
void err();
void start();
int enterCode(int changePIN);
int succPIN();
void enAlarm();
void disAlarm();
void selection();
void displayMenu();
void displayStart();


int pin[4] = {-1, 0, 0, 0};			//stored pin number for arming system
int unlockPIN[4] = {-2, 0, 0, 0};	//unlock pin number for unlocking system

int alarmEnable = 0;	//1 when alarm is enabled
int PINset = 0;			//1 when pin has been set

/*
 * Function:  main
 *  Calls functions to initialize ports, timers, and fill display array and to
 *	handle the commands from the key pad. Uses a forever loop to constantly 
 *	update the 4 digit 7 segment display and handle commands from the key pad
 *
 *  returns:    0   successful program run.
 */
int main(void)
{	
	LCD_init();			//initialize LCD screen
	initScrollStr();	//initialize scrolling text for LCD screen
	initKeypad();		//initialize keypad module
	displayStart();		//scroll starting message
	
	//forever loop
	while(1){
		selection();
	}
	return 0;
}

/*
 * Function:  blinkMsg
 *  Blinks the input string on the LCD screen for 3 seconds. Each second, the 
 *	LCD screen shows message for 0.5 seconds, then shows a blank screen for 
 *	0.5 seconds.
 *
 *	str		char[40]	input string to be displayed
 *
 *  returns:    none
 */
void blinkMsg(char str[40]){
	int line;
	
	//blink message each second for 3 seconds
	for(int i = 0; i < 3; i++){
		//clear both lines of LCD screen and display message on first line for
		//0.5 sec (wrapping long text to the second line)
		line = 1;
		LCD_clear_line(&line);
		line = 0;
		LCD_clear_line(&line);
		LCD_write_str(str, &line);
		
		_delay_ms(500);
		
		//clear both lines of LCD screen for 0.5 sec
		line = 1;
		LCD_clear_line(&line);
		line = 0;
		LCD_clear_line(&line);
		
		_delay_ms(500);
	}
	return;
}

/*
 * Function:  err
 *  Displays blinking error message. Used as a General error message.
 *
 *  returns:    none
 */
void err(){
	char str[6] = "Error";
	blinkMsg(str);
	return;
}

/*
 * Function:  start
 *  blinks all segments in display to indicate no pin has been set. Calls
 *	enterCode function to set pin if "A" or "C" buttons are pressed.
 *
 *  returns:    none
 */
void start(){
	int confirm = 1;	//User PIN confirmation flag
	
	//wait for A or C to be pressed
	while(!((pressedKey == 0xA) || (pressedKey == 0xC))){
		getNewKey();	//wait for new key from keypad
	}
	stopScrollStr();	//stop scrolling start message
	
	while(confirm != 0){		//loop until pin is confirmed by user
		if(enterCode(1) == 0){	//get acceptable PIN
			confirm = succPIN();//confirm PIN with user
			PINset = 1;			//PIN has been set
		}
		else{					//if pin is unsuccsessful, display error
			err();
		}
	}
	return;
}

/*
 * Function:  enterCode
 *  sets pin or unlock pin based on input from keypad and value of input param
 *	changePIN.
 *
 *	changePIN	0 changes the pin to unlock the system (unlockPIN)
 *				1 changes the stored pin (pin)
 *
 *  returns:    0 entered pin is acceptable
 *				1 entered pin is unacceptable
 */
int enterCode(int changePIN){
	int newKey = 0;		//next key pressed on keypad
	int errr = 1;		//set error flag true
	int LCDline;		//line on LCD
	char dispPIN[17];	//string to display the pin
	
	
	LCDline = 0;		//write message to top line of LCD
	LCD_clear_line(&LCDline);
	LCD_write_str("Enter PIN:", &LCDline);
	
	LCDline = 1;		//set to bottom line of LCD and clear it
	LCD_clear_line(&LCDline);
	
	
	//loop to get each new value and exit if pin cant fit on a line of the LCD
	for(int i = 0; i < 17; i++){
		getNewKey();	//get next key that's pressed
		newKey = pressedKey;
		
		if(newKey == 0xE){	//if # is pressed, exit the loop
			if(i == 4){		//successful pin condition
				errr = 0;	//reset error flag if entered pin is 4 digits
			}
			break;			//exit loop early
		}
		
		if(newKey > 9){		//if pressed key is not a number, exit
			break;
		}
		
		//store the pressed key in the 4 digit pin or unlockPIN array
		if(i < 4){
			//if changePIN is true, store the new key in the pin array
			if(changePIN == 1){
				pin[i] = newKey;
			}
			//if changePIN is true, store the new key in the unlockPIN array
			else{
				unlockPIN[i] = newKey;
			}
		}
		
		//convert newKey to char and store in dispPIN
		dispPIN[i] = newKey + 0x30;
		dispPIN[i + 1] = '\0';
		
		//write display pin to bottom line of LCD
		LCDline = 1;
		LCD_write_str(dispPIN, &LCDline);
	}
	return errr;	//return error state
}

/*
 * Function:  succPIN
 *  Displays a message showing the entered pin and prompts the user to confirm
 *	their pin or chose to enter a new one. This function recursively calls 
 *	itself on invalid input from keypad
 *
 *  returns:    0	pin confirmed by user
 *				1	user would like to enter a different pin	
 */
int succPIN(){
	int LCDline;
	int confirm = 0;
	
	//confirmation message top line
	char message[18] = "You entered: ";
	//add PIN to message
	message[13] = pin[0] + 0x30;
	message[14] = pin[1] + 0x30;
	message[15] = pin[2] + 0x30;
	message[16] = pin[3] + 0x30;
	message[17] = ' ';
	message[18] = ' ';
	message[19] = '\0';
	
	//print first line (scrolling)
	startScrollStr(message);
	//LCDline = 0;
	//LCD_write_str(message, &LCDline);
	
	//print second line
	strcpy(message, "1=OK, 2=New Pin");
	LCDline = 1;
	LCD_write_str(message, &LCDline);
	
	getNewKey();			//wait for user option
	confirm = pressedKey;	//get user option
	
	//if invalid key, error and call recursively call function
	if((confirm < 1)|(confirm > 2)){
		err();
		return succPIN();	//call function until input is valid, then return
	}
	//stop scrolling text
	stopScrollStr();
	//deprecate option for 0 = OK 1 = new pin and return the value
	return confirm - 1;
}

/*
 * Function:  enAlarm
 *  Checks that the pin has been set and enables the alarm.
 *
 *  returns:    none
 */
void enAlarm(){
	if(PINset == 1){		//if PIN has been set, arm alarm system
		alarmEnable = 1;	//set alarm enable flag
		
		int line;
		char msg[13] = "System Armed";	//message for LCD
		
		line = 1;						//clear LCD lines
		LCD_clear_line(&line);
		line = 0;
		LCD_clear_line(&line);
		
		LCD_write_str(msg, &line);		//send message to LCD top line for 1s
		_delay_ms(1000);
	}
	else{					//if PIN has not been set, enter one
		pressedKey = 0xA;	//skip system startup message
		start();
	}
	return;
}

/*
 * Function:  disAlarm
 *  disables alarm if entered unlockPIN is the same as stored pin. Displays 
 *	error and returns to previous displayed message if alarm is not enabled or
 *	if entered unlockPIN does not match the stored pin.
 *
 *  returns:    none
 */
void disAlarm(){
	int incorrectPIN = 0;	//flag for if incorrect pin is entered
	int er = 0;				//flag for input pin error
	int LCDline;			//LCD line
	char msg[13];			//LCD message
	
	if(alarmEnable == 0){	//if alarm is not enabled, display error message 
							//and return
		
		LCDline = 1;						//clear LCD lines
		LCD_clear_line(&LCDline);
		LCDline = 0;
		LCD_clear_line(&LCDline);
		
		strcpy(msg, "System not armed");	//error message
		blinkMsg(msg);
		
		return;
	}
	
	er = enterCode(0);	//get unlockPIN from keypad input
	
	if(er == 1){		//if a bad code has been entered, (re)enable alarm
		enAlarm();
		return;
	}
	
	for(int i = 0; i < 4; i++){		//loop to check if pin matches unlockPIN
		if(pin[i] != unlockPIN[i])	//check each pin digit
			incorrectPIN = 1;		//incorrctPIN = 1 if incorrect pin is 
									//entered
	}
	
	if(incorrectPIN == 1){	//if incorrect pin is entered, display message and
							//(re)enable the alarm
		strcpy(msg, "Wrong PIN");
		blinkMsg(msg);		//display error message
		
		enAlarm();			//(re)enable the alarm
		return;
	}
	
	alarmEnable = 0;		//disable alarm
	
	LCDline = 1;					//clear LCD lines
	LCD_clear_line(&LCDline);
	LCDline = 0;
	LCD_clear_line(&LCDline);
	
	strcpy(msg, "Success");			//success message
	LCD_write_str(msg, &LCDline);	//send message to LCD top line for 1s
	_delay_ms(1000);
	return;
}

/*
 * Function:  selection
 *  calls start, enAlarm, and disAlarm functions based on current state of 
 *	system and input from keypad
 *
 *  returns:    none
 */
void selection(){
	if(PINset == 0){	//if pin is not set, call start function to indicate
						//no pin is set and get pin
		start();
		pressedKey = -1;	//clear pressedKey
		return;
	}
	displayMenu();		//display menu of alarm system options
	getNewKey();		//wait for user input
	
	if(pressedKey == 0xC){	//if "C" is pressed, call enterCode to set the PIN
		enterCode(1);
		pressedKey = -1;	//clear pressedKey
		return;
	}
	
	if(pressedKey == 0xA){	//if "A" is pressed, call enAlarm to set the alarm
		enAlarm();
		pressedKey = -1;	//clear pressedKey
		return;
	}
	
	if(pressedKey == 0xD){	//if "D" is pressed, call disAlarm to disable the
		disAlarm();			//alarm
		pressedKey = -1;	//clear pressedKey
		return;
	}
	
	return;	//return if no conditions are met
}

/*
 * Function:  displayMenu
 *  prints menu options to the LCD screen for user to interact with security
 *	system.
 *
 *  returns:    none
 */
void displayMenu(){
	//menu message
	//Change pin option entirely wraps to second line of LCD
	char str[32] = "A:Arm  D:Disarm C:Change Pin Num";
	
	//clear lines
	int line = 1;
	LCD_clear_line(&line);
	line = 0;
	LCD_clear_line(&line);
	
	//print message
	LCD_write_str(str, &line);
	return;
}

/*
 * Function:  displayStart
 *  prints scrolling startup message for system
 *
 *  returns:    none
 */
void displayStart(){
	char str[20] = "System Setup    ";	//startup message
	startScrollStr(str);				//scroll startup message
	return;
}