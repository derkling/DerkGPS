/*
  time.c

  Copyright (c) 2008-2009 Patrick Bellasi

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA

*/

#include "at90can.h"

// The number of times timer 0 has overflowed since the program started.
// Must be volatile or gcc will optimize away some uses of it.
volatile unsigned long timer1_overflow_count = 0;

// The timestamp of last and previous pulses on ICP
// Must be volatile or gcc will optimize away some uses of it.
volatile unsigned int timer1_prev_ts = 0;

unsigned long millis(void) {
	unsigned long base = 0;
	unsigned long delta = 0;
	unsigned long t = 0;
#if 0
	// timer 0 increments every 64 cycles, and overflows when it reaches
	// 256.  we would calculate the total number of clock cycles, then
	// divide by the number of clock cycles per millisecond, but this
	// overflows too often.
	//return timer0_overflow_count * 64UL * 256UL / (F_CPU / 1000UL);
	
	// instead find 1/128th the number of clock cycles and divide by
	// 1/128th the number of clock cycles per millisecond
	return timer0_overflow_count * 64UL * 2UL / (F_CPU / 128000UL);
#endif
	//(COUNT * 256UL) / (F_CPU / 1000UL )
	// This will overflow everty 2.097152 s
	base = timer1_overflow_count;
	delta = TCNT1;
	
	t = (delta * 16UL) / (F_CPU / 64000UL);
	t = t + (base * 8192UL);
	return t;
	
}

void delay(unsigned long ms) {
	unsigned long start = millis();
	
	while ( (millis()-start) < ms )
		;
}

// NOTE This function MUST be called with interrupt disabled
void initTime(void) {
	
	// timer 1 is used for millis() and delay()
	timer1_overflow_count = 0;
	
	//  Input Capture Noise Canceler (ENABLE)
	sbi(TCCR1B, ICNC1);
	
	//  Input Capture Edge Select (RAISING EDGE)
	sbi(TCCR1B, ICES1);
	
	// Clock Select (PRESCALER @ 1024)
	//  F_CPU: 8MHz		===>	F_PSC: 7,8125 KHz
	//  T_CPU: 0,125us	===>	T_PSC: 128us
	sbi(TCCR1B, CS12);
	cbi(TCCR1B, CS11);
	sbi(TCCR1B, CS10);
	
	// Configure Output Compare Register A (TOP Value)
	// 64000*T_PSC = 0xFA00*T_PSC ===> 8192ms = 0x2000
	OCR1AH = 0xFA;
	OCR1AL = 0x00;
	
	// Configuring "Compare Output Mode for Channel A" to Normal (disconnected PIN)
	cbi(TCCR1A, COM1A1);
	cbi(TCCR1A, COM1A0);
	
	// Enable Clear Timer on Comapre Match Mode
	cbi(TCCR1B, WGM13);
	sbi(TCCR1B, WGM12);
	cbi(TCCR1A, WGM11);
	cbi(TCCR1A, WGM10);
	
	// Enable Output Compare Match Interrupt A
	// This will set 1 into OCF1A in register TIFR1
	sbi(TIMSK1, OCIE1A);
	// Disable timer 1 overflow interrupt
	cbi(TIMSK1, TOIE1);
	// Enable Input CaPture Interrupt
	sbi(TIMSK1, ICIE1);

}

//----- Interrupt handlers

// Output Compare Match Interrupt A
SIGNAL(SIG_OUTPUT_COMPARE1A) {
	timer1_overflow_count++;
	
// 	if ( timer1_overflow_count & 0x1 ) {
// 		sbi(PORTA, PA7);
// 	} else {
// 		cbi(PORTA, PA7);
// 	}
	
}

// Input CaPtude Interrupt
SIGNAL(SIG_INPUT_CAPTURE1) {
	timer1_prev_ts = ICR1;
    
// 	if ( PORTA & 0x1 ) {
// 		cbi(PORTA, PA0);
// 	} else {
// 		sbi(PORTA, PA0);
// 	}
       
}
       


