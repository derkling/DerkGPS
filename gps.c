/*
  LibGPS.cpp - Library to interface a serial GPS
  Copyright (c) 2007 Patrick Bellasi.  All right reserved.
*/

#include "gps.h"
#include <math.h>

#define GpsRead()	read(UART0)
#define GpsAvailable()	available(UART0)

#if 0
# define GpsDebugChr(CHR)	print(UART1, CHR)
# define GpsDebugStr(STR)	printStr(UART1, STR)
# define GpsDebugToken(STR)	printStr(UART1, STR); printStr(UART1, " ")
# define GpsDebugNewLine()	printLine(UART1, "")
#else
# define GpsDebugChr(CHR)
# define GpsDebugStr(STR)
# define GpsDebugToken(STR)
# define GpsDebugNewLine()
#endif

#define makeUpdated(SENTENCE)	toUpdate &= ~SENTENCE

//-----[ Locals ]---------------------------------------------------------------

/// The mask of enabled sentences
unsigned long enSentence;
/// Sentences not yet parsed
unsigned long toUpdate;
/// Max number of attempts to update a sentence type
unsigned int maxSentences = 0;
	
//--- GLL - Geographic Position - Latitude/Longitude
/// Last parsed Latitude
double lat;
/// Last parsed Longitude
double lon;
/// UTC of Last valid position
unsigned long utc = 0;
/// True if the current position is a valid data
short validity = FIX_INVALID;

//--- VTG - Track made good and Ground speed
/// Speed [Km/h]
double kmh = 0;
/// Direction [degree]
double dir = 0;

//--- GSA - GPS DOP and active satellites
/// Current fix type
unsigned fix = FIX_NONE;
/// PDOP
double pdop = 100;
/// HDOP
double hdop = 100;
/// VDOP
double vdop = 100;

//---  GSV - Satellites in view
/// Total number of satellites in view
unsigned long siv;

//--- RMC - Recommended Minimum Navigation Information
/// Date (ddmmyy)
unsigned long date = 0;
/// Speed [knots]
double knots = 0;
/// Magnetic Variation [degrees]
double var = 0;
short varEst = 1;

//--- Parsing vars
char byte;
char buff[16];


//----- Initialization
void initGps(unsigned long mask) {
	// Bitmask of sentences to be parsed
	enSentence = mask;
}

//----- Local utility methods
char gpsReadSerial(void) {

	// Waiting for a char being available...
	while ( !GpsAvailable() )
		;

	// Read the incoming byte
	return GpsRead();
	
}

inline short gpsSentenceEnabled(gpsSentence_t type) {
	return (type & enSentence);
}

void gpsNextToken(char n) {
	do { do {
		byte = gpsReadSerial();
	} while (byte!=',');
	} while (--n);
}

void gpsGetToken(char *buf) {
	
	byte = gpsReadSerial();
	while ( (byte != ',') && (byte != '*') ) {
		*buf = byte; buf++;
		byte = gpsReadSerial();
	};
	*buf = 0;
}

double minToDec(double nmea) {
	long deg;
	double min;
	double dec;
	
	deg = (long)floor(nmea/100.0);
	min = nmea-(deg*100.0);
	dec = (double)deg + (min/60.0);
	
	return dec;
}

//----- Parsing sentence type
inline gpsSentence_t gpsParseType_G(void) {
	
	byte = gpsReadSerial();
	switch ( byte ) {
	case 'L':
		byte = gpsReadSerial();
		switch (byte) {
		case 'L':
			// GLL - Geographic Position - Latitude/Longitude
			// eat-up the following ','
			byte = gpsReadSerial();
			return GPS_GLL;
		}
	case 'S':
		byte = gpsReadSerial();
		switch (byte) {
		case 'A':
			// GSA - GPS DOP and active satellites
			// eat-up the following ','
			byte = gpsReadSerial();
			return GPS_GSA;
		case 'V':
			// GSV - Satellites in view
			// eat-up the following ','
			byte = gpsReadSerial();
			return GPS_GSV;
		}
	}
	return GPS_UNK;
}

inline gpsSentence_t gpsParseType_R(void) {
	
	byte = gpsReadSerial();
	switch ( byte ) {
	case 'M':
		byte = gpsReadSerial();
		switch (byte) {
		case 'C':
			// RMC - Recommended Minimum Navigation Information
			// eat-up the following ','
			byte = gpsReadSerial();
			return GPS_RMC;
		}
	}
	return GPS_UNK;
}

inline gpsSentence_t gpsParseType_V(void) {
	
	byte = gpsReadSerial();
	switch ( byte ) {
	case 'T':
		byte = gpsReadSerial();
		switch (byte) {
		case 'G':
			// VTG -  Track Made Good and Ground Speed
			// eat-up the following ','
			byte = gpsReadSerial();
			return GPS_VTG;
		}
	}
	return GPS_UNK;
}

inline gpsSentence_t gpsParseType(void) {
	gpsSentence_t type;
	
	// Get to next sentence start '$'
	do {
		byte = gpsReadSerial();
	} while ( byte != '$' );
	
	// Eat-up two unneeded 'GP' bytes
	byte = gpsReadSerial();
	byte = gpsReadSerial();
	
	// Parse sentence type start
	byte = gpsReadSerial();
	switch ( byte ) {
		case 'G':
			type = gpsParseType_G();
			return type;
		case 'R':
			type = gpsParseType_R();
			return type;
		case 'V':
			type = gpsParseType_V();
			return type;
	}
	
	return GPS_UNK;
	
}


//--- Parsing sentences

void updateLatLon(void) {
	gpsGetToken(buff); GpsDebugToken(buff);
	lat  = strtod(buff, (char **)NULL);
	gpsGetToken(buff); GpsDebugToken(buff);
	if (buff[0] == 'S')
		lat = -lat;

	gpsGetToken(buff); GpsDebugToken(buff);
	lon  = strtod(buff, (char **)NULL);
	gpsGetToken(buff); GpsDebugToken(buff);
	if (buff[0] == 'W')
		lon = -lon;
}

// GLL - Geographic Position - Latitude/Longitude
inline void gpsParseGLL(void) {
	
	GpsDebugStr("GLL ");
	
	updateLatLon();
	
	gpsGetToken(buff);
	utc = strtoul(buff, (char **)NULL, 10);

	gpsGetToken(buff);
	validity = (buff[0] == 'A') ? FIX_VALID : FIX_INVALID;
	
	GpsDebugNewLine();
	
}

// VTG - Track made good and Ground speed
inline void gpsParseVTG(void) {
	
	gpsGetToken(buff);
	dir = strtod(buff, (char **)NULL);

	gpsNextToken(5);
	gpsGetToken(buff);
	kmh = strtod(buff, (char **)NULL);
	knots = (unsigned long)(kmh/1.852);
}

// GSA - GPS DOP and active satellites
inline void gpsParseGSA(void) {
	
	gpsNextToken(1);
	gpsGetToken(buff);
	switch(buff[0]) {
	case '1':
		fix = FIX_NONE;
		break;
	case '2':
		fix = FIX_2D;
		break;
	case '3':
		fix = FIX_3D;
		break;
	}

	gpsNextToken(12);
	gpsGetToken(buff);
	pdop = strtod(buff, (char **)NULL);
	
	gpsGetToken(buff);
	hdop = strtod(buff, (char **)NULL);
	
	gpsGetToken(buff);
	vdop = strtod(buff, (char **)NULL);
}

// GSV - 
inline void gpsParseGSV(void) {
	gpsNextToken(2);
	gpsGetToken(buff);
	siv = strtoul(buff, (char **)NULL, 10);
}

// RMC - 
inline void gpsParseRMC(void) {

	gpsGetToken(buff);
	utc = strtoul(buff, (char **)NULL, 10);

	gpsGetToken(buff);
	validity = (buff[0] == 'A') ? FIX_VALID : FIX_INVALID;

	updateLatLon();
	
	gpsGetToken(buff);
	knots = strtod(buff, (char **)NULL);
	kmh = knots*1.852;

	gpsGetToken(buff);
	dir = strtod(buff, (char **)NULL);

	gpsGetToken(buff);
	date = strtoul(buff, (char **)NULL, 10);

	gpsGetToken(buff);
	var  = strtod(buff, (char **)NULL);

	gpsGetToken(buff);
	varEst = (buff[0] == 'E') ? EST : OVEST;
}



//----- Public methods
void gpsConfig(unsigned long mask) {
	enSentence = mask;
}

// Reset GPS variables state: to be called after a power down
void gpsReset(void) {
	lat = 99.999;
	lon = 999.999;
	utc = 0;
	validity = FIX_INVALID;
	kmh = 0;
	dir = 0;
	fix = FIX_NONE;
	pdop = 100;
	hdop = 100;
	vdop = 100;
	siv = 0;
	date = 0;
	knots = 0;
	var = 0;
	varEst = 1;
}

// This method should update just one sentence each time is called
void gpsParse(void) {
	gpsSentence_t type;
	
	if (!toUpdate || !maxSentences) {
		// Start a new update
		toUpdate = enSentence;
		maxSentences = GPS_MAXSENTENCE;
	}
	
// 	while(toUpdate && maxSentences) {
		
	// Ckecking if the pending sentence is of interest
	type = gpsParseType();
	if ( !gpsSentenceEnabled(type) ) {
		maxSentences--;
// 		continue;
		return;
	}
	
	switch(type) {
	case GPS_GLL:
		gpsParseGLL();
		makeUpdated(GPS_GLL);
		break;
	case GPS_GSA:
		gpsParseGSA();
		makeUpdated(GPS_GSA);
		break;
	case GPS_GSV:
		gpsParseGSV();
		makeUpdated(GPS_GSV);
		break;
	case GPS_RMC:
		gpsParseRMC();
		makeUpdated(GPS_RMC);
		break;
	case GPS_VTG:
		gpsParseVTG();
		makeUpdated(GPS_VTG);
		break;
	default:
		return;
	}
	
// 	}
	
}


double gpsLat(void) {
	
	if (validity)
		return minToDec(lat);
	
	return 99.999;
}

double gpsLon(void) {
	
	if (validity)
		return minToDec(lon);
	
	return 999.999;
}

unsigned long	gpsTime(void) {
	return utc;
}

short	gpsIsPosValid(void) {
	return validity;
}

//--- VTG - Track made good and Ground speed
double gpsSpeed(void) {
	return kmh;
}

double gpsDegree(void) {
	return dir;
}

//--- GSA - GPS DOP and active satellites
unsigned gpsFix(void) {
	return fix;
}

double gpsPdop(void) {
	return pdop;
}

double gpsHdop(void) {
	return hdop;
}

double gpsVdop(void) {
	return vdop;
}

//--- GSV - GPS Satellites in View
unsigned gpsSatInView(void) {
	return siv;
}

//--- RMC - Recommended Minimum Navigation Information
unsigned gpsRMC(char *buff, uint8_t size) {
	unsigned len;
	
	//RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,xxxx,x.x,a,m,*hh<CR><LF>
	len = snprintf(buff, size, "%lu,%c,%f,%c,%f,%c,%f,%f,%lu,%f,%c",
			utc,				// UTC Time
			validity ? 'A' : 'V',		// Status, V=Navigation receiver warning A=Valid
   			(lat>0) ? lat : -lat,		// Latitude
			(lat>0) ? 'N' : 'S',		// N or S
			(lon>0) ? lon : -lon,		// Longitude
			(lon>0) ? 'E' : 'W',		// E or W
			knots,				// Speed over ground, knots
			dir,				// Track made good, degrees true
			date,				// Date, ddmmyy
			var,				// Magnetic Variation, degrees
			(var>0) ? 'E' : 'W'		// E or W
		);
	return len;
}

unsigned long gpsDate(void) {
	return date;
}

double gpsKnots(void) {
	return knots;
}

double gpsVar(void) {
	return var;
}
