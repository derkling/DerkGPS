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
uint8_t ledPin2		=   16;	// Running DERKGPS_DEBUG led	(using PA2)
uint8_t ledPin3		=   17;	// Running DERKGPS_DEBUG led	(using PA3)
uint8_t gpsPowerPin 	=   14;	// HIGH=>GPS_ON, LOW=>GPS_OFF	(using PA0)
uint8_t intPin      	=   18;	// Interrupt pin		(using PA4)
#else
uint8_t led1		=   6;	// Running DERKGPS_DEBUG led	(using PC0)
uint8_t ledPin2		=   7;	// Running DERKGPS_DEBUG led	(using PC1)
uint8_t ledPin3		=   8;	// Running DERKGPS_DEBUG led	(using PC2)
uint8_t gpsPowerPin 	=   9;	// HIGH=>GPS_ON, LOW=>GPS_OFF	(using PC3)
uint8_t intPin      	=   10;	// Interrupt pin		(using PC4)
#endif

//----- ODOMETER
/// Odometer pulses count
volatile unsigned long d_pcount = 0;
/// Last computed odometer pulses frequency
unsigned long d_freq = 0;
/// Current max [ppm/s] EVENT_OVER_SPEED alarm
unsigned long d_minOverSpeed = 0;
/// Current max [ppm/s] decelleration EVENT_EMERGENCY_BREAK alarm
unsigned long d_minEmergencyBreak = 0;
/// Frequency variation since last check [Hz]
long d_df = 0;
/// Time interval between last two update [s]
unsigned long d_dt = 0;

//----- AT Interface
/// Delay ~[s] between dispaly monitor sentences, if 0 DISABLED (default 0);
unsigned d_displayTime = 5;
/// Loop count for display hiding (Init=1 to have a display at first call)
unsigned d_displayCount = 1;
/// Buffer for sentence display formatting
char d_displayBuff[OUTPUT_BUFFER_SIZE];
/// ATinterface top-halves Interrupt scheduling flags
short d_thIntrCMD = 0;

//----- EVENT GENERATION
/// Events enabled to generate signals
derkgps_event_t d_activeEvents[EVENT_CLASS_TOT] = {EVENT_NONE, EVENT_NONE};
/// Events suspended by this code to avoid interrupt storms
derkgps_event_t d_suspendedEvents[EVENT_CLASS_TOT] = {EVENT_NONE, EVENT_NONE};
/// Events pending to be ACKed
derkgps_event_t d_pendingEvents[EVENT_CLASS_TOT] = {EVENT_NONE, EVENT_NONE};

//----- GPS DATA
/// The GPS power state: 1=ON, 0=OFF
unsigned d_gpsPowerState = 0;
/// The old fix value
unsigned oldFix = FIX_INVALID;
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
	
	snprintf(buf, len, "%+ld.%ld", integer, fractal);
}

//----- Display monitor
inline void display(void) {
	unsigned long cur_count;
	unsigned long cur_freq;
	char cur_lat[12];
	char cur_lon[13];
	unsigned long time;
	
	time = millis()-d_displayLastUpdate;
	if ( time < (d_displayCount*1000) )
		return;
	
// 	if (!--d_displayCount) {
	cur_count = d_pcount;
	cur_freq = d_freq;
	formatDouble(gpsLat(), cur_lat, 12);
	formatDouble(gpsLon(), cur_lon, 13);
	
	if (gpsIsPosValid()) {
		snprintf(d_displayBuff, OUTPUT_BUFFER_SIZE,
			"0x%02X%02X %7lu %7lu %u %u %s %s",
			d_pendingEvents[EVENT_CLASS_GPS],
			d_pendingEvents[EVENT_CLASS_ODO],
			cur_count, cur_freq,
			gpsSatInView(), gpsFix(),
			cur_lat, cur_lon);
	} else {
		snprintf(d_displayBuff, OUTPUT_BUFFER_SIZE,
			"0x%02X%02X %7lu %7lu %u %u NA NA",
			d_pendingEvents[EVENT_CLASS_GPS],
			d_pendingEvents[EVENT_CLASS_ODO],
			cur_count, cur_freq,
			gpsSatInView(), gpsFix());
	}
	
	Serial_printLine(d_displayBuff);
	d_displayCount = d_displayTime;
// 	}
	
	d_displayLastUpdate = millis();
	
}

//----- AT Control Interface
inline void doUserCmd(void) {

// 	if ( Serial_readLine(userCmd, DERKGPS_CMD_MAXLEN) == -1 ) {
// 		return;
// 	}
// 	// Debugging: print received command
// 	snprintf(d_displayBuff, OUTPUT_BUFFER_SIZE,
// 			"CMD: %s", userCmd);
// 	Serial_printLine(d_displayBuff);
	
	parseCommand();
	
}

//----- Alarms control
inline void notifyEvent(derkgps_event_class_t event_class, uint8_t event) {
	derkgps_event_t alreadyNotified;
	derkgps_event_t intrEnabled;
	
	event = 0x01<<event;
	
	// Not-null iff this event is enabled to generate interrupt
	intrEnabled = d_activeEvents[event_class] & event;
	
	// Not-null iff this event is already pending to be ACKed
	alreadyNotified = d_pendingEvents[event_class] & event;
	
	// saving event
	d_pendingEvents[event_class] |= event;
	
	// looking if it has to be notified
	if (intrEnabled && !alreadyNotified) {
		pinMode(intPin, OUTPUT);
		digitalWrite(intPin, LOW);
// delay(DERKGPS_INTR_LEN);
// pinMode(intPin, INPUT);
	}
	
}

// NOTE this method should avoid notification storms.
// Once an event has been notified it should not be notified anymore while
// is still old... only at the time of reverse event the notifier should be
// re-enabled.
inline void checkAlarms(void) {
	unsigned newFix;
	
	// Checking for EVENT_EMERGENCY_BREAK event
	// NOTE Acceleration is always OK
	if ( d_minEmergencyBreak ) {
		
		if ( d_df < 0 ) { // Decelleration: monitoring rate
			d_df = -d_df*1000;
			if ( (d_df/d_dt) > d_minEmergencyBreak &&
				!(d_suspendedEvents[EVENT_CLASS_ODO] & ODO_EVENT_EMERGENCY_BREAK) ) {
				
				// Trigger an interrupt
				notifyEvent(EVENT_CLASS_ODO, ODO_EVENT_EMERGENCY_BREAK);
				
				// NOTE this code SUPPOSE that we can't modify d_activeEvents
				//	using AT Commands... otherwise we must pay attention
				//	when disabling an event because we don't know anymore if
				//	the user has required it to be disabled or not.
				// Now that the event has been notified we suspend it until
				// we return within the speed limit
				d_suspendedEvents[EVENT_CLASS_ODO] |= ODO_EVENT_EMERGENCY_BREAK;
			}
			
		} else { // Acceleration reenable the event notify
			d_suspendedEvents[EVENT_CLASS_ODO] &= ~ODO_EVENT_EMERGENCY_BREAK;
		}
	}

	// Checking for EVENT_OVER_SPEED event
	if ( d_minOverSpeed ) {
		
		if ( d_freq > d_minOverSpeed && // Overspeed: event notify
			!(d_suspendedEvents[EVENT_CLASS_ODO] & ODO_EVENT_OVER_SPEED)) {
			
			// Trigger an interrupt
			notifyEvent(EVENT_CLASS_ODO, ODO_EVENT_OVER_SPEED);
			
			// NOTE this code SUPPOSE that we can't modify d_activeEvents
			//	using AT Commands... otherwise we must pay attention
			//	when disabling an event because we don't know anymore if
			//	the user has required it to be disabled or not.
			// Now that the event has been notified we suspend it until
			// we return within the speed limit
			d_suspendedEvents[EVENT_CLASS_ODO] |= ODO_EVENT_OVER_SPEED;
			
		} else { // Safe speed reenable the event notify
			d_suspendedEvents[EVENT_CLASS_ODO] &= ~ODO_EVENT_OVER_SPEED;
		}
	}
	
	// Checking GPS fix
	newFix = gpsFix();
	if (newFix != oldFix) {
		if (newFix>0)
			notifyEvent(EVENT_CLASS_GPS, GPS_EVENT_FIX_GET);
		else
			notifyEvent(EVENT_CLASS_GPS, GPS_EVENT_FIX_LOSE);
		oldFix = newFix;
	}

	// Event led control
	if (d_pendingEvents[EVENT_CLASS_ODO] ||
		d_pendingEvents[EVENT_CLASS_GPS]) {
		digitalWrite(ledPin2, HIGH);
	} else {
		digitalWrite(ledPin2, LOW);
	}
	
}

//----- System Initialization
inline void setup(void) {

	// GPS module power control
	digitalWrite(gpsPowerPin, LOW);
	pinMode(gpsPowerPin, OUTPUT);
	
	//Note: to assert an interrupt the INTR line should be asserted LOW for a while
	pinMode(intPin, INPUT);
	digitalWrite(intPin, LOW);
	
	// NMEA Parsing Activity LED
	digitalWrite(led1, LOW);
	pinMode(led1, OUTPUT);
	// Events Pending Monitor LED
	digitalWrite(ledPin2, LOW);
	pinMode(ledPin2, OUTPUT);
	// TO BE DEFINED
	digitalWrite(ledPin3, LOW);
	pinMode(ledPin3, OUTPUT);
	
	// Odometer INTR routine initialization
	attachInterrupt(INTR0, countPulse, RISING);
	
	// Loop function static variables INITIALIZATION
	t0 = millis();
	c0 = d_pcount;
	f0 = 0;
	
	// Powering on GPS and Optical Interrupt line
	digitalWrite(gpsPowerPin, HIGH);
	d_gpsPowerState = 1;
	
}

/// @return 0 on successfull data update
int odoUpdate(void) {
	unsigned long t1 = 0;
	unsigned long c1 = 0;
	unsigned long dc = 0;
	
	t1 = millis();
	c1 = d_pcount;
	
	// NOTE millis() return the number of milliseconds since the current
	// program started running, as an unsigned long.
	// This number will overflow (go back to zero), after approximately 9 hours.
	// If the counter has overflowed => simply we avoid the update
	// next call will success
	if (t1 < t0) {
		t0 = t1;
		c0 = c1;
		return -1;
	}

	// NOTE d_freq = dc/d_dt=dc/((t1-t0)/1000) => dc*1000/(t1-t0)
	// this last formula is safer using unsigned long values due to truncation
	//	of decimal digits ;-)
	d_dt = (t1 - t0);			// Elapsed time in [s]
	dc = (c1 - c0) * 1000;			// Pulses count variation
	d_freq = (dc==0) ? 0 : (dc/d_dt);	// New frequency
	d_df = d_freq - f0;			// Frequency variation

	// Saving values for next cycle
	f0 = d_freq;
	t0 = t1;
	c0 = c1;
	
	return 0;
}

inline gpsUpdate(void) {

	if (d_gpsPowerState == 0) {
		// Powering off GPS
		digitalWrite(gpsPowerPin, HIGH);
		digitalWrite(led1, LOW);
		// Return with no other parsing
		return;
	}
		
	// Powering on GPS
	digitalWrite(gpsPowerPin, HIGH);
	
	// Update GPS info; this call takes about 1s, this is the minimum
	// 	delay for a speed update...
	if ( checkInterrupt(INTR_GPS) ) {
		digitalWrite(led1, LOW);
		gpsParse();
		ackInterrupt(INTR_GPS);
		digitalWrite(led1, HIGH);
	}

}

inline void loop(void) {
	
	gpsUpdate();
	
	// Updating GPS and ODO data
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
// 		digitalSwitch(ledPin3);
		doUserCmd();
		ackInterrupt(INTR_CMD);
	}

}

int main(void) {

	cli();			// Disable interrupts just in case
	PORTA   = 0x00;		// Give PORTA and "led" a initial startvalue
	DDRA    = 0xFF;		// Set PORTA as output
	initTime();		// Configure timers
	initSerials();		// Configure UART ports
	initGps((unsigned long)GPS_VTG|GPS_GSV|GPS_GLL|GPS_GSA);
	sei();			// Enable interrupts
	
	setup();
	Serial_printLine("AT Command ready");
	
	odoUpdate();		// Initial data update
	while(1) {
		loop();
	}
	
}


