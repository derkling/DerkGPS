/*
  HardwareSerial.h - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef HardwareSerial_h
#define HardwareSerial_h

#include "atmega162.h"

#define UART0_BAUD_RATE 9600
#define UART1_BAUD_RATE 9600

// Compute the UART baud rate
#define UART_BAUD_CALC(UART_BAUD_RATE,F_CPU) ((F_CPU)/((UART_BAUD_RATE)*16l)-1)

// Used to define variables for buffering incoming serial data.  We're
// using a ring buffer (I think), in which rx_buffer_head is the index of the
// location to which to write the next incoming character and rx_buffer_tail
// is the index of the location from which to read.
// NOTE max=256
#define UART0_BUFFER_SIZE 76
// Schedule top-halves interrupt when >=64 bytes
#define UART0_BUFFER_THLIMIT	72
#define UART1_BUFFER_SIZE 48
// Schedule top-halves interrupt when >=40 bytes
#define UART1_BUFFER_THLIMIT	44

// Define the line terminator (CR(\r) = 13 = 0x0D)
#define LINE_TERMINATOR 0x0D

typedef enum {
	UART0 = 0,
	UART1,
 	UART_NUM	// This must be the last entry
} uart_port_t;


// Top-Halves serials interrupt scheduling flags
extern uint8_t intr[UART_NUM];
#define scheduleTopHalve(PORT)	intr[PORT]=1
#define ackInterrupt(PORT)	intr[PORT]=0
#define checkInterrupt(PORT) 	intr[PORT]==1

void	initSerials(void);

uint8_t	available(uart_port_t port);

char	look(uart_port_t port);
char	read(uart_port_t port);
int	readLine(uart_port_t port, char *buff, unsigned short len);

void	flush(uart_port_t port);

void	print(uart_port_t port, char c);
void	printStr(uart_port_t port, const char *str);
void	printLine(uart_port_t port, const char *str);

#endif

