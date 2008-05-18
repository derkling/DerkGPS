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

#include "atmega162.h"
#include "serials.h"
#include "atinterface.h"
#include "gps.h"

#define OUTPUT_BUFFER_SIZE	64

//----- EVENT GENERATION
typedef enum {
	ODO_EVENT_MOVE		= 0,
	ODO_EVENT_STOP,
	ODO_EVENT_OVER_SPEED,
	ODO_EVENT_EMERGENCY_BREAK,
} derkgps_event_odo_t;

typedef enum {
	GPS_EVENT_FIX_GET	= 0,
	GPS_EVENT_FIX_LOSE,
} derkgps_event_gps_t;

/// Event mask defining the events enabled to generate signals
typedef uint8_t derkgps_event_t;

#define EVENT_NONE (derkgps_event_t)0x00

typedef enum {
	EVENT_CLASS_ODO = 0,
	EVENT_CLASS_GPS,
	EVENT_CLASS_TOT	// This must be the last entry
} derkgps_event_class_t;


// ATControl (UART1)
#define Serial_print(C)			print(UART1, C)
#define Serial_printStr(STR)		printStr(UART1, STR)
#define Serial_printLine(STR)		printLine(UART1, STR)
#define Serial_printFlush()		flush(UART1)
#define SERIAL_NEWLINE	"\r\n"
#define Serial_readLine(BUFF, LEN)	readLine(UART1, BUFF, LEN)

// Utility functions
void formatDouble(double val, char *buf, int len);

// Top-Halves serials interrupt names
#define INTR_GPS	UART0
#define INTR_CMD	UART1


#endif
