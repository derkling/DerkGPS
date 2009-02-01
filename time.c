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

// The number of times timer 1 has overflowed since the program started.
// Must be volatile or gcc will optimize away some uses of it.
volatile unsigned long timer1_overflow_count;

unsigned long millis(void) {
	// timer 1 increments every 64 cycles, and overflows when it reaches
	// 256.  we would calculate the total number of clock cycles, then
	// divide by the number of clock cycles per millisecond, but this
	// overflows too often.
	//return timer0_overflow_count * 64UL * 256UL / (F_CPU / 1000UL);
	
	// instead find 1/128th the number of clock cycles and divide by
	// 1/128th the number of clock cycles per millisecond
	return timer0_overflow_count * 64UL * 2UL / (F_CPU / 128000UL);
}

void delay(unsigned long ms) {
	unsigned long start = millis();
	
	while ( (millis()-start) < ms )
		;
}

void initTime(void) {
	
	// this needs to be called before setup() or some functions won't
	// work there
	sei();
	
	// timer 1 is used for millis() and delay()
	timer1_overflow_count = 0;
	
	// set timer 1 prescale factor to 64
	sbi(TCCR0, CS01);
	sbi(TCCR0, CS00);

	// enable timer 0 overflow interrupt
	sbi(TIMSK, TOIE0);

}


//----- Interrupt handlers

SIGNAL(SIG_OVERFLOW0) {
	timer0_overflow_count++;
}

