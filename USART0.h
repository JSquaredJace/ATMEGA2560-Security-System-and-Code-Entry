/*
 * USART0.h
 *
 * header file to control LCD screen
 * Author : Jace Johnson
 * Hardware:	ATMega 2560 operating at 16 MHz
 *				a device to communicate with
 * configuration: TX0 connected to RX of other device and RX0 connected to TX
 *				  of other device.
 *
 * tested with USB ports on micro controller and a computer connected via USB 
 * male A to USB male B chord. Communication done using an SSH client (PuTTY)
 * on computer
 */

#ifndef USART0_H
#define USART0_H

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define USART_BAUDRATE 57600	//set baud rate
#define BAUD_PRESCALE F_CPU / (USART_BAUDRATE * 16UL) - 1

//function prototypes for file pointers
int uart_putchar0(char c, FILE* stream);
int uart_getchar0(void);

static FILE USART0_OUT = FDEV_SETUP_STREAM(uart_putchar0, NULL,
_FDEV_SETUP_WRITE);
static FILE USART0_IN = FDEV_SETUP_STREAM(NULL, uart_getchar0,
_FDEV_SETUP_READ);

/*
 * Function:	uart_putchar0
 *	Function to send chars to USART0. Used to setup USART0_OUT as a file
 *	pointer to the serial port so that data can be sent there.
 *
 *	c		char	character to be transmitted through the serial line
 *	stream	FILE*	pointer for USART0 output
 *
 *  returns:	0	successful function run
 */
int uart_putchar0(char c, FILE* stream){
	if(c == '\n') uart_putchar0('\r', stream);	//change newlines to return 
												//carriage
	loop_until_bit_is_set(UCSR0A, UDRE0);		//wait until transmitter is 
												//ready for new data
	UDR0 = c;									//transmit next character
	return 0;
}

/*
 * Function:	uart_getchar0
 *	Function to get chars from USART0. Used to setup USART0_IN as a file 
 *	pointer to the serial port so that data can be received from USART0.
 *
 *  returns:	int	value recieved from serial communication port, USART0
 */
int uart_getchar0(void){
	uint8_t temp;
	while(!(UCSR0A & (1 << RXC0)));	//wait for serial value to enter UDR
	temp = UDR0;					//get value from UDR and return it
	return temp;
}

/*
 * Function:	InitUSART0
 *	Sets up USART0 to use a baudrate equal to the global constant 
 *	USART_BAUDRATE, and use 8 bit character frames and async mode.
 *
 *  returns:	none
 */
void initUSART0(){
	UCSR0B |= 0x18;	//Enable RX and TX
	UCSR0C |= 0x06;	//Use 8 bit character frames in async mode
	
	//set baud rate (upper 4 bits should be zero)
	UBRR0L = BAUD_PRESCALE;
	UBRR0H = (BAUD_PRESCALE >> 8);
	return;
}

#endif