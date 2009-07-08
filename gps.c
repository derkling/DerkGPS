/*
  LibGPS.cpp - Library to interface a serial GPS
  Copyright (c) 2007 Patrick Bellasi.  All right reserved.
*/

#include "gps.h"
#include <math.h>

# define GpsRead()	read(UART_GPS)
# define GpsAvailable()	available(UART_GPS)

#ifdef TEST_GPS
# define GpsDebugChr(CHR)	print(UART_AT, CHR)
# define GpsDebugStr(STR)	printStr(UART_AT, STR)
# define GpsDebugToken(STR)	printStr(UART_AT, STR); printStr(UART_AT, " ")
# define GpsDebugNewLine()	printLine(UART_AT, "")
# define GpsDebugDumpNMEA()			\
	do {					\
		byte = gpsReadSerial();		\
		GpsDebugChr(byte);		\
	} while ( byte != 0x0D );		\
	return;
#else
# define GpsDebugChr(CHR)
# define GpsDebugStr(STR)
# define GpsDebugToken(STR)
# define GpsDebugNewLine()
# define GpsDebugDumpNMEA()
#endif

//-----[ GPS Specific Protocol Commands ]---------------------------------------

struct gps_cmd {
	char *cmd;
	uint8_t size;
};

#define GPS_CMD(_name)					\
	{_name, sizeof(_name)/sizeof(uint8_t)}


//-----[ Locals ]---------------------------------------------------------------

/// The mask of enabled sentences
unsigned long enSentence;
	
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
double pdop = 25;
/// HDOP
double hdop = 25;
/// VDOP
double vdop = 25;

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

//--- GPS Binary Command Support
char gps_cmd_cold_start[] = {0xb5,0x62,0x06,0x04,0x04,0x00,0xff,0x07,0x02,0x00,0x16,0x79};
char gps_cmd_hot_start[] = {0xb5,0x62,0x06,0x04,0x04,0x00,0x00,0x00,0x02,0x00,0x10,0x68};
char gps_cmd_warm_start[] = {0xb5,0x62,0x06,0x04,0x04,0x00,0x01,0x00,0x02,0x00,0x11,0x6c};

struct gps_cmd cmds[] = {
	GPS_CMD(gps_cmd_cold_start),
	GPS_CMD(gps_cmd_hot_start),
	GPS_CMD(gps_cmd_warm_start),
};

#define GPS_CMD_COUNT sizeof(cmds)/sizeof(struct gps_cmd)



//----- Initialization
void initGps(unsigned long mask) {
	// Bitmask of sentences to be parsed
	enSentence = mask;
}

//----- Local utility methods
char gpsReadSerial(void) {
	char byte;
	
	// Waiting for a char being available...
	while ( !GpsAvailable() )
		;

	// Read the incoming byte
	byte = GpsRead();
	
	// Echoing readed char (if TEST_GPS defined)
	GpsDebugChr(byte);
	
	return byte;
	
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

//----- GPS Binary Command support
int gpsSendCmd(uint8_t index) {
	uint8_t i;

	if ( index >= GPS_CMD_COUNT ) {
		return -1;
	}

	for (i=0; i<cmds[index].size; i++) {
		print(UART_GPS, cmds[index].cmd[i]);
	}

	return 0;
}

//----- Parsing sentence type
inline gpsSentence_t gpsParseType_G(void) {
	
	byte = gpsReadSerial();
// 	GpsDebugChr(byte);
	switch ( byte ) {
	case 'L':
		byte = gpsReadSerial();
// 		GpsDebugChr(byte); GpsDebugChr(' ');
		switch (byte) {
		case 'L':
			// GLL - Geographic Position - Latitude/Longitude
			// eat-up the following ','
			byte = gpsReadSerial();
			return GPS_GLL;
		}
	case 'S':
		byte = gpsReadSerial();
// 		GpsDebugChr(byte); GpsDebugChr(' ');
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
// 	GpsDebugChr(byte);
	switch ( byte ) {
	case 'M':
		byte = gpsReadSerial();
// 		GpsDebugChr(byte); GpsDebugChr(' ');
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
// 	GpsDebugChr(byte);
	switch ( byte ) {
	case 'T':
		byte = gpsReadSerial();
// 		GpsDebugChr(byte); GpsDebugChr(' ');
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
// 	GpsDebugChr(byte);
	switch ( byte ) {
		case 'G':
			type = gpsParseType_G();
			break;
		case 'R':
			type = gpsParseType_R();
			break;
		case 'V':
			type = gpsParseType_V();
			break;
		default:
			type = GPS_UNK;
	}
	
	return type;
	
}


//--- Parsing sentences

void updateLatLon(void) {
	
	gpsGetToken(buff); //GpsDebugToken(buff);
	lat  = strtod(buff, (char **)NULL);
	gpsGetToken(buff); //GpsDebugToken(buff);
	if (buff[0] == 'S')
		lat = -lat;

	gpsGetToken(buff); //GpsDebugToken(buff);
	lon  = strtod(buff, (char **)NULL);
	gpsGetToken(buff); //GpsDebugToken(buff);
	if (buff[0] == 'W')
		lon = -lon;
}

// GLL - Geographic Position - Latitude/Longitude
inline void gpsParseGLL(void) {

// 	GpsDebugDumpNMEA();
	
	updateLatLon();
	
	gpsGetToken(buff);
	utc = strtoul(buff, (char **)NULL, 10);

	gpsGetToken(buff);
	validity = (buff[0] == 'A') ? FIX_VALID : FIX_INVALID;
	
}

// VTG - Track made good and Ground speed
inline void gpsParseVTG(void) {

// 	GpsDebugDumpNMEA();

	if (!validity)
		return;
	
	gpsGetToken(buff);
	if (buff[0]!=0) {
		// The Track Degrees is not present when not computed
		GpsDebugToken(buff);
		dir = strtod(buff, (char **)NULL);
	} else {
		// Don't update dir when it is not computed by the GPS
		GpsDebugToken("?");
	}
	
	gpsNextToken(5);
	gpsGetToken(buff);
// 	GpsDebugToken(buff);
	kmh = strtod(buff, (char **)NULL);
	knots = (unsigned long)(kmh/1.852);
	
}

// GSA - GPS DOP and active satellites
inline void gpsParseGSA(void) {

// 	GpsDebugDumpNMEA();
	
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

// 	GpsDebugDumpNMEA();

	gpsNextToken(2);
	gpsGetToken(buff);
	siv = strtoul(buff, (char **)NULL, 10);
}

// RMC - 
inline void gpsParseRMC(void) {

// 	GpsDebugDumpNMEA();

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
	pdop = 25;
	hdop = 25;
	vdop = 25;
	siv = 0;
	date = 0;
	knots = 0;
	var = 0;
	varEst = 1;
}

// This method should update just one sentence each time is called
void gpsParse(void) {
	gpsSentence_t type;
// 	unsigned char chksum = 0;
		
	// Ckecking if the pending sentence is of interest
	type = gpsParseType();
	switch(type) {
	case GPS_GLL:
		gpsParseGLL();
		//makeUpdated(GPS_GLL);
		break;
	case GPS_GSA:
		gpsParseGSA();
		//makeUpdated(GPS_GSA);
		break;
	case GPS_GSV:
		gpsParseGSV();
		//makeUpdated(GPS_GSV);
		break;
	case GPS_RMC:
		gpsParseRMC();
		//makeUpdated(GPS_RMC);
		break;
	case GPS_VTG:
		gpsParseVTG();
		//makeUpdated(GPS_VTG);
		break;
	default:
		break;
	}
	
// 	GpsDebugNewLine();
	
	// Consuming input buffer until we reach the CR char
	do {
		byte = gpsReadSerial();
	} while ( byte != 0x0D );
	
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

unsigned long gpsTime(void) {
	return utc;
}

short gpsIsPosValid(void) {
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

char gpsHdopLevel(void) {
    unsigned hdop;
    
    hdop = gpsHdop()*10;
    
    if ( hdop>210) {
	// POOR
	// At this level, measurements are inaccurate by as much as half
	// a football field and should be discarded.
	return 'P';
    }
    if ( hdop>90) {
	// FAIR
	// Represents a low confidence level. Positional measurements
	// should be discarded or used only to indicate a very rough
	// estimate of the current location.
	return 'F';
    }
    if ( hdop>70) {
	// MODERATE
	// Positional measurements could be used for calculations, but
	// the fix quality could still be improved. A more open view of
	// the sky is recommended.
	return 'M';
    }
    if ( hdop>40) {
	// GOOD
	// Represents a level that marks the minimum appropriate for
	// making business decisions. Positional measurements could be
	// used to make reliable in-route navigation suggestions to the
	// user.
	return 'G';
    }
    if ( hdop>20) {
	// EXCELLENT
	// At this confidence level, positional measurements are
	// considered accurate enough to meet all but the most
	// sensitive applications.
	return 'E';
    }
    
    // IDEAL
    // This is the highest possible confidence level to be used for
    // applications demanding the highest possible precision at all times
    return 'I';
    
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
