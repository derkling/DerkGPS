/*
  Part of the Wiring project - http://wiring.uniandes.edu.co

  Copyright (c) 2004-05 Hernando Barragan

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
  
  Modified 24 November 2006 by David A. Mellis
*/

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "at90can.h"

volatile static voidFuncPtr intFunc[EXTERNAL_NUM_INTERRUPTS];

void attachInterrupt(uint8_t interruptNum, void (*userFunc)(void), int mode) {
  if(interruptNum < EXTERNAL_NUM_INTERRUPTS) {
    intFunc[interruptNum] = userFunc;

    switch (interruptNum) {
    case 0:
        EICRA = (EICRA & ~(_BV(ISC00)|_BV(ISC01))) | (mode << ISC00);
        EIMSK |= (1 << INT0);
        break;
    case 1:
        EICRA = (EICRA & ~(_BV(ISC00)|_BV(ISC11))) | (mode << ISC10);
        EIMSK |= (1 << INT1);
        break;
    case 2:
        EICRA = (EICRA & ~(_BV(ISC00)|_BV(ISC21))) | (mode << ISC20);
        EIMSK |= (1 << INT2);
        break;
    case 3:
        EICRA = (EICRA & ~(_BV(ISC00)|_BV(ISC31))) | (mode << ISC30);
        EIMSK |= (1 << INT3);
        break;

// NOTE that recognition of falling or rising edge interrupts on INT7:4 requires
// the presence of an I/O clock,

    case 4:
        EICRB = (EICRB & ~(_BV(ISC40)|_BV(ISC41))) | (mode << ISC40);
        EIMSK |= (1 << INT4);
        break;
    case 5:
        EICRB = (EICRB & ~(_BV(ISC50)|_BV(ISC51))) | (mode << ISC50);
        EIMSK |= (1 << INT5);
        break;
    case 6:
        EICRB = (EICRB & ~(_BV(ISC60)|_BV(ISC61))) | (mode << ISC60);
        EIMSK |= (1 << INT6);
        break;
    case 7:
        EICRB = (EICRB & ~(_BV(ISC70)|_BV(ISC71))) | (mode << ISC70);
        EIMSK |= (1 << INT7);
        break;
    }

}

void detachInterrupt(uint8_t interruptNum) {

  if(interruptNum < EXTERNAL_NUM_INTERRUPTS) {
    switch (interruptNum) {
    case 0:
        EIMSK &= ~_BV(INT0);
        break;
    case 1:
        EIMSK &= ~_BV(INT1);
        break;
    case 2:
        EIMSK &= ~_BV(INT2);
        break;
    case 3:
        EIMSK &= ~_BV(INT3);
        break;
    case 4:
        EIMSK &= ~_BV(INT4);
        break;
    case 5:
        EIMSK &= ~_BV(INT5);
        break;
    case 6:
        EIMSK &= ~_BV(INT6);
        break;
    case 7:
        EIMSK &= ~_BV(INT7);
        break;
    }

    intFunc[interruptNum] = 0;
  }
}

#define SIG_INTERRUPT_INSTALL(_num_)	\
SIGNAL(SIG_INTERRUPT##_num_##) {	\
  if(intFunc[EXTERNAL_INT_##_num_##])	\
    intFunc[EXTERNAL_INT_##_num_##]();	\
}

SIG_INTERRUPT_INSTALL(0);
SIG_INTERRUPT_INSTALL(1);
SIG_INTERRUPT_INSTALL(2);
SIG_INTERRUPT_INSTALL(3);
SIG_INTERRUPT_INSTALL(4);
SIG_INTERRUPT_INSTALL(5);
SIG_INTERRUPT_INSTALL(6);
SIG_INTERRUPT_INSTALL(7);
