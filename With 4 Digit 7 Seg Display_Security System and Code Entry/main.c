/*
 * Lab04 - Security System and Code Entry
 * Author : Jace Johnson
 * Created: 2/21/2020 10:06:54 AM
 *	Rev 1	2/21/2020
 * Description:	ATMEGA2560 emulates a home security keypad with a 4 digit
 *				display. It does this by taking information from a keypad
 *				matrix and outputting prompts and other information to the
 *				4 digit 7 segment display.
 * 
 * Hardware:	ATMega 2560 operating at 16 MHz
 *				four digit, seven segment display
 *				4x4 keypad module
 *				four 330 Ohm resistors
 *				jumper wires
 * Configuration shown below
 *
 *  ATMega 2560		   7 segment
 *   PORT  pin			Display
 *	----------		   ----------	Display Pin, PORTA Bit
 *	| A0   22|---------|2		|		 _________		
 *	| A1   23|---------|4		|		|  11, A4 |		
 *	| A2   24|---------|5		|		|		  |7, A3
 *	| A3   25|---------|7		|		|10, A6	  |		
 *	| A4   26|---------|11		|		|_________|		
 *	| A5   27|---------|1		|		|  5, A2  |
 *	| A6   28|---------|10		|		|		  |4, A1
 *	| A7   29|---------|3		|		|1, A5	  |     _
 *	| B3   50|-330 Ohm-|6		|		|_________|    |_|
 *	| B2   51|-330 Ohm-|8		|	       2, A0	  3, A7
 *	| B1   52|-330 Ohm-|9		|
 *	| B0   53|-330 Ohm-|12		|
 *	|		 |		   ----------
 *	|		 |
 *	|		 |		     Keypad
 *	|		 |		   ----------				  4 Digit
 *	| C0   37|---------|4		|				Control Pins
 *	| C1   36|---------|5		|				 _  _  _  _
 *	| C2   35|---------|6		|				|_||_||_||_|
 *	| C3   34|---------|7		|				|_||_||_||_|
 *	| C4   33|---------|1		|	  PORTB Bit: 0  1  2  3
 *	| C5   32|---------|2		|	Display Pin: 12 9  8  6 
 *	| C6   31|---------|3		|	Writing a pin high turns the digit off
 *	| C7   30|---------|4		|	Writing a pin low turns the digit on
 *	----------		   ----------
 *
 */ 

#define F_CPU 16000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <util/delay.h>

void init();
void initializePorts();
void initializeTimers();
void fillNumArray();
int decodeChar(char c);
void setEnterCode();
void setSuccess();
void setAlarm();
void setError();
void setEnterPIN();
void setStart();
void setPIN();
void setTimer3(int t);
void disableTimer1();
void enableTimer1Blink500();
void err();
void updateDisplay();
void checkNumPad();
int readNumPad();
void start();
int enterCode();
int fillPIN(int index, int changePIN);
void succPIN();
void enAlarm();
void disAlarm();
void selection();


int pin[4] = {-1, 0, 0, 0};			//stored pin number for arming system
int unlockPIN[4] = {-2, 0, 0, 0};	//unlock pin number for unlocking system
						
int input = 0;			//stores input from number pad
int timer3Finish = 0;	//1 when timer 0 finishes
int displayOff = 0x0;	//1 turns display off
int keypadClear = 1;	//1 when keypad buttons are unpressed

int alarmEnable = 0;	//1 when alarm is enabled
int PINset = 0;			//1 when pin has been set

int digits[4] = {20, 20, 20, 20};	//stores digits of display from left to
									//right
int displayNums[22];	
//index values correspond with displayed character when array value is 
//outputted to PORTA. Display characters in order: 0 - 9, A, C, D, E, I, L, P,
//R, S, U, all segments, and no segments.

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
	init();		//initialize ports, timers, and fill displayNums array
	
    while (1) {				//forever loop
		updateDisplay();	//display characters in digits
		selection();		//select option
    }
	return 0;
}

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

/*
 * ISR:  TIMER1_OVF_vect
 *  Interrupt for timer1 overflow. Toggles the display on and off every 0.5 s,
 *	i.e., causes the display to "blink"
 *
 *  returns:    none
 */
ISR(TIMER1_OVF_vect) {
	displayOff ^= 0x1;	//toggle whether the display is on or off
	TCNT1 = 65536 - (int)((500.0 * 16000.0) / 256.0);   //set timer for 0.5 s
	return;
}

/*
 * ISR:  TIMER3_OVF_vect
 *  Interrupt for timer3 overflow. Sets timer3Finish = 1 and turns off timer3
 *
 *  returns:    none
 */
ISR(TIMER3_OVF_vect) {
	timer3Finish = 1;	//indicates timer 3 has ended
	TCCR3B &= 0xF8;		//turns timer off
	return;
}

/*
 * Function:  init
 *  Calls functions to initialize PORTs A, B, and C and timers 0, 1, and 3 and
 *	fill the displayNums array
 *
 *  returns:    none
 */
void init(){
	initializePorts();		//initialize PORTs A, B, and C
	initializeTimers();		//initialize timers 0, 1, and 3
	fillNumArray();			//fill display value array
	
	return;
}

/*
 * Function:  initializePorts
 *  Sets input and output pins for PORTs A, B, and C. Sets PORTC outputs high
 *	and activates pull up resistors for PORTC inputs.
 *
 *  returns:    none
 */
void initializePorts(){
	DDRA = 0XFF;	//set PORTA as output
	DDRB = 0xFF;	//set PORTB as input
	DDRC = 0x0F;	//set low nybble as output and high nybble as input
	
	PORTC = 0xFF;	//set PORTC outputs high and enable pull up resistors for
					//inputs
	return;
}

/*
 * Function:  initializeTimers
 *  Sets input and output pins for PORTs A, B, and C. Sets PORTC outputs high
 *	and activates pull up resistors for PORTC inputs.
 *
 *  returns:    none
 */
void initializeTimers(){
	TCCR0A = 0x00;
	TCCR0B |= (1<<CS12);	//turn timer0 on with 256 prescaler

	TCCR1A = 0x00;
	TCCR1B &= 0xF8;			//turn timer1 off
	
	TCCR3A = 0x00;
	TCCR3B &= 0xF8;			//turn timer3 off
	
	sei();		//Enable global interrupts by setting global interrupt enable
				//bit in SREG
	
	TIMSK0 = (1 << TOIE0);		//enable timer0 overflow interrupt(TOIE0)
	TCNT0 = 65536 - 1;			//timer0 overflow in one clock cycle
	
	TIMSK1 = (1 << TOIE1);		//enable timer1 overflow interrupt(TOIE1)
	TIMSK3 = (1 << TOIE3);		//enable timer3 overflow interrupt(TOIE3)
	
	return;
}

/*
 * Function:  fillNumArray
 *  Fills displayNums array with values to be outputted to PORTA. Array values
 *	can be outputted to PORTA to display different characters on the seven 
 *	segment display. Display characters in order: 0 - 9, A, C, D, E, I, L, P,
 *	R, S, U, all segments, and no segments.
 *
 *  returns:    none
 */
void fillNumArray(){
	displayNums[0] = 0x7B;	//0
	displayNums[1] = 0x0A;	//1
	displayNums[2] = 0x3D;	//2
	displayNums[3] = 0x1F;	//3
	displayNums[4] = 0x4E;	//4
	displayNums[5] = 0x57;	//5
	displayNums[6] = 0x77;	//6
	displayNums[7] = 0x1A;	//7
	displayNums[8] = 0x7F;	//8
	displayNums[9] = 0x5F;	//9
	displayNums[10] = 0x7E;	//A
	displayNums[11] = 0x71;	//C
	displayNums[12] = 0x2F;	//D
	displayNums[13] = 0x75;	//E
	displayNums[14] = 0x60;	//I 
	displayNums[15] = 0x61;	//L
	displayNums[16] = 0x7C;	//P
	displayNums[17] = 0x24;	//R
	displayNums[18] = 0x57;	//S
	displayNums[19] = 0x6B;	//U
	displayNums[20] = 0xFF;	//all LEDs
	displayNums[21] = 0x00;	//no LEDs
	
	return;
}

/*
 * Function:  decodeChar
 *  Outputs index value for displayNums that corresponds to the input 
 *	character c
 *	
 *	c	input character that corresponds to a character that displays when 
 *	displayNums[index] is outputted to PORTA
 *
 *  returns:    int		index value that corresponds to input char
 *				20		default case for unknown input
 */
int decodeChar(char c){
	switch(c){
		case 'a':
		case 'A':
			return 10;
		case 'c':
		case 'C':
			return 11;
		case 'd':
		case 'D':
			return 12;
		case 'e':
		case 'E':
			return 13;
		case 'i':
		case 'I':
			return 14;
		case 'l':
		case 'L':
			return 15;
		case 'p':
		case 'P':
			return 16;
		case 'r':
		case 'R':
			return 17;
		case 's':
		case 'S':
			return 18;
		case 'u':
		case 'U':
			return 19;
		default:
			return 20;
		
	}
	return 20;
}

/*
 * Function:  setEnterCode
 *  Sets digits for seven segment display to display ECDE for enter code
 *
 *  returns:    none
 */
void setEnterCode(){
	digits[0] = decodeChar('e');	//E
	digits[1] = decodeChar('c');	//C
	digits[2] = decodeChar('d');	//D
	digits[3] = decodeChar('e');	//E
	return;
}

/*
 * Function:  setSuccess
 *  Sets digits for seven segment display to display SUCC for success
 *
 *  returns:    none
 */
void setSuccess(){
	digits[0] = decodeChar('s');	//S
	digits[1] = decodeChar('u');	//U
	digits[2] = decodeChar('c');	//C
	digits[3] = decodeChar('c');	//C
	return;
}

/*
 * Function:  setAlarm
 *  Sets digits for seven segment display to display ALAR for alarm
 *
 *  returns:    none
 */
void setAlarm(){
	digits[0] = decodeChar('a');	//A
	digits[1] = decodeChar('l');	//L
	digits[2] = decodeChar('a');	//A
	digits[3] = decodeChar('r');	//R
	return;
}

/*
 * Function:  setError
 *  Sets digits for seven segment display to display ERR for error
 *
 *  returns:    none
 */
void setError(){
	digits[0] = decodeChar('e');	//E
	digits[1] = decodeChar('r');	//R
	digits[2] = decodeChar('r');	//R
	digits[3] = 21;					//no segments (blank)
	return;
}

/*
 * Function:  setEnterPIN
 *  Sets digits for seven segment display to display ERPI for enter PIN
 *
 *  returns:    none
 */
void setEnterPIN(){
	digits[0] = decodeChar('e');	//E
	digits[1] = decodeChar('r');	//R
	digits[2] = decodeChar('p');	//P
	digits[3] = decodeChar('i');	//I
	return;
}

/*
 * Function:  setStart
 *  Sets digits for seven segment display to display all segments
 *
 *  returns:    none
 */
void setStart(){
	digits[0] = 20;		//all segments
	digits[1] = 20;		//all segments
	digits[2] = 20;		//all segments
	digits[3] = 20;		//all segments
}

/*
 * Function:  setPIN
 *  Sets digits for seven segment display to the numbers stored as the pin
 *
 *  returns:    none
 */
void setPIN(){
	for(int i = 0; i < 4; i++){	//loop through each digit
		digits[i] = pin[i];		//set digits number equal to pin number
	}
	return;
}

/*
 * Function:  setTimer3
 *  Enables timer3 and sets it to overflow after input t milliseconds. This
 *	triggers an ISR to disable timer3 and set timer3finish equal to 1.
 *
 *	t	timer3 will overflow after t milliseconds
 *
 *  returns:    none
 */
void setTimer3(int t){
	
	timer3Finish = 0;		//timer3Finish set to 0. var gets set to 1 when 
							//timer3 overflows
	TCCR3B = (1<<CS12);		//timer3 turn on with 256 prescaler
	
	//timer3 set to a value that causes it to overflows after t milliseconds
	TCNT3 = 65536 - (int)(((double)t * 16000.0) / 256.0);	
	return;
}

/*
 * Function:  disableTimer1
 *  turns off timer1 (turns off display blinking)
 *
 *  returns:    none
 */
void disableTimer1(){
	TCCR1B &= 0xF8;		//timer1 mode off
	displayOff = 0x0;	//turn display on
	return;
}

/*
 * Function:  enableTimer1Blink500
 *  Enables timer1 and sets it to overflow after 500 milliseconds. This
 *	triggers an ISR to reset timer1 to overflow after 500 milliseconds and
 *	toggle displayOff. This causes the display to blink on and off every
 *	second (on for 0.5 seconds, off for 0.5 seconds).
 *
 *  returns:    none
 */
void enableTimer1Blink500(){
	TCCR1B |= (1<<CS12);  //timer1 on with 256 prescaler
	
	//set timer1 to overflow after 0.5 sec
	TCNT1 = 65536 - (int)((500.0 * 16000.0) / 256.0);   
	return;
}

/*
 * Function:  err
 *  makes the display blink ERR on and off every second for five seconds
 *
 *  returns:    none
 */
void err(){
	setError();			//set digits to display ERR
	displayOff = 0x0;	//turn display on
	
	for(int i = 0; i < 5; i++){		//loop through 5 times
		setTimer3(500);				//set timer3Finish to become 1 after 0.5 s
		while(timer3Finish == 0){	//display ERR for 0.5 s
			updateDisplay();
		}
		
		PORTC |= 0x0F;		//turn off all digits in display
		_delay_ms(500);		//delay 0.5 s with display off
	}
	return;
}

/*
 * Function:  updateDisplay
 *  toggle though each seven segment display digit and display the 
 *	corresponding digit stored in the digits array. Displays values from 
 *	digits array on seven segment display.
 *
 *  returns:    none
 */
void updateDisplay(){
	PORTA = 0x00;		//clear PORTA
	
	if(displayOff == 0x1){	//if display should be off, exit
		PORTB |= 0x0F;		//set all digits high (turn off all digits)
		return;
	}
	
	//loop to turn off all digits, update the displayed number, and turn on
	//the next digit
	for(int i = 0; i < 4; i++){
		PORTB |= 0x0F;			//set all digits high (turn off all digits)
		
		PORTA = displayNums[digits[i]];	//display stored digit for the current
										//location
		
		//turn on current digit on display based on i value
		switch(i){
			case 0:
			PORTB &= 0xFE;	//turn on first digit
			break;
			case 1:
			PORTB &= 0xFD;	//turn on second digit
			break;
			case 2:
			PORTB &= 0xFB;	//turn on third digit
			break;
			case 3:
			PORTB &= 0xF7;	//turn on fourth digit
			break;
		}
	}
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

/*
 * Function:  start
 *  blinks all segments in display to indicate no pin has been set. Calls
 *	enterCode function to set pin if "A" or "C" buttons are pressed.
 *
 *  returns:    none
 */
void start(){
	setStart();				//set display to show turn segments on
	enableTimer1Blink500();	//turn on timer to make display blink every second

	while(!((input == 0xA) || (input == 0xC))){	//loop to show display
												//when "C" or "A" is pressed
		updateDisplay();	//display digits on display
	}
	
	disableTimer1();	//turn off blinking timer
	
	enterCode(1);		//set pin number based on input from keypad
	
	return;
}

/*
 * Function:  enterCode
 *  sets pin or unlock pin based on input from keypad and value of input param
 *	changePIN
 *
 *	changePIN	1 changes pin array
 *				0 changes unlockPIN array
 *
 *  returns:    1 exit if called by disAlarm command
 *				0 exited normally
 */
int enterCode(int changePIN){
	int finish = 0;	//var for when pin or unlockPIN is set without error
	int index;		//index value for fillPIN function
	int next = 1;	//var for when PIN is ready for next input
	int errr = 0;	//error flag
	int strt = 1;	//var for first run through loop
	
	while(finish != 1){	//loop that exits when pin or unlockPIN is set without
						//error
		errr = 0;	//set error to 0
		input = -1;	//set input to be blank
		
		if(changePIN == 1)	//check if pin or unlockPIN is being set
			setEnterCode();	//display enter code if pin is being set
		else
			setEnterPIN();	//display enter PIN of unlockPIN is being set
			
		if(strt == 1){	//check if this is the first run through the loop
			index = -1;	//set index value to -1 if this is the first run 
						//through loop
			strt = 0;	//set strt to 0 to indicate first run through loop
						//has happened
		}
		else{
			index = 0;	//set index to 0 if not first run through loop
		}
		
		enableTimer1Blink500();	//enable blinking display
		
		while(input != 0xE){	//loop for storing input from keypad in pin or
								//unlockPIN. Exit when "#" is pressed
			updateDisplay();	//display digits on display
			
			if((keypadClear == 1)){	//if keypad buttons are not pressed, set
									//next to 1 (indicating next pin/unlockPIN
									//digit is ready to set)
				next = 1;
			}
			
			//if button is pressed on keypad and next digit in pin/unlockPIN
			//is ready to be set
			if((keypadClear == 0) && (next == 1)){
				//set next to 0 to prevent multiple pin/unlockPIN digits set 
				//to same input value
				next = 0;	
				//fill pin or unlockPIN digit based on index value and changePIN value
				errr = fillPIN(index, changePIN);	
				//increment index value
				index++;
			}
		}
		disableTimer1();	//disable display blinking
		
		if(index != 5)	//error if wrong number of buttons were pressed
			errr = 1;
			
		if(errr != 0){	//blink error 5 times if error has occurred, then 
						//repeat loop
			err();
			if(changePIN == 0){
				return 1;
			}
		}
		else{
			finish = 1;	//exit loop if no errors have occurred
		}
	}
	succPIN();			//display successful pin set
	return 0;
}

/*
 * Function:  fillPIN
 *  sets pin or unlock pin digit based on input from keypad and value of input
 *	params changePIN and index
 *
 *	changePIN	1 changes pin array
 *				0 changes unlockPIN array
 *	index		sets which digit will be changed in pin/unlockPIN to the value
 *				input from the keypad
 *
 *  returns:    0	no errors
 *				1	index value too high
 *				2	input is not a number
 */
int fillPIN(int index, int changePIN){
	if(input == -1)		//if input value is blank (represented as -1) then exit
		return 0;
		
	if(index > 4)		//if index value exceeds 4 then exit with error
		return 1;
		
	if(index > 3)		//if index value exceeds 3 then exit
		return 0;
	
	
	if((input > 0x9) && (input != 0x10))	//if button pressed is not a 
		return 2;							//number then exit
	
	if(changePIN == 1){			//check if pin or unlockPIN should be changed
		setEnterCode();			//set display to show enter code if pin should
								//be changed
		
		if(input == 0x10)		//set pin digit to 0 if 0 is pressed
			pin[index] = 0;
		else
			pin[index] = input;	//set pin to input value otherwise
	}
	else{						//code for if unlockPIN should be changed
		setEnterPIN();			//set display to show enter PIN
		if(input == 0x10)		//set unlockPIN digit to 0 if 0 is pressed
			unlockPIN[index] = 0;
		else
			unlockPIN[index] = input;	//set unlockPIN to input value otherwise
	}
	
	return 0;	//no errors occurred
}

/*
 * Function:  succPIN
 *  displays pin for two seconds, then blinks pin once, then displays success 
 *	for successful pin
 *
 *  returns:    none
 */
void succPIN(){
	setPIN();			//display pin
	displayOff = 0x0;	//turn display on
	
	for(int i = 0; i < 2; i++){	//loop to leave display on for 2 seconds
		setTimer3(1000);		//set timer3 for 1 sec and loop twice
								//display pin for 2 sec
		while(timer3Finish == 0){	//display pin until timer3 has overflowed
			updateDisplay();
		}
	}
	PORTA = 0x00;		//turn off display
	
	_delay_ms(500);		//wait 0.5 sec with display off
	
	setTimer3(500);		//set timer3 for 0.5 sec
						//blink pin for 0.5 sec
	while(timer3Finish == 0){	//display pin until timer3 has overflowed
		updateDisplay();
	}
	
	PORTA = 0x00;		//turn off display
	_delay_ms(500);		//wait 0.5 sec with display off
	
	setSuccess();		//set display to shou successful pin has been set
	
	while(keypadClear == 1){	//display success until a button is pressed
		updateDisplay();
	}
	
	PINset = 1;			//var to indicate PIN has been set
	return;
}

/*
 * Function:  enAlarm
 *  enables the alarm
 *
 *  returns:    none
 */
void enAlarm(){
	if(PINset == 1){	//if PIN has been set, arm alarm system
		alarmEnable = 1;	//set alarm enable flag
		setAlarm();			//set display to show alarm
	}
	else{				//if PIN has not been set, enter one
		enterCode(1);	//enter PIN and enable system
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
	int incorrectPIN = 0;	//var for if incorrect pin is entered
	int ex = 0;
	
	if(alarmEnable == 0){	//if alarm is not enabled, error and return to 
							//previous display
		err();			//blink error for 5 seconds
		setSuccess();	//display success (previous display)
		return;
	}
	
	ex = enterCode(0);	//enter code function to get unlockPIN from keypad input
	
	if(ex == 1){		//if exit is returned from enterCode, clear input and
						//return to previous display state
		setAlarm();		//display alarm (previous display state)
		return;
	}
	
	for(int i = 0; i < 4; i++){		//loop to check if pin matches unlockPIN
		if(pin[i] != unlockPIN[i])	//check each pin digit
			incorrectPIN = 1;		//indirectPIN = 1 if incorrect pin is 
									//entered
	}
	
	if(incorrectPIN == 1){	//if incorrect pin is entered, display error and
							//display alarm (previous display state)
		err();				//blink error for 5 seconds
		setAlarm();			//display alarm (previous display)
		return;
	}
	
	alarmEnable = 0;	//disable alarm
	PINset = 0;			//pin is no longer set
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
						//no pin is set
		start();
		input = -1;		//clear input
		return;
	}
	
	if(input == 0xC){	//if "C" is pressed, call enterCode to set the PIN
		enterCode(1);
		input = -1;		//clear input
		return;
	}
	
	if(input == 0xA){	//if "A" is pressed, call enAlarm to set the alarm
		enAlarm();
		input = -1;		//clear input
		return;
	}
	
	if(input == 0xD){	//if "D" is pressed, call disAlarm to disable the alarm
		disAlarm();
		input = -1;		//clear input
		return;
	}
	
	return;	//return if no conditions are met
}
