/*
  pins_arduino.c - pin definitions for the Arduino board
  Part of Arduino / Wiring Lite

  Copyright (c) 2005 David A. Mellis

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

  $Id: pins_arduino.c 254 2007-04-20 23:17:38Z mellis $
*/

#include <avr/io.h>
#include "pins.h"

#ifdef ATMEGA162
// On the Arduino board, digital pins are also used
// for the analog output (software PWM).  Analog input
// pins are a separate set.

// ATMEL ATMEGA162
//
//                    +-\/-+
//      (D0)    PB0  1|    |40  VCC
//      (D1)    PB1  2|    |39  PA0
//      (RX1)   PB2  3|    |38  PA1
//      (TX1)   PB3  4|    |37  PA2
//      (D2)    PB4  5|    |36  PA3
//      (D3)    PB5  6|    |35  PA4
//      (D4)    PB6  7|    |34  PA5
//      (D5)    PB7  8|    |33  PA6
//            RESET  9|    |32  PA7
//      (RX0)   PD0 10|    |31  PE0
//      (TX0)   PD1 11|    |30  PE1
//      (INT0)  PD2 12|    |29  PE2
//      (INT1)  PD3 13|    |28  PC7    (D13)
//              PD4 14|    |27  PC6    (D12)
//              PD5 15|    |26  PC5    (D11)
//              PD6 16|    |25  PC4    (D10)
//              PD7 17|    |24  PC3    (D9)
//            XTAL2 18|    |23  PC2    (D8)
//            XTAL1 19|    |22  PC1    (D7)
//              GND 20|    |21  PC0    (D6)
//                    +----+
// NOTE: Port A is used as DIGITAL only for TESTING!!!
// (PWM+ indicates the additional PWM pins on the ATmega168.)


#define PA 1
#define PB 2
#define PC 3
#define PD 4
#define PE 5

// these arrays map port names (e.g. port B) to the
// appropriate addresses for various functions (e.g. reading
// and writing)
const uint8_t PROGMEM port_to_mode_PGM[] = {
	NOT_A_PORT,
	&DDRA,
	&DDRB,
	&DDRC,
	&DDRD,
	&DDRE,
};

const uint8_t PROGMEM port_to_output_PGM[] = {
	NOT_A_PORT,
	&PORTA,
	&PORTB,
	&PORTC,
	&PORTD,
	&PORTE,
};

const uint8_t PROGMEM port_to_input_PGM[] = {
	NOT_A_PORT,
	&PINA,
	&PINB,
	&PINC,
	&PIND,
	&PINE,
};

const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
	PB, /* 0 */
	PB,
	PB,
	PB,
	PB,
	PB,
	PC, /* 6 */
	PC,
	PC,
	PC,
	PC,
	PC,
	PC,
	PC,
	PA, /* 14 */
	PA,
	PA,
	PA,
	PA,
	PA,
	PA,
	PA,
};

const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
	_BV(0), /* 0, port B */
	_BV(1),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	_BV(0), /* 6, port C */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	_BV(0), /* 14, port A */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
};
#endif
