/*
odo.c

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

#include "can.h"

/*_____ C O N S T A N T E S - D E F I N I T I O N  ___________________________*/

// CAN Baud Rate Settings for 8MHz
#if 1
#if CAN_BITRATE == 1000
# warning Configuring CAN bitrate @ 1000KB
# define CAN_CANBT1		0x00
# define CAN_CANBT2		0x04
# define CAN_CANBT3		0x13
#elif CAN_BITRATE == 500
# warning Configuring CAN bitrate @ 500KB
# define CAN_CANBT1		0x02
# define CAN_CANBT2		0x04
# define CAN_CANBT3		0x13
#elif CAN_BITRATE == 250
# warning Configuring CAN bitrate @ 250KB
# define CAN_CANBT1		0x06
# define CAN_CANBT2		0x04
# define CAN_CANBT3		0x13
#elif CAN_BITRATE == 200
# warning Configuring CAN bitrate @ 200KB
# define CAN_CANBT1		0x08
# define CAN_CANBT2		0x04
# define CAN_CANBT3		0x13
#elif CAN_BITRATE == 125
# warning Configuring CAN bitrate @ 125KB
# define CAN_CANBT1		0x0E
# define CAN_CANBT2		0x04
# define CAN_CANBT3		0x13
#elif CAN_BITRATE == 100
# warning Configuring CAN bitrate @ 100KB
# define CAN_CANBT1		0x08
# define CAN_CANBT2		0x0C
# define CAN_CANBT3		0x37
#endif
#endif

#if 0
#define CAN_CANBT1		0x06
#define CAN_CANBT2		0x00
#define CAN_CANBT3		0x05
#endif






#define FLAG_TX		0x01
#define FLAG_RX		0x02
#define FLAG_OT		0x04

/*_____ M A C R O S - DE C L A R A T I O N ___________________________________*/

#define CAN_SET_CHANNEL(mob) (CANPAGE = (mob << MOBNB0))
#define CAN_GET_CHANNEL      (CANPAGE >> MOBNB0)

// Configuration of Extended Identifier
#define CAN_SET_EXT_ID_28_21(id) (((*((unsigned char *)(&id)+3))<<3)+((*((unsigned char *)(&id)+2))>>5))
#define CAN_SET_EXT_ID_20_13(id) (((*((unsigned char *)(&id)+2))<<3)+((*((unsigned char *)(&id)+1))>>5))
#define CAN_SET_EXT_ID_12_5(id)  (((*((unsigned char *)(&id)+1))<<3)+((* (unsigned char *)(&id)   )>>5))
#define CAN_SET_EXT_ID_4_0(id)    ((* (unsigned char *)(&id)   )<<3)

#define CAN_SET_EXT_MSK_28_21(mask) CAN_SET_EXT_ID_28_21(mask)
#define CAN_SET_EXT_MSK_20_13(mask) CAN_SET_EXT_ID_20_13(mask)
#define CAN_SET_EXT_MSK_12_5(mask)  CAN_SET_EXT_ID_12_5(mask)
#define CAN_SET_EXT_MSK_4_0(mask)   CAN_SET_EXT_ID_4_0(mask)

// Configuration of Standard Identifier
#define CAN_SET_STD_ID_10_3(id) (((*((unsigned char *)(&id)+1))<<5)+((*(unsigned char *)(&id))>>3))
#define CAN_SET_STD_ID_2_0(id)  ((*(unsigned char *)(&id))<<5)

#define CAN_SET_STD_MSK_10_3(mask) CAN_SET_STD_ID_10_3(mask)
#define CAN_SET_STD_MSK_2_0(mask)  CAN_SET_STD_ID_2_0(mask)

#define CAN_DISABLE_CH        ( CANCDMOB &= ~(0x3 << CONMOB0) )
#define CAN_ENABLE_CH_TX      { CAN_DISABLE_CH ; CANCDMOB |= (0x1 << CONMOB0); }
#define CAN_ENABLE_CH_RX      { CAN_DISABLE_CH ; CANCDMOB |= (0x2 << CONMOB0); }
#define CAN_ENABLE_CH_BUFF    { CAN_DISABLE_CH ; CANCDMOB |= (0x3 << CONMOB0); }

/*_____ L O C A L E S ______________________________________________________*/
unsigned int canOvrTCount = 0;		//
unsigned int canDlcwCount = 0;		//
unsigned int canTxCount = 0;		//
unsigned int canRxCount = 0;		//
unsigned int canBxCount = 0;		// Frame Buffer Receive Interrupt
unsigned int canMObErrCount = 0;	// CAN General error count
unsigned int  canAckMObErrCount = 0;	//	Acknowledgment Error
unsigned int  canForMObErrCount = 0;	//	Form Error
unsigned int  canCrcMObErrCount = 0;	//	CRC Error
unsigned int  canStuMObErrCount = 0;	//	Stuff Error
unsigned int  canBitMObErrCount = 0;	//	 Bit Error (Only in Transmission)
unsigned int canGenErrCount = 0;	// CAN General error count
unsigned int  canAckGenErrCount = 0;	//	Acknowledgment Error
unsigned int  canForGenErrCount = 0;	//	Form Error
unsigned int  canCrcGenErrCount = 0;	//	CRC Error
unsigned int  canStuGenErrCount = 0;	//	Stuff Error
unsigned int canBusOffCount = 0;	//	Bus Off Interrupt Flag

unsigned int  numMObTx;		// MOb number with TxOK
unsigned int  numMObRx;		// MOb number with RxOK

// Status flasg
volatile unsigned char canFlags = 0x00;

char canDebugBuff[32];


static void canResetAllMailbox (void) {
    unsigned char ch, data;
    
    for (ch = 0; ch < CAN_CHANNELS; ch++) {
	CANPAGE  = (ch << MOBNB0);
	CANSTMOB = 0x00;
	CANCDMOB = 0x00;
	CANIDT4  = 0x00;
	CANIDT3  = 0x00;
	CANIDT2  = 0x00;
	CANIDT1  = 0x00;
	CANIDM4  = 0x00;
	CANIDM3  = 0x00;
	CANIDM2  = 0x00;
	CANIDM1  = 0x00;
	for (data = 0; data < CAN_MAX_DATA; data++) {
	    // This pointer will auto-increment if auto-incrementation is used
	    CANMSG = 0x00;
	}
    }    
}

// NOTE This function MUST be called with interrupt disabled
void initCan(void) {
    
    // Resetting CAN controller
    sbi(CANGCON, ENASTB);
    
    // Resetting all mailboxes
    canResetAllMailbox();
    
    // Configuring CAN prescaler
    // Tclk_CANTIM = Tclk_IO x 8 x (CANTCON [7:0] + 1)
    CANTCON = 0xFF;
    
    // Configuring bitrates
    CANBT1 = CAN_CANBT1;
    CANBT2 = CAN_CANBT2;
    CANBT3 = CAN_CANBT3;
    
    // Enable all interrupts
    CANGIE=0xFF;    // with OVRTIM 
//  CANGIE=0xFE;    // without OVRTIM

    // Enable all MOb's interrupts
    CANIE2=0xFF;
    CANIE1=0xFF;
  
    // Enable CAN Controller
    sbi(CANGCON, ENASTB);
    
}


void canConfChannel_Rx(canChannelConf_t * conf) {
    
    CAN_SET_CHANNEL(conf->ch);
    
    // Resetting CAN MOb Status Register
    CANSTMOB = 0x00;
    
    // Resetting CAN MOb Control and DLC Register to default configuration
    CANCDMOB = 0x00;
    
    if ( conf->opts & CONF_IDE ) {
	// 29-bit CAN Identifier Tag Registers 
	CANIDT1 = CAN_SET_EXT_ID_28_21 (conf->filter.ext);
	CANIDT2 = CAN_SET_EXT_ID_20_13 (conf->filter.ext);
	CANIDT3 = CAN_SET_EXT_ID_12_5  (conf->filter.ext);
	CANIDT4 = CAN_SET_EXT_ID_4_0   (conf->filter.ext);
	// RTRTAG, RB1TAG and RB0TAG cleared
	
	// 29-bit CAN Identifier Mask Registers
	CANIDM1 = CAN_SET_EXT_MSK_28_21 (conf->mask.ext);
	CANIDM2 = CAN_SET_EXT_MSK_20_13 (conf->mask.ext);
	CANIDM3 = CAN_SET_EXT_MSK_12_5  (conf->mask.ext);
	CANIDM4 = CAN_SET_EXT_MSK_4_0   (conf->mask.ext);
	// RTRMSK and IDEMSK cleared
	
	// Enable IDentifier Extension" on CAN MOb Control and DLC Register
	sbi(CANCDMOB, IDE);

    } else {
	// 11-bit identifier tag
	CANIDT1 = CAN_SET_STD_ID_10_3 (conf->filter.std);
	CANIDT2 = CAN_SET_STD_ID_2_0 (conf->filter.std);
	// Clear RTRTAG, RB1TAG and RB0TAG
	CANIDT4 = 0x00;
	
	// 11-bit identifier mask
	CANIDM1 = CAN_SET_STD_MSK_10_3 (conf->mask.std);
	CANIDM2 = CAN_SET_STD_MSK_2_0 (conf->mask.std);
	// Clear RTRMSK and IDEMSK
	CANIDM4 = 0x00;
	
	// IDE=0 in CANCDMOB already done at the beginning
    }

    CANCDMOB |= ( CAN_MAX_DATA && 0X0F) ;
    // If DLC of received message < NB_DATA_MAX, DLCW will be set

    if ( conf->opts & CONF_RTR ) {
	// Enabling  "Remote Transmission Request Tag"
	sbi(CANIDT4, RTRTAG);
    }

    if ( conf->opts & CONF_MSK_RTR ) {
	// Enable "Remote Transmission Request Mask" (bit-comparision)
	sbi(CANIDM4, RTRMSK);
    }

    if ( conf->opts & CONF_MSK_IDE ) {
	// Enable "Identifier Extension Mask" (bit-comparision)
	sbi(CANIDM4, IDEMSK);
    }

    // Enable reception on this channel
    if ( conf->opts & CONF_BUFFER ) {
	CAN_ENABLE_CH_BUFF;
    } else {
	CAN_ENABLE_CH_RX;
    }

}

void canReadMsg (canMsg_t * msg, unsigned char nextConf) {
    unsigned char * pData = 0;
    unsigned char dlc2read = 0;
    unsigned char rxConf = CANCDMOB;

    // Save control register
    msg->ctrl = rxConf;
    
    if ( rxConf & CONF_IDE ) {
	msg->id.tab[3] = (CANIDT1 >> 3);
	msg->id.tab[2] = (CANIDT1 << 5) | (CANIDT2 >> 3);
	msg->id.tab[1] = (CANIDT2 << 5) | (CANIDT3 >> 3);
	msg->id.tab[0] = (CANIDT3 << 5) | (CANIDT4 >> 3);
    } else {
	msg->id.std = (CANIDT1 << 3) | (CANIDT2 >> 5);
    }
    
    // Reading msg data
    pData = msg->pData;
    dlc2read = (rxConf & 0x0F);
    for ( ; dlc2read!=0; dlc2read--) {
	*pData++ = CANMSG;
    }
    
    
    //--- Reconfiguring this channel
    if ( nextConf == CONF_CH_DISABLE ) {
	CAN_DISABLE_CH;
    } else if ( nextConf == CONF_CH_RX_ENABLE ) {
	CAN_ENABLE_CH_RX;
    } else {
	CAN_ENABLE_CH_BUFF;
    }
  
}


unsigned char canFindFirstChIt (void) {
    unsigned char chNum;
    
    chNum = (CANHPMOB >> HPMOB0);
    
    return ( (chNum <= CAN_LAST_CHANNEL) ? chNum : NO_CHANNEL );
    
}


static void readOnMOb0 (void) {
    canChannelConf_t conf;
    canMsg_t canRxMsg;
    unsigned char data[CAN_MAX_DATA];
    
    // Configure standard reception on MOb 0
#if 0
    conf.ch = CHANNEL_0;
//     conf.filter.ext = 0UL;
//     conf.mask.ext = 0UL;
    conf.filter.std = 0xF003;
    conf.mask.std = 0xFFFF;
//     conf.opts = 0x00 | CONF_NOIDE | CONF_NORTR | CONF_DLC_8;
    conf.opts = 0x00 | CONF_NOIDE | CONF_MSK_IDE | CONF_NORTR | CONF_DLC_8;
    canConfChannel_Rx(&conf);
#endif

    // Configure extended reception on MOb 1
    conf.ch = CHANNEL_1;
//     conf.filter.ext = 0UL;
//     conf.mask.ext = 0UL;
    conf.filter.ext = 0xF00300;
    conf.mask.ext = 0xFFFFFF;
    conf.opts = 0x00 | CONF_IDE | CONF_MSK_IDE | CONF_NORTR | CONF_DLC_8;
    canConfChannel_Rx(&conf);
    
    conf.ch = CHANNEL_2;
    //     conf.filter.ext = 0UL;
    //     conf.mask.ext = 0UL;
    conf.filter.ext = 0xF00400;
    conf.mask.ext = 0xFFFFFF;
    conf.opts = 0x00 | CONF_IDE | CONF_MSK_IDE | CONF_NORTR | CONF_DLC_8;
    canConfChannel_Rx(&conf);
    
digitalWrite(led1, HIGH);
    while ( canFlags == 0x00 ) {
	/* Waiting for a CAN Message */;
    }
digitalWrite(led1, LOW);
	
    // Get MOb number of the incomming message
    CAN_SET_CHANNEL(numMObRx);
    
    // Read incomming message
    canRxMsg.pData = data;
    canReadMsg(&canRxMsg, CONF_CH_DISABLE);
    
    //----- Echo message on UART
    
    // Print ID
    if ( canRxMsg.ctrl & CONF_IDE ) {
	snprintf(canDebugBuff, 32, "Ch-%d %08lX ",
	    numMObRx, canRxMsg.id.ext);
    } else {
	snprintf(canDebugBuff, 32, "Ch-%d %04X ",
	    numMObRx, canRxMsg.id.std);
    }
    Serial_printStr(canDebugBuff);
    
    // Print DATA
    snprintf(canDebugBuff, 32, "%02X %02x %02x %02x %02X %02x %02x %02x",
	      canRxMsg.pData[7],
	      canRxMsg.pData[6],
	      canRxMsg.pData[5],
	      canRxMsg.pData[4],
	      canRxMsg.pData[3],
	      canRxMsg.pData[2],
	      canRxMsg.pData[1],
	      canRxMsg.pData[0]
	      );
    Serial_printLine(canDebugBuff);
    
    // Resetting flag
    canFlags = 0x00;
    
}

void canSniff ( void ) {
 
    while ( 1 ) {
	readOnMOb0();
    }
    
}


/*_____ INTERRUPT HANDLERS____________________________________________________*/

SIGNAL(SIG_CAN_INTERRUPT1) {
    unsigned char ch;
    unsigned char savedCanPage;
    unsigned char error;
    
    // Save the current page
    savedCanPage = CANPAGE;

digitalSwitch(led3);
    
    ch = canFindFirstChIt();
    if ( ch != NO_CHANNEL ) {
	// MOB Interrupts
	
	CAN_SET_CHANNEL(ch);
	
// snprintf(canDebugBuff, 32, "MObInt: %02X Ch-%d %d", CANSTMOB, ch, (CANCDMOB & 0x0F) );
// Serial_printLine(canDebugBuff);
	
	if ( tbi(CANSTMOB, DLCW) ) {
	    canDlcwCount++;
	}
	
	if ( tbi(CANSTMOB, RXOK) ) {
	    canRxCount++;
	    canFlags = FLAG_RX;
	    numMObRx = ch;
	    CAN_DISABLE_CH;
digitalSwitch(led2);
	}
	
	if ( tbi(CANSTMOB, TXOK) ) {
	    canTxCount++;
	    canFlags = FLAG_TX;
	    numMObTx = ch;
	    CAN_DISABLE_CH;
	}
	
	error = CANSTMOB & 0x1F;
	if ( error ) {
	    canMObErrCount++;
	    if ( error & 0x01 ) canAckMObErrCount++;
	    if ( error & 0x02 ) canForMObErrCount++;
	    if ( error & 0x04 ) canCrcMObErrCount++;
	    if ( error & 0x08 ) canStuMObErrCount++;
	    if ( error & 0x10 ) canBitMObErrCount++;
	}
	
	// Reset all MOb Int, Keep WDLC
	CANSTMOB = CANSTMOB & 0x80;
    
    } else {
	// No Channel matchs with the interrupt => General IT
	
// snprintf(canDebugBuff, 32, "GenErr: %02X", CANGIT);
// Serial_printLine(canDebugBuff);
	
	// Reset Int_flag setting it "1"
	error = CANGIT & 0x1F;
	if (error != 0x00) {
	    canGenErrCount++;
	    if ( error & 0x01 ) canAckGenErrCount++;
	    if ( error & 0x02 ) canForGenErrCount++;
	    if ( error & 0x04 ) canCrcGenErrCount++;
	    if ( error & 0x08 ) canStuGenErrCount++;
	    // Reset Int_ERR_GEN done (global) in IT_Handler
	}
	
	if ( tbi(CANGIT,BXOK) ) {
	    canBxCount++;
	    // Reset Int_Buffer_OK done (global) in IT_Handler
	}
	
	if ( tbi(CANGIT,BOFFIT)) {
	    canBusOffCount++;
	    Serial_printLine("\r\n  CAN Error Bus Off ");
	    // Reset Int_Bus_Off done in (global) IT_Handler
	}
	
	// New for AVR
	CANGIT = 0x5F;
    } 
    
    // Restore the previous page
    CANPAGE = savedCanPage;
    
}

SIGNAL(SIG_CAN_OVERFLOW1) {
    canOvrTCount++;
//     PORTA=(unsigned char)(canOvrTCount);
    canFlags &= (0x01 << FLAG_OT); 
}
