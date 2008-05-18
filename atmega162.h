/*
  sys_atmage162.h - ATmega162 System Definitions

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

#ifndef atmega162_h
#define atmega162_h

#include <stdlib.h>
#include <stdio.h>

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>

#include "binary.h"

// Define the uC
#define ATMEGA162

// Oscillator-frequency in Hz
#define F_CPU 8000000

#ifdef __cplusplus
extern "C"{
#endif

#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1

#define true 0x1
#define false 0x0

#define PI 3.14159265
#define HALF_PI 1.57079
#define TWO_PI 6.283185

#define LSBFIRST 0
#define MSBFIRST 1

#define INTR0	0
#define INTR1	1

#define LOW_LEVEL	0x0
#define CHANGE		0x1
#define FALLING		0x2
#define RISING		0x3

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#ifndef abs
# define abs(x) ((x)>0?(x):-(x))
#endif
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )

#ifndef cbi
# define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
# define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// EXTERNAL INTERRUPTS: INT0..2, and PCINT0..15
#define EXTERNAL_INT_0 0
#define EXTERNAL_INT_1 1
// NOTE INT2 is onle EDGE triggered!!!
#define EXTERNAL_INT_2 XX

#define EXTERNAL_NUM_INTERRUPTS 3

typedef void (*voidFuncPtr)(void);

// defined on digitals.c
void	pinMode(uint8_t, uint8_t);
void	digitalWrite(uint8_t, uint8_t);
void	digitalSwitch(uint8_t pin);
int	digitalRead(uint8_t);

// defined on interrupts.c
void	attachInterrupt(uint8_t, void (*)(void), int mode);
void	detachInterrupt(uint8_t);

// defined on time.c
void		initTime(void);
unsigned long	millis(void);
void		delay(unsigned long ms);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
