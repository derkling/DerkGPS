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

//----- ODOMETER
/// Odometer pulses count
unsigned long d_pcount = 0;
/// Last computed odometer pulses frequency
unsigned long d_freq = 0;
/// The old freq value used for Alarms Checking
unsigned d_oldFreq = 0;

/// Current max [ppm/s] EVENT_OVER_SPEED alarm
unsigned long d_minOverSpeed = 0;
/// Current max [ppm/s] decelleration EVENT_EMERGENCY_BREAK alarm
unsigned long d_minEmergencyBreak = 0;
/// Pulses between distance interrupts
unsigned d_distIntrPCount = 0;


/// Frequency variation since last check [Hz]
long d_df = 0;
/// Time interval between last two update [s]
unsigned long d_dt = 0;
/// Last update time [ms]
unsigned long d_odoLastUpdate = 0;
/// Pulses of next distance interrupt
unsigned long d_distIntrNext = 0;

//----- AT Interface
/// Delay ~[s] between dispaly monitor sentences, if 0 DISABLED (default 0);
unsigned d_displayTime = 1;
/// Buffer for sentence display formatting
char d_displayBuff[OUTPUT_BUFFER_SIZE];
/// ATinterface top-halves Interrupt scheduling flags
short d_thIntrCMD = 0;

//----- EVENT GENERATION
///// Last update time [ms]
// unsigned long d_eventsLastUpdate = 0;
///// Events enabled to generate signals
//derkgps_event_t d_activeEvents[EVENT_CLASS_TOT] = {EVENT_NONE, EVENT_NONE}; // Disabling all interrupts by default
derkgps_event_t d_activeEvents[EVENT_CLASS_TOT] = { 0x3F, 0x03 };
/// Events suspended by this code to avoid interrupt storms
derkgps_event_t d_suspendedEvents[EVENT_CLASS_TOT] = {EVENT_NONE, EVENT_NONE};
/// Events pending to be ACKed
derkgps_event_t d_pendingEvents[EVENT_CLASS_TOT] = {EVENT_NONE, EVENT_NONE};
/// Time to reset interrupt line if not readed before [ms]
unsigned long d_intrResetTime = 0;
/// How long an interrupt last [ms]
unsigned d_intrTimeout = 30000;

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

// //----- Loop function static variables
unsigned long t0 = 0;
unsigned long c0 = 0;
unsigned long f0 = 0;

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
// 	unsigned long cc = d_pcount;
	unsigned long cc = odoPulseCount();
	unsigned long cf = d_freq;
	unsigned long time = millis();
	uint8_t ge = d_pendingEvents[EVENT_CLASS_GPS];
	uint8_t oe = d_pendingEvents[EVENT_CLASS_ODO];
	unsigned siv = gpsSatInView();
	unsigned fix = gpsFix();
	char hdop = gpsHdopLevel();
	
	time -= d_displayLastUpdate;
	if ( time < (d_displayTime*1000) ) {
		return;
	}
	
	if (gpsIsPosValid()) {
		// 28 Bytes for this first part
		snprintf(d_displayBuff, OUTPUT_BUFFER_SIZE,
			"0x%02X%02X %8lu %4lu %2u %1u %1c ",
			ge, oe, cc, cf, siv, fix, hdop);
		// This is what we have to append: "+99.9999 +999.9999"
		formatDouble(gpsLat(), d_displayBuff+28, 9);
		formatDouble(gpsLon(), d_displayBuff+28+9, 10);
		d_displayBuff[28+8]=' ';
		
		// This call require a total of:
		// 28+9+10=47 Bytes;
	} else {
		snprintf(d_displayBuff, OUTPUT_BUFFER_SIZE,
			"0x%02X%02X %8lu %4lu %2u %1u %1c NA NA",
			ge, oe, cc, cf, siv, fix, hdop);
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
	intrEnabled = d_activeEvents[event_class] & (unsigned char)event;
	
	// Not-null iff this event is already pending to be ACKed
	alreadyNotified = d_pendingEvents[event_class] & (unsigned char)event;
	
	// saving event
	d_pendingEvents[event_class] |= (unsigned char)event;
	
	// looking if it has to be notified
	if (intrEnabled && !alreadyNotified) {
		pinMode(intReq, OUTPUT);
/* Wait until the interrupt has been read
		delay(DERKGPS_INTR_LEN);
		pinMode(intReq, INPUT);
*/
		if ( d_intrTimeout>0 ) {
			d_intrResetTime  = millis();
			d_intrResetTime += d_intrTimeout;
		}


	}
	
}

// NOTE this method should avoid notification storms.
// Once an event has been notified it should not be notified anymore while
// it still olds... only at the time of reverse event the notifier should be
// re-enabled.
#define SET_EVENT(_evt_)			\
	event = (0x1 << _evt_)
void checkAlarms(void) {
	unsigned long newFreq;
	unsigned newFix;
	uint8_t event = 0;
	
	// Checking ODO Moving Events...
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
	
/*
	// Checking GPS Moving Events...
	if ( gpsSpeed() > MIN_MOVE_SPEED ) {
		// START Moving Event
		SET_EVENT(GPS_EVENT_MOVE);
		notifyEvent(EVENT_CLASS_GPS, event);
		digitalWrite(led3, HIGH);
	} else {
		// STOP Moving Event
		SET_EVENT(GPS_EVENT_STOP);
		notifyEvent(EVENT_CLASS_GPS, event);
		digitalWrite(led3, LOW);
	}
*/
	// Checking for EVENT_EMERGENCY_BREAK event
	// NOTE Acceleration is always OK
	if ( d_minEmergencyBreak ) {
		
		SET_EVENT(ODO_EVENT_EMERGENCY_BREAK);
		
		if ( d_df < 0 ) { // Decelleration: monitoring rate
			d_df = -d_df*1000;
			if ( (d_df/d_dt) > d_minEmergencyBreak &&
				!( d_suspendedEvents[EVENT_CLASS_ODO] & (unsigned char)event ) ) {
				
				// Trigger an interrupt
				notifyEvent(EVENT_CLASS_ODO, event);
				
				// NOTE this code SUPPOSE that we can't modify d_activeEvents
				//	using AT Commands... otherwise we must pay attention
				//	when disabling an event because we don't know anymore if
				//	the user has required it to be disabled or not.
				// Now that the event has been notified we suspend it until
				// we return within the speed limit
				d_suspendedEvents[EVENT_CLASS_ODO] |= (unsigned char)event;
			}
			
		} else { // Acceleration reenable the event notify
			d_suspendedEvents[EVENT_CLASS_ODO] &= (unsigned char)~event;
		}
	}

	// Checking for EVENT_OVER_SPEED event
	if ( d_minOverSpeed ) {
		
		if ( d_freq > d_minOverSpeed) { // Overspeed: event notify
			
			// Over-speed reenable the SAFE_SPEED event notify
			SET_EVENT(ODO_EVENT_SAFE_SPEED);
			d_suspendedEvents[EVENT_CLASS_ODO] &= (unsigned char)~event;
			
			SET_EVENT(ODO_EVENT_OVER_SPEED);
			if ( !(d_suspendedEvents[EVENT_CLASS_ODO] & (unsigned char)event) ) {
				// Trigger an interrupt
				notifyEvent(EVENT_CLASS_ODO, event);
				
				// NOTE this code SUPPOSE that we can't modify d_activeEvents
				//	using AT Commands... otherwise we must pay attention
				//	when disabling an event because we don't know anymore if
				//	the user has required it to be disabled or not.
				// Now that the event has been notified we suspend it until
				// we return within the speed limit
				d_suspendedEvents[EVENT_CLASS_ODO] |= (unsigned char)event;
			}
			
		} else {
			
			// Safe speed reenable the OVER_SPEED event notify
			SET_EVENT(ODO_EVENT_OVER_SPEED);
			d_suspendedEvents[EVENT_CLASS_ODO] &= (unsigned char)~event;
			
			SET_EVENT(ODO_EVENT_SAFE_SPEED);
			if ( !(d_suspendedEvents[EVENT_CLASS_ODO] & (unsigned char)event) ) {
				// Trigger an interrupt
				notifyEvent(EVENT_CLASS_ODO, event);
				
				// NOTE this code SUPPOSE that we can't modify d_activeEvents
				//	using AT Commands... otherwise we must pay attention
				//	when disabling an event because we don't know anymore if
				//	the user has required it to be disabled or not.
				// Now that the event has been notified we suspend it until
				// we return within the speed limit
				d_suspendedEvents[EVENT_CLASS_ODO] |= (unsigned char)event;
			}
		}
		
		// Checking GPS OverSpeed Events...
		
	}
	
	// Checking GPS fix
	newFix = gpsFix();
	if (newFix != d_oldFix) {
		if ( newFix>0 ) {
			SET_EVENT(GPS_EVENT_FIX_GET);
		} else {
			SET_EVENT(GPS_EVENT_FIX_LOSE);
		}
		notifyEvent(EVENT_CLASS_GPS, event);
		d_oldFix = newFix;
	}

/*
	// Checking ODO distance interrupt
	if ( d_distIntrPCount>0 &&
		d_pcount>d_distIntrNext ) {
		SET_EVENT(ODO_EVENT_DISTANCE);
		notifyEvent(EVENT_CLASS_ODO, event);
		d_distIntrNext  = d_pcount;
		d_distIntrNext += d_distIntrPCount;
// 		snprintf(d_displayBuff, OUTPUT_BUFFER_SIZE,
// 			"Next PCount Intr @ %8lu",
// 			d_distIntrNext);
// 		Serial_printLine(d_displayBuff);
	}
*/

	// Event led control
	if (d_pendingEvents[EVENT_CLASS_ODO] ||
		d_pendingEvents[EVENT_CLASS_GPS]) {
		digitalWrite(led2, HIGH);
	} else {
		digitalWrite(led2, LOW);
	}
	
	// Releasing interrupt line after a safe timeout period
	if ( d_intrTimeout>0 &&
		d_intrResetTime < millis() ) {
		// NOTE these interrupts are loose!... if we are not
		//	able to read them within the timeout: than it should
		//	be safer to remove them and go ahead
		d_pendingEvents[EVENT_CLASS_ODO] = EVENT_NONE;
		d_pendingEvents[EVENT_CLASS_GPS] = EVENT_NONE;
		pinMode(intReq, INPUT);
	}

}

/// @return 0 on successfull data update
int odoUpdate(void) {
	unsigned long t1 = 0;
	unsigned long c1 = 0;
	unsigned long dc = 0;
	
	t1 = millis();
	c1 = odoPulseCount();
	
	// 511[ms] = 0x1FF
	if ( (t1-d_odoLastUpdate)>>9 ) {
		d_odoLastUpdate = t1;
	} else {
		// Less the 511 ms since last update
		return -1;
	}
	
	// NOTE millis() return the number of milliseconds since the current
	// program started running, as an unsigned long.
	// This number will overflow (go back to zero), after approximately 49 days.
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
		digitalWrite(gpsAntPowerPin, LOW);
		digitalWrite(led1, LOW);
		gpsReset();
		// Return with no other parsing
		return;
	}
		
	// Powering on GPS
	digitalWrite(gpsAntPowerPin, HIGH);
	digitalWrite(gpsPowerPin, HIGH);
	
	// Update GPS info; this call takes about 1s, this is the minimum
	// 	delay for a speed update...
	if ( checkInterrupt(UART_GPS) ) {
		digitalSwitch(led1);
		gpsParse();
		ackInterrupt(UART_GPS);
	}

}

//----- System Initialization
void setup(void) {

	// Disable interrupts (just in case)
	cli();
	
	// Disable JTAG interface
	// This require TWICE writes in 4 CPU cycles
	sbi(MCUCR, JTD);
	sbi(MCUCR, JTD);
	
	// Configure timers
	initTime();
	
	// Configure odometer
	initOdo();
	
	// Configure UART ports
	initSerials();
	
	// Configure GPS
	initGps((unsigned long)GPS_VTG|GPS_GSV|GPS_GLL|GPS_GSA);
	
	// Configure CAN Bus
	initCan();
	
	// APE Interrupt
	// NOTE to assert an interrupt the INTR line should be asserted LOW for a while
	pinMode(intReq, INPUT);
	digitalWrite(intReq, LOW); // NOTE this will disable the internal Pull-up
	
	// GPS module power control
	pinMode(gpsAntPowerPin, OUTPUT);
	digitalWrite(gpsAntPowerPin, LOW);
	pinMode(gpsPowerPin, OUTPUT);
	digitalWrite(gpsPowerPin, LOW);
	
	// CAN modules power control
	pinMode(can1Power, OUTPUT);
	digitalWrite(can1Power, HIGH); // Forcing Stand-By Mode
	pinMode(can2Power, OUTPUT);
	digitalWrite(can1Power, HIGH); // Forcing Stand-By Mode
	pinMode(canSwitchSelect, OUTPUT);
	digitalWrite(canSwitchSelect, LOW); // Pre-Selecting channel A
	pinMode(canSwitchEnable, OUTPUT);
	digitalWrite(canSwitchEnable, HIGH); // Powering-off switch
	
	// Movement sensors
	pinMode(memsTestPin, OUTPUT);
	digitalWrite(memsTestPin, LOW);
	pinMode(moveSensorIntr, INPUT);
	digitalWrite(moveSensorIntr, LOW); // NOTE this will disable the internal Pull-up
	
	// Odometer input
	pinMode(odoPulsePin, INPUT);
	digitalWrite(odoPulsePin, HIGH); // enable internal Pull-up
	
	// LEDs confiugration
	//	NMEA Parsing Activity LED
	pinMode(led1, OUTPUT);
	digitalWrite(led1, LOW);
	//	Events Pending Monitor LED
	pinMode(led2, OUTPUT);
	digitalWrite(led2, LOW);
	//	Movement LED
	pinMode(led3, OUTPUT);
	digitalWrite(led3, LOW);
	
	// Loop function static variables INITIALIZATION
	t0 = millis();
	c0 = odoPulseCount();
	
	// Powering on GPS
	d_gpsPowerState = 1;
	digitalWrite(gpsAntPowerPin, HIGH);
	digitalWrite(gpsPowerPin, HIGH);
	
	// Initial data update
	odoUpdate();
	
	// Enable interrupts
	sei();
	
}

inline void loop(void) {
	 
	// Updating GPS data
	gpsUpdate();
	
#ifndef TEST_GPS
	// Updating ODO data
	if ( odoUpdate() != 0 ) {
		return;
	}
	
	// Check constraints and issue alarms
	checkAlarms();
	
	// Display summary sentence
	if (d_displayTime) {
		display();
	}
#endif
	// Execute user commands
	if ( checkInterrupt(UART_AT) ) {
		parseCommand();
		ackInterrupt(UART_AT);
	}
	
}

int main(void) {

	setup();
	
	if (d_displayTime) {
	    Serial_printLine("DerkGPS v2.0 by Patrick Bellasi <derkling@gmail.com>");
	}

	while(1) {
#ifdef TEST_CAN
#warning CAN Sniffing TEST enabled
		digitalWrite(canSwitchEnable, LOW);
		digitalWrite(canSwitchSelect, CAN_PORTA);
		digitalWrite(can1Power, LOW);
		digitalWrite(can2Power, HIGH);
		canSniff();
#else
		loop();
#endif
	}
	
}


