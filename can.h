/*
  can.h

  Copyright (c) 2008-2009 Patrick Bellasi

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

*/

#ifndef Can_h
#define Can_h

#include "derkgps.h"

#define CAN_BITRATE		250

/*_____ C O N S T A N T E S - D E F I N I T I O N  ___________________________*/

#define CAN_MAX_DATA		8
#define CAN_CHANNELS		15
#define CAN_LAST_CHANNEL	14
#define NO_CHANNEL		0xFF

// Constant for configuration of conf_rx and conf_tx
#define CONF_NOBUFFER	0x00
#define CONF_BUFFER	0x01
// Bits 2-4 are used by canDlc_t
#define CONF_NOIDE	0x00
#define CONF_IDE	0x10
#define CONF_NORTR	0x00
#define CONF_RTR	0x20
#define CONF_NOMSK_RTR	0x00
#define CONF_MSK_RTR	0x40
#define CONF_NOMSK_IDE	0x00
#define CONF_MSK_IDE	0x80

// Channel reconfiguration options
#define CONF_CH_DISABLE        0x01
#define CONF_CH_RX_ENABLE      0x02
#define CONF_CH_RXB_ENABLE     0x04

/*_____ T Y P E D E F - D E C L A R A T I O N ________________________________*/

typedef union {
    unsigned long ext;
    unsigned int  std;
    unsigned char tab[4];
} canId_t;

typedef enum {
    CHANNEL_0,
    CHANNEL_1, CHANNEL_2, CHANNEL_3,  CHANNEL_4, CHANNEL_5,CHANNEL_6, CHANNEL_7,
    CHANNEL_8, CHANNEL_9, CHANNEL_10, CHANNEL_11, CHANNEL_12, CHANNEL_13, CHANNEL_14,
} canChannel_t;

typedef enum {
    CONF_DLC_0,
    CONF_DLC_1, CONF_DLC_2, CONF_DLC_3, CONF_DLC_4,
    CONF_DLC_5, CONF_DLC_6, CONF_DLC_7, CONF_DLC_8,
} canDlc_t;

typedef struct {
    canChannel_t ch;
    canId_t filter;
    canId_t mask;
    unsigned char opts;
} canChannelConf_t;

typedef struct {
    canId_t id;
    unsigned char ctrl;
    unsigned char * pData;
} canMsg_t;

void initCan(void);

void canConfChannel_Rx(canChannelConf_t * conf);
void canReadMsg (canMsg_t * msg, unsigned char nextConf);

void canSniff ( void );


#endif
