/*
  derkgps.c - Enanched GPS on TTY interface

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

#include "derkgps.h"
// #include "gps.h"
// #include "atinterface.h"

// Registri:
// - odo_speedAlarm, RW il valore di velocit\u00e0 oltre il quale generare un allarme [in mssu]
// - odo_breakAlarm, RW il valore di decellerazione che triggera un allarme [in mmsu]
// - reg_event, RW eventi generati dall'ultima lettura, un INTERRUPT viene generato solo quando viene
//			generato un evento non ancora notificato
//		Praticamente ogni bit indica un evento gi\u00e0 notificato ma non ancora
//		confermato dall'host, un evento notificato non viene pi\u00f9 notificato fino
//		a che il primo non viene confermato.
//
// - EVENTI = { MOVING, STOP, EVENT_OVER_SPEED, EVENT_EMERGENCY_BREAK }

// This relations holds:
//	m/s * p/m = p/s     =>      d_speed * d_ppm = d_freq


#define DERKGPS_INTR_LEN        200

//----- DIGITAL PINS
// #define USE_DIP_PACKAGE
#ifdef USE_DIP_PACKAGE
# warning Using DIP package pinout
uint8_t led1		=   15;	// Running DERKGPS_DEBUG led	(using PA1)
uint8_t led2		=   16;	// Running DERKGPS_DEBUG led	(using PA2)
uint8_t led3		=   17;	// Running DERKGPS_DEBUG led	(using PA3)
uint8_t gpsPowerPin 	=   14;	// HIGH=>GPS_ON, LOW=>GPS_OFF	(using PA0)
uint8_t intReq      	=   18;	// Interrupt pin		(using PA4)
#else
uint8_t led1		=   6;	// Running DERKGPS_DEBUG led	(using PC0)
uint8_t led2		=   7;	// Running DERKGPS_DEBUG led	(using PC1)
uint8_t led3		=   8;	// Running DERKGPS_DEBUG led	(using PC2)
uint8_t gpsPowerPin 	=   9;	// HIGH=>GPS_ON, LOW=>GPS_OFF	(using PC3)
uint8_t intReq      	=   10;	// Interrupt pin		(using PC4)
#endif

//----- ODOMETER
/// Odometer pulses count
volatile unsigned long d_pcount = 0;
/// Last computed odometer pulses frequency
volatile unsigned long d_freq = 0;
/// The old freq value used for Alarms Checking
unsigned d_oldFreq = 0;
/// Current max [ppm/s] EVENT_OVER_SPEED alarm
unsigned long d_minOverSpeed = 0;
/// Current max [ppm/s] decelleration EVENT_EMERGENCY_BREAK alarm
unsigned long d_minEmergencyBreak = 0;
/// Frequency variation since last check [Hz]
long d_df = 0;
/// Time interval between last two update [s]
unsigned long d_dt = 0;
/// Last update time [ms]
unsigned long d_odoLastUpdate = 0;

//----- AT Interface
/// Delay ~[s] between dispaly monitor sentences, if 0 DISABLED (default 0);
unsigned d_displayTime = 5;
/// Buffer for sentence display formatting
char d_displayBuff[OUTPUT_BUFFER_SIZE];
/// ATinterface top-halves Interrupt scheduling flags
short d_thIntrCMD = 0;

//----- EVENT GENERATION
/// Last update time [ms]
unsigned long d_eventsLastUpdate = 0;
/// Events enabled to generate signals
//derkgps_event_t d_activeEvents[EVENT_CLASS_TOT] = {EVENT_NONE, EVENT_NONE}; // Disabling all interrupts by default
derkgps_event_t d_activeEvents[EVENT_CLASS_TOT] = { 0x1F, 0x03 };
/// Events suspended by this code to avoid interrupt storms
derkgps_event_t d_suspendedEvents[EVENT_CLASS_TOT] = {EVENT_NONE, EVENT_NONE};
/// Events pending to be ACKed
derkgps_event_t d_pendingEvents[EVENT_CLASS_TOT] = {EVENT_NONE, EVENT_NONE};

//----- GPS DATA
/// The GPS power state: 1=ON, 0=OFF
unsigned d_gpsPowerState = 0;
/// The old fix value used for Alarms Checking
unsigned d_oldFix = FIX_INVALID;
/// GPS top-halves Interrupt scheduling flags
short d_thIntrGPS = 0;

//----- DISPLAY
/// Last update time [ms]
unsigned long d_displayLastUpdate = 0;

//----- Loop function static variables
unsigned long t0;
unsigned long c0;
unsigned long f0;

//----- ODO Interrupt handler
/// Odometer interrupt handler.
/// This method is called by the ISR and simply take track of pulses
/// coming in from the odometer input line.
void countPulse(void) {
	d_pcount++;
}

//----- Utility functions
void formatDouble(double val, char *buf, int len) {
	long integer;
	long fractal;
	
	integer = (long)val;
	fractal = (long)(((double)val-(double)integer)*(double)10000);
	fractal = (fractal<0) ? -fractal : fractal;
	
	snprintf(buf, len, "%+ld.%04ld", integer, fractal);
}

//----- Display monitor
void display(void) {
	unsigned long cc = d_pcount;
	unsigned long cf = d_freq;
	unsigned long time = millis();
	uint8_t ge = d_pendingEvents[EVENT_CLASS_GPS];
	uint8_t oe = d_pendingEvents[EVENT_CLASS_ODO];
	unsigned siv = gpsSatInView();
	unsigned fix = gpsFix();
	
	time -= d_displayLastUpdate;
	if ( time < (d_displayTime*1000) ) {
		return;
	}
	
	if (gpsIsPosValid()) {
		// 26 Bytes for this first part
		snprintf(d_displayBuff, OUTPUT_BUFFER_SIZE,
			"0x%02X%02X %8lu %4lu %2u %1u ",
			ge, oe, cc, cf, siv, fix);
		// This is what we have to append: "+99.9999 +999.9999"
		formatDouble(gpsLat(), d_displayBuff+26, 9);
		formatDouble(gpsLon(), d_displayBuff+26+9, 10);
		d_displayBuff[26+8]=' ';
		
		// This call require a total of:
		// 26+9+10=45 Bytes
;
	} else {
		snprintf(d_displayBuff, OUTPUT_BUFFER_SIZE,
			"0x%02X%02X %8lu %4lu %2u %1u NA NA",
			ge, oe, cc, cf, siv, fix);
	}
	
	Serial_printLine(d_displayBuff);
	
	d_displayLastUpdate = millis();
}

//----- Alarms control
void notifyEvent(derkgps_event_class_t event_class, uint8_t event) {
	derkgps_event_t alreadyNotified;
	derkgps_event_t intrEnabled;
	
// 	Already done in checkAlarms!
// 	event = 0x01<<event;
	
	// Not-null iff this event is enabled to generate interrupt
	intrEnabled = d_activeEvents[event_class] & event;
	
	// Not-null iff this event is already pending to be ACKed
	alreadyNotified = d_pendingEvents[event_class] & event;
	
	// saving event
	d_pendingEvents[event_class] |= event;
	
	// looking if it has to be notified
	if (intrEnabled && !alreadyNotified) {
		pinMode(intReq, OUTPUT);
/* Wait until the interrupt has been read
		delay(DERKGPS_INTR_LEN);
		pinMode(intReq, INPUT);
*/
	}
	
}

// NOTE this method should avoid notification storms.
// Once an event has been notified it should not be notified anymore while
// is still old... only at the time of reverse event the notifier should be
// re-enabled.
#define SET_EVENT(_evt_)			\
	event = (0x1 << _evt_)
void checkAlarms(void) {
	unsigned long newFreq;
	unsigned newFix;
	uint8_t event = 0;
// 	unsigned long time;
	
// 	time = millis()-d_eventsLastUpdate;
// 	if ( time < 500 )
// 		return;
	
	// Checking Moving Events...
	newFreq = d_freq;
	if ( d_oldFreq==0 ) {
		if ( newFreq>0 ) {
			// START Moving Event
			SET_EVENT(ODO_EVENT_MOVE);
			notifyEvent(EVENT_CLASS_ODO, event);
			digitalWrite(led3, HIGH);
		}
	} else if ( newFreq==0 ) {
		if ( d_oldFreq>0 ) {
			// STOP Moving Event
			SET_EVENT(ODO_EVENT_STOP);
			notifyEvent(EVENT_CLASS_ODO, event);
			digitalWrite(led3, LOW);
		}
	}
	d_oldFreq = newFreq;
	
	// Checking for EVENT_EMERGENCY_BREAK event
	// NOTE Acceleration is always OK
	if ( d_minEmergencyBreak ) {
		
		SET_EVENT(ODO_EVENT_EMERGENCY_BREAK);
		
		if ( d_df < 0 ) { // Decelleration: monitoring rate
			d_df = -d_df*1000;
			if ( (d_df/d_dt) > d_minEmergencyBreak &&
				!( d_suspendedEvents[EVENT_CLASS_ODO] & event ) ) {
				
				// Trigger an interrupt
				notifyEvent(EVENT_CLASS_ODO, event);
				
				// NOTE this code SUPPOSE that we can't modify d_activeEvents
				//	using AT Commands... otherwise we must pay attention
				//	when disabling an event because we don't know anymore if
				//	the user has required it to be disabled or not.
				// Now that the event has been notified we suspend it until
				// we return within the speed limit
				d_suspendedEvents[EVENT_CLASS_ODO] |= event;
			}
			
		} else { // Acceleration reenable the event notify
			d_suspendedEvents[EVENT_CLASS_ODO] &= ~event;
		}
	}

	// Checking for EVENT_OVER_SPEED event
	if ( d_minOverSpeed ) {
		
		if ( d_freq > d_minOverSpeed) { // Overspeed: event notify
			
			// Over-speed reenable the SAFE_SPEED event notify
			SET_EVENT(ODO_EVENT_SAFE_SPEED);
			d_suspendedEvents[EVENT_CLASS_ODO] &= ~event;
			
			SET_EVENT(ODO_EVENT_OVER_SPEED);
			if ( !(d_suspendedEvents[EVENT_CLASS_ODO] & event) ) {
				// Trigger an interrupt
				notifyEvent(EVENT_CLASS_ODO, event);
				
				// NOTE this code SUPPOSE that we can't modify d_activeEvents
				//	using AT Commands... otherwise we must pay attention
				//	when disabling an event because we don't know anymore if
				//	the user has required it to be disabled or not.
				// Now that the event has been notified we suspend it until
				// we return within the speed limit
				d_suspendedEvents[EVENT_CLASS_ODO] |= event;
			}
			
		} else {
			
			// Safe speed reenable the OVER_SPEED event notify
			SET_EVENT(ODO_EVENT_OVER_SPEED);
			d_suspendedEvents[EVENT_CLASS_ODO] &= ~event;
			
			SET_EVENT(ODO_EVENT_SAFE_SPEED);
			if ( !(d_suspendedEvents[EVENT_CLASS_ODO] & event) ) {
				// Trigger an interrupt
				notifyEvent(EVENT_CLASS_ODO, event);
				
				// NOTE this code SUPPOSE that we can't modify d_activeEvents
				//	using AT Commands... otherwise we must pay attention
				//	when disabling an event because we don't know anymore if
				//	the user has required it to be disabled or not.
				// Now that the event has been notified we suspend it until
				// we return within the speed limit
				d_suspendedEvents[EVENT_CLASS_ODO] |= event;
			}
		}
	}
	
	// Checking GPS fix
	newFix = gpsFix();
	if (newFix != d_oldFix) {
		if (newFix>0) {
			SET_EVENT(GPS_EVENT_FIX_GET);
		} else {
			SET_EVENT(GPS_EVENT_FIX_LOSE);
		}
		notifyEvent(EVENT_CLASS_GPS, event);
		d_oldFix = newFix;
	}

	// Event led control
	if (d_pendingEvents[EVENT_CLASS_ODO] ||
		d_pendingEvents[EVENT_CLASS_GPS]) {
		digitalWrite(led2, HIGH);
	} else {
		digitalWrite(led2, LOW);
	}
	
// 	d_eventsLastUpdate = millis();
}

/// @return 0 on successfull data update
int odoUpdate(void) {
	unsigned long t1 = 0;
	unsigned long c1 = 0;
	unsigned long dc = 0;
	
	t1 = millis();
	c1 = d_pcount;
	
	// 511[ms] = 0x1FF
	if ( (t1-d_odoLastUpdate)>>9 ) {
		d_odoLastUpdate = t1;
	} else {
		// Less the 511 ms since last update
		return -1;
	}
	
	// NOTE millis() return the number of milliseconds since the current
	// program started running, as an unsigned long.
	// This number will overflow (go back to zero), after approximately 9 hours.
	// If the counter has overflowed => simply we avoid the update
	// next call will success
	if (t1 < t0) {
		t0 = t1;
		c0 = c1;
		return -2;
	}

	// NOTE d_freq = dc/d_dt=dc/((t1-t0)/1000) => dc*1000/(t1-t0)
	// this last formula is safer using unsigned long values due to truncation
	//	of decimal digits ;-)
	if ( c1 == c0 ) {
		d_freq = 0;
	} else {
		dc = (c1 - c0) * 1000;		// Pulses count variation
		d_dt = (t1 - t0);		// Elapsed time in [s]
		d_freq = (dc/d_dt);		// New frequency
	}
	d_df = d_freq - f0;			// Frequency variation

	// Saving values for next cycle
	f0 = d_freq;
	t0 = t1;
	c0 = c1;
	
	return 0;
}

void gpsUpdate(void) {

	if (d_gpsPowerState == 0) {
		// Powering off GPS
		digitalWrite(gpsPowerPin, LOW);
		digitalWrite(led1, LOW);
		gpsReset();
		// Return with no other parsing
		return;
	}
		
	// Powering on GPS
	digitalWrite(gpsPowerPin, HIGH);
	
	// Update GPS info; this call takes about 1s, this is the minimum
	// 	delay for a speed update...
	if ( checkInterrupt(INTR_GPS) ) {
		digitalSwitch(led1);
		gpsParse();
		ackInterrupt(INTR_GPS);
	}

}

//----- System Initialization
void setup(void) {

	// Disable interrupts (just in case)
	cli();
	
	// Disable JTAG interface
	// This require TWICE writes in 4 CYCLES
	MCUCSR |= 0x80;
	MCUCSR |= 0x80;

	// Inputs: MEMS (PA0, PA1 and PA2)
	DDRA    = 0xF8;
	// Disabling Pull-Ups
	PORTA   = 0x00;
	
	// Outpust: LEDS(PC0-2), GPG PowerUp(PC3), Inputs(HiZ) INT_REQ(PC4)
	DDRC	= 0x8F;
	// Disabling Pull-Ups
	PORTC	= 0x00;
	
	// Configure timers
	initTime();
	
	// Configure UART ports
	initSerials();
	
	// Configure GPS
	initGps((unsigned long)GPS_VTG|GPS_GSV|GPS_GLL|GPS_GSA);
	
	// Enable interrupts
	sei();

	// GPS module power control
	pinMode(gpsPowerPin, OUTPUT);
	digitalWrite(gpsPowerPin, LOW);
	
	//Note: to assert an interrupt the INTR line should be asserted LOW for a while
	pinMode(intReq, INPUT);
	digitalWrite(intReq, LOW);
	
	// NMEA Parsing Activity LED
	pinMode(led1, OUTPUT);
	digitalWrite(led1, LOW);
	
	// Events Pending Monitor LED
	pinMode(led2, OUTPUT);
	digitalWrite(led2, LOW);
	
	// TO BE DEFINED
	pinMode(led3, OUTPUT);
	digitalWrite(led3, LOW);
	
	// Odometer INTR routine initialization
	attachInterrupt(INTR0, countPulse, RISING);
	
	// Loop function static variables INITIALIZATION
	t0 = millis();
	c0 = d_pcount;
	f0 = 0;
	
	// Powering on GPS and Optical Interrupt line
	digitalWrite(gpsPowerPin, HIGH);
	d_gpsPowerState = 1;
	
	// Initial data update
	odoUpdate();
	
}

inline void loop(void) {
	
	// Updating GPS data
	gpsUpdate();
	
	// Updating ODO data
	if (odoUpdate()!=0)
		return;
	
	// Check constraints and issue alarms
	checkAlarms();
	
	// Display summary sentence
	if (d_displayTime) {
		display();
	}
	
	// Execute user commands
	if ( checkInterrupt(INTR_CMD) ) {
		parseCommand();
		ackInterrupt(INTR_CMD);
	}

}

int main(void) {

	setup();
	
// 	Serial_printLine("             DerkGPS v1.0 (30-07-2008)                ");
// 	Serial_printLine("Copyright 2008 by Patrick Bellasi <derkling@gmail.com>");
	Serial_printLine("DerkGPS v1.0.0 by Patrick Bellasi");
// 	Serial_printLine("AT Command ready");
	
	while(1) {
		loop();
	}
	
}


