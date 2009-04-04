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

#ifdef __AVR_AT90CAN128__
// ATMEL AT90CAN128 (TQFP)
//
//                     +---------+
//                NC   1|         | 64  AVCC
//    (RXD0,PDI) PE0   2|         | 63  GND
//    (TXD0,PDO) PE1   3|         | 62  AREF
//   (XCK0,AIN0) PE2   4|         | 61  PF0 (ADC0)
//   (OC3A,AIN1) PE3   5|         | 60  PF1 (ADC1)
//   (OC3B,INT4) PE4   6|         | 59  PF2 (ADC2)
//   (OC3C,INT5) PE5   7|         | 58  PF3 (ADC3)
//     (T3,INT6) PE6   8|         | 57  PF4 (ADC4,TCK)
//   (ICP3,INT7) PE7   9|         | 56  PF5 (ADC5,TMS)
//         (/SS) PB0  10|         | 55  PF6 (ADC6,TDO)
//         (SCK) PB1  11|         | 54  PF7 (ADC7,TDI)
//        (MOSI) PB2  12|         | 53  GND
//        (MISO) PB3  13|         | 52  VCC
//        (OC2A) PB4  14|         | 51  PA0 (AD0)
//        (OC1A) PB5  15|         | 50  PA1 (AD1)
//        (OC1B) PB6  16|         | 49  PA2 (AD2)
//                   ----       -----
//   (OC0A,OC1C) PB7  17|         | 48  PA3 (AD3)
//       (TOSC2) PG3  18|         | 47  PA4 (AD4)
//       (TOSC1) PG4  19|         | 46  PA5 (AD5)
//             RESET  20|         | 45  PA6 (AD6)
//               VCC  21|         | 44  PA7 (AD7)
//               GND  22|         | 43  PG2 (ALE)
//             XTAL2  23|         | 42  PC7 (A15,CLKO)
//             XTAL1  24|         | 41  PC6 (A14)
//    (SCL,INT0) PD0  25|         | 40  PC5 (A13)
//    (SDA,INT1) PD1  26|         | 39  PC4 (A12)
//   (RXD1,INT2) PD2  27|         | 38  PC3 (A11)
//   (TXD1,INT3) PD3  28|         | 37  PC2 (A10)
//        (ICP1) PD4  29|         | 36  PC1 (A9)
//  (TXCAN,XCK1) PD5  30|         | 35  PC0 (A8)
//    (RXCAN,T1) PD6  31|         | 34  PG1 (RD)
//          (T0) PD7  32|         | 33  PG0 (/WR)
//                      +---------+

#define PA 1
#define PB 2
#define PC 3
#define PD 4
#define PE 5
#define PF 6
#define PG 7

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
	&DDRF,
	&DDRG,
};

const uint8_t PROGMEM port_to_output_PGM[] = {
	NOT_A_PORT,
	&PORTA,
	&PORTB,
	&PORTC,
	&PORTD,
	&PORTE,
	&PORTF,
	&PORTG,
};

const uint8_t PROGMEM port_to_input_PGM[] = {
	NOT_A_PORT,
	&PINA,
	&PINB,
	&PINC,
	&PIND,
	&PINE,
	&PINF,
	&PING,
};

const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
	PA, /* 0 */
	PA,
	PA,
	PA,
	PA, /* 4 */
	PA,
	PA,
	PA,
	PB, /* 8 */
	PB,
	PB,
	PB,
	PB, /* 12 */
	PB,
	PB,
	PB,
	PC, /* 16 */
	PC,
	PC,
	PC,
	PC, /* 20 */
	PC,
	PC,
	PC,
	PE, /* 24 */
	PE,
	PE,
	PE,
	PE, /* 28 */
	PE,
	PE,
	PE,
	PD, /* 32 */

};

const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
	_BV(0), /* 0, port A */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	_BV(0), /* 8, port B */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	_BV(0), /* 16, port C */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	_BV(0), /* 24, port E */
	_BV(1),
	_BV(2),
	_BV(3),
	_BV(4),
	_BV(5),
	_BV(6),
	_BV(7),
	_BV(7), /* 32, port D */

};
#endif
