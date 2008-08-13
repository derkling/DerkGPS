/*
  gps.h - Interface to serial GPS
  Copyright (c) 2007 Patrick Bellasi.  All right reserved.
*/

#ifndef gps_h
#define gps_h

#include "derkgps.h"

/// NMEA valid sentence type
typedef enum sentenceType {
	GPS_UNK = 0x00000000,	// Sentence not yet supported
	//GPS_ALM = 0x00000001,	//GPS Almanac Data
	//GPS_APA = 0x00000000,	//Autopilot format
	//GPS_APB = 0x00000000,	//Autopilot format
	//GPS_ASD = 0x00000000,	//Autopilot System Data
	//GPS_BEC = 0x00000002,	//Bearing & Distance to Waypoint, Dead Reckoning
	//GPS_BOD = 0x00000004,	//Bearing, Origin to Destination
	//GPS_BWC = 0x00000008,	//Bearing & Distance to Waypoint, Great Circle
	//GPS_BWR = 0x00000010,	//Bearing & Distance to Waypoint, Rhumb Line
	//GPS_BWW = 0x00000020,	//Bearing, Waypoint to Waypoint
	//GPS_DBT = 0x00000000,	//Depth Below Transducer
	//GPS_DCN = 0x00000000,	//Decca Position
	//GPS_DPT = 0x00000000,	//Depth
	//GPS_FSI = 0x00000040,	//Frequency Set Information
	//GPS_GGA = 0x00000080,	//Global Positioning System Fix Data
	//GPS_GLC = 0x00000100,	//Geographic Position, Loran-C
	GPS_GLL = 0x00000200,	//Geographic Position, Latitude
	//GPS_GRS = 0x00000400,	//GPS Range Residuals
	GPS_GSA = 0x00000800,	//GPS DOP and Active Satellites
	//GPS_GST = 0x00001000,	//GPS Pseudorange Noise Statistics
	GPS_GSV = 0x00002000,	//GPS Satellites in View
	//GPS_GXA = 0x00004000,	//TRANSIT Position
	//GPS_HDG = 0x00008000,	//Heading, Deviation & Variation
	//GPS_HDT = 0x00010000,	//Heding, True
	//GPS_HSC = 0x00020000,	//Heading Steering Command
	//GPS_LCD = 0x00000000,	//Loran-C Signal Data
	//GPS_MSK = 0x00000000,	//Control for a Beacon Receiver
	//GPS_MSS = 0x00000000,	//Beacon Receiver Status
	//GPS_MTA = 0x00000000,	//Air Temperature (to be phased out)
	//GPS_MTW = 0x00000000,	//Water Temperature
	//GPS_MWD = 0x00000000,	//Wind Direction
	//GPS_MWV = 0x00000000,	//Wind Speed and Angle
	//GPS_OLN = 0x00000000,	//Omega Lane Numbers
	//GPS_OSD = 0x00000000,	//Own Ship Data
	//GPS_R00 = 0x00040000,	//Waypoint active route (not standard)
	//GPS_RMA = 0x00000000,	//Recommended Minimum Specific Loran-C Data
	//GPS_RMB = 0x00000000,	//Recommended Minimum Navigation Information
	GPS_RMC = 0x00080000,	//Recommended Minimum Navigation Information
	//GPS_ROT = 0x00000000,	//Rate of Turn
	//GPS_RPM = 0x00100000,	//Revolutions
	//GPS_RSA = 0x00200000,	//Rudder Sensor Angle
	//GPS_RSD = 0x00000000,	//RADAR System Data
	//GPS_RTE = 0x00000000,	//Routes
	//GPS_SFI = 0x00000000,	//Scanning Frequency Information
	//GPS_STN = 0x00000000,	//Multiple Data ID
	//GPS_TRF = 0x00000000,	//Transit Fix Data
	//GPS_TTM = 0x00000000,	//Tracked Target Message
	//GPS_VBW = 0x00000000,	//Dual Ground
	//GPS_VDR = 0x00400000,	//Set and Drift
	//GPS_VHW = 0x00000000,	//Water Speed and Heading
	//GPS_VLW = 0x00000000,	//Distance Traveled through the Water
	//GPS_VPW = 0x00800000,	//Speed, Measured Parallel to Wind
	GPS_VTG = 0x01000000,	//Track Made Good and Ground Speed
	//GPS_WCV = 0x02000000,	//Waypoint Closure Velocity
	//GPS_WNC = 0x04000000,	//Distance, Waypoint to Waypoint
	//GPS_WPL = 0x08000000,	//Waypoint Location
	//GPS_XDR = 0x00000000,	//Transducer Measurements
	//GPS_XTE = 0x00000000,	//Cross-Track Error, Measured
	//GPS_XTR = 0x00000000,	//Cross-Track Error, Dead Reckoning
	//GPS_ZDA = 0x10000000,	//UTC Date / Time and Local Time Zone Offset
	//GPS_ZFO = 0x00000000,	//UTC & Time from Origin Waypoint
	//GPS_ZTG = 0x00000000,	//UTC & Time to Destination Waypoint
} gpsSentence_t;

#define	FIX_NONE	0
#define	FIX_ASSIST	1
#define	FIX_2D		2
#define	FIX_3D		3

#define FIX_INVALID	0
#define FIX_VALID	1

#define SUD		0
#define NORD		1
#define OVEST		0
#define EST		1

/// Initialize GPS data structures
/// @param mask the ORed mask of gpsSentence_t sentences to parse at each update
void initGps(unsigned long mask);

/// Define sentences to enable.
/// @param confMask an ORed mask of gpsSentence_t to enable.
void gpsConfig(unsigned long mask);

/// Reset GPS variables state.
/// This method must be called after a power-down
void gpsReset(void);

/// Update enabled sentences values.
void gpsParse(void);

//--- GLL - Geographic Position - Latitude/Longitude
double		gpsLat(void);
double		gpsLon(void);
unsigned long	gpsTime(void);
short		gpsIsPosValid(void);

//--- VTG - Track made good and Ground speed
double		gpsSpeed(void);
double		gpsDegree(void);

//--- GSA - GPS DOP and active satellites
unsigned	gpsFix(void);
double		gpsPdop(void);
double		gpsHdop(void);
double		gpsVdop(void);

//--- GSV - GPS Satellites in View
unsigned	gpsSatInView(void);

//--- RMC - Recommended Minimum Navigation Information
unsigned	gpsRMC(char *buff, uint8_t size);
unsigned long	gpsDate(void);
double		gpsKnots(void);
double		gpsVar(void);

#endif

