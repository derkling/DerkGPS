/*
  derkgps.h - Enanched GPS on TTY interface

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

#ifndef derkgps_h
#define derkgps_h

#include <string.h>

#include "at90can.h"
#include "serials.h"
#include "atinterface.h"
#include "gps.h"
#include "odo.h"
#include "can.h"

// Uncomment to enable GPS sentence testing...
// #define TEST_GPS
#define UART_AT		UART0
#define UART_GPS	UART1

#define OUTPUT_BUFFER_SIZE	47

//----- EVENT GENERATION
typedef enum {
	ODO_EVENT_MOVE		= 0,
	ODO_EVENT_STOP,
	ODO_EVENT_OVER_SPEED,
	ODO_EVENT_EMERGENCY_BREAK,
	ODO_EVENT_SAFE_SPEED,
	ODO_EVENT_DISTANCE,
} derkgps_event_odo_t;

typedef enum {
	GPS_EVENT_FIX_GET	= 0,
	GPS_EVENT_FIX_LOSE,
	GPS_EVENT_MOVE,
	GPS_EVENT_STOP,
} derkgps_event_gps_t;

/// Event mask defining the events enabled to generate signals
typedef uint8_t derkgps_event_t;

#define EVENT_NONE (derkgps_event_t)0x00

typedef enum {
	EVENT_CLASS_ODO = 0,
	EVENT_CLASS_GPS,
	EVENT_CLASS_TOT	// This must be the last entry
} derkgps_event_class_t;


// AT Control
#define Serial_print(C)			print(UART_AT, C)
#define Serial_printStr(STR)		printStr(UART_AT, STR)
#define Serial_printLine(STR)		printLine(UART_AT, STR)
#define Serial_printFlush()		flush(UART_AT)
#define Serial_readLine(BUFF, LEN)	readLine(UART_AT, BUFF, LEN)
#define SERIAL_NEWLINE	"\r\n"

// Utility functions
void formatDouble(double val, char *buf, int len);

//----- DIGITAL PINS
// APE Interrupt pin (Active LOW) (PA0)
#define intReq      	0

// HIGH=>GPS_ON, LOW=>GPS_OFF	  (PA1)
#define gpsPowerPin 	1
// HIGH=>ANT_ON, LOW=>GPS_OFF	  (PC7)
#define gpsAntPowerPin 	23

// CAN1 Power Enable HIGH=>Active (PA2)
#define can1Power	2
// CAN2 Power Enable HIGH=>Active (PA3)
#define can2Power	3
// CAN Switch Select (LOW=>CAN1)  (PD7)
#define canSwitchSelect	32
// CAN Switch Enabler LOW=>Active (PE3)
#define canSwitchEnable	27

// MEMS Test mode (HIGH Active)   (PA4)
#define memsTestPin	4
// Move sensor interrupt INPUT    (PE7)
#define moveSensorIntr  31

// ODO Pulses Count               (PE6/T3)
#define odoPulsePin	30

// Running DERKGPS_DEBUG led	  (PA5)
#define led1		5
// Running DERKGPS_DEBUG led	  (PA6)
#define led2		6
// Running DERKGPS_DEBUG led	  (PA7)
#define led3		7


#endif
