/*
  testport.c - Simple test for platform porting

  Copyright (c) 2008-2009 Patrick Bellasi

  This is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA

*/

#include <string.h>

#include "at90can.h"
#include "serials.h"

#define OUTPUT_BUFFER_SIZE	47

// ATControl (UART_AT)
#define Serial_print(C)			print(UART_AT, C)
#define Serial_printStr(STR)		printStr(UART_AT, STR)
#define Serial_printLine(STR)		printLine(UART_AT, STR)
#define Serial_printFlush()		flush(UART_AT)
#define SERIAL_NEWLINE	"\r\n"
#define Serial_readLine(BUFF, LEN)	readLine(UART_AT, BUFF, LEN)

// Top-Halves serials interrupt names
#define INTR_GPS	UART_GPS
#define INTR_CMD	UART_AT

uint8_t led1		=   5;	// Running DERKGPS_DEBUG led	(using PA5)
uint8_t led2		=   6;	// Running DERKGPS_DEBUG led	(using PA6)
uint8_t led3		=   7;	// Running DERKGPS_DEBUG led	(using PA7)

unsigned int i = 0;
unsigned int j = 0;

//----- System Initialization
void setup(void) {

	// Disable interrupts (just in case)
	cli();
	
	// Disable JTAG interface
	// This require TWICE writes in 4 CPU cycles
	sbi(MCUCR, JTD);
	sbi(MCUCR, JTD);

	// Inputs:
	//	(on DVK board ) KBD (PE4-7)
	cbi(DDRE,PE7);
	
	// Outpust:
	//	LEDS(PC0-4), Inputs(HiZ) INT_REQ(PA0), GPG PowerUp(PA1), USB Reset(PA2)
	//	(on DVK board ) LEDS(PA0-7)
	DDRA	|= 0xFF;
	sbi(DDRC,PC7);
	sbi(DDRD,PD7);
	sbi(DDRE,PE3);
	
	// Disabling Pull-Ups for Inputs
	PORTA	= 0x00;
	PORTE	= 0x00;
	
	// Configure timers
	initTime();
	
	// Configure UART ports
	initSerials();
	
	// Enable interrupts
	sei();

	// LED1 control
	pinMode(led1, OUTPUT);
	pinMode(led2, OUTPUT);
	pinMode(led3, OUTPUT);
	
	digitalWrite(led1, HIGH);
// 	digitalWrite(led2, HIGH);
// 	i=0; j=0;
// 	while( ++i < 0xFFFF ) {
// 	    while( ++j < 0xFFFF );
// 	}
	digitalWrite(led2, HIGH);
	
	sbi(PORTA, PORTA0);
	sbi(PORTA, PORTA1);
	sbi(PORTA, PORTA2);
	sbi(PORTA, PORTA3);
	
}

void loop(void) {

// 	while( ++i < 0xFFFF ) {
// 	    while( ++j < 0xFFFF );
// 	}
	delay(1000);
	
	digitalSwitch(led1);

}

int main(void) {

	setup();
	
	Serial_printLine("AT90CAN Test, by Patrick Bellasi");
	
	while(1) {
		loop();
	}
	
}


