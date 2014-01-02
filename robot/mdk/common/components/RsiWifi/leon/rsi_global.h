/**
 * @file
 * @version		2.3.0.0
 * @date 		2011-May-30
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 * @brief HEADER, GLOBAL, Global Header file, the things that must be almost everywhere 
 *
 * @section Description
 * This is the top level global.h file for things that need to be in every module
 *
 *
 */


#ifndef _RSIGLOBAL_H_
#define _RSIGLOBAL_H_

#include <stdio.h>

/**
 * Global defines
 */


/// Some things are boolean values
#define RSI_TRUE        1	
#define RSI_FALSE	0

/// Interrupt Mode Selection
#define RSI_INTERRUPTS


#ifndef SPI_INTERRUPT_LEVEL
#define SPI_INTERRUPT_LEVEL 3
#endif

/// Polled Mode selection
#define RSI_POLLED

#define RSI_MODULE_23_24                 0    /// Enable it only in case of 23 or 24 module firmware
#define RSI_FIRMWARE_UPGRADE             0
#define RSI_LOAD_SBDATA2_FROM_HOST       1    /// set it to 1 to load sbdata2 from host
//#define RSI_63BYTE_PSK_SUPPORT         1    // make it 1 to in case 63 Byte PSK feature enabled


//#define RSI_LITTLE_ENDIAN                       0
// 32bit support still WIP
//#define RSI_BIT_32_SUPPORT

 /**
* Global Defines
*/
// todo move these as global variables
// passed down from init
#define RSI_RESET_PIN 105
#define RSI_IRQ_PIN   54

/*
 * @ Type Definitions
 */
typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned long	uint32;
typedef signed char	int8;
typedef signed short	int16;
typedef signed long	int32;

/*
 * @Time out for various operations
 */

#ifndef RSI_HWTIMER 
/*@ need to define this macro if h/w timer is available and it should increment spiTimer2, spiTimer1 */
#define RSI_TICKS_PER_SECOND   180000000 
/*@ approximate ticks for timeout implementation */
#define RSI_INC_TIMER_2        rsi_spiTimer2++
#define RSI_INC_TIMER_1        rsi_spiTimer1++
#define RSI_INC_TIMER_3        rsi_spiTimer3++
#define RSI_RESET_TIMER1       rsi_spiTimer1=0
#define RSI_RESET_TIMER2       rsi_spiTimer2=0
#define RSI_RESET_TIMER3       rsi_spiTimer3=0
#else
#define RSI_TICKS_PER_SECOND   1800000000
#define RSI_INC_TIMER_3        rsi_spiTimer3
#define RSI_INC_TIMER_2        rsi_spiTimer2
#define RSI_INC_TIMER_1        rsi_spiTimer1
#define RSI_RESET_TIMER1       rsi_spiTimer1=0
#define RSI_RESET_TIMER2       rsi_spiTimer2=0
#define RSI_RESET_TIMER3       rsi_spiTimer3=0
#endif

/*@ firmware upgradation timeout */
#define RSI_FWUPTIMEOUT       100 * RSI_TICKS_PER_SECOND
/*@ bootloading timeout */
#define RSI_BLTIMEOUT         1 * RSI_TICKS_PER_SECOND
/*@ band timeout */
#define RSI_BANDTIMEOUT       1 * RSI_TICKS_PER_SECOND
/*@ Init timeout */
#define RSI_INITTIMEOUT       1 * RSI_TICKS_PER_SECOND
/*@ Query firmware version timeout */
#define RSI_QFWVTIMEOUT       1 * RSI_TICKS_PER_SECOND
/*@ Set Mac address timeout */
#define RSI_SMATIMEOUT        1 * RSI_TICKS_PER_SECOND
/*@ Scan timeout */
#define RSI_SCANTIMEOUT         12 * RSI_TICKS_PER_SECOND
/*@ Join timeout */
#define RSI_JOINTIMEOUT         12 * RSI_TICKS_PER_SECOND
/*@ Disconnect timeout */
#define RSI_DISCONTIMEOUT     1 * RSI_TICKS_PER_SECOND
/*@ Query connection status timeout */
#define RSI_QCSTIMEOUT        3 * RSI_TICKS_PER_SECOND
/*@ Query dhcp params timeout */
#define RSI_QDPTIMEOUT        3 * RSI_TICKS_PER_SECOND
/*@ Query network params timeout */
#define RSI_QNPTIMEOUT        3 * RSI_TICKS_PER_SECOND
/*@ Ip configuration timeout */
#define RSI_IPPTIMEOUT        6 * RSI_TICKS_PER_SECOND
/*@ Query RSSI Value timeout */
#define RSI_RSSITIMEOUT       1 * RSI_TICKS_PER_SECOND
/*@ recv timeout */
#define RSI_RECVTIMEOUT       1 * RSI_TICKS_PER_SECOND
/*@ Socket open timeout */
#define RSI_SOPTIMEOUT    6 * RSI_TICKS_PER_SECOND
/*@ Regread timeout */
#define RSI_REGREADTIMEOUT    1 * RSI_TICKS_PER_SECOND
/*@ Query DNS timeout */
#define RSI_QDNSTIMEOUT       6 * RSI_TICKS_PER_SECOND
/*@ Start token timeout */
#define RSI_START_TOKEN_TIMEOUT 10 * RSI_TICKS_PER_SECOND
/*@ Set Listen interval timeout */
#define RSI_SLITIMEOUT          1 * RSI_TICKS_PER_SECOND
/*@ Config Enable timeout */
#define RSI_CETIMEOUT     1 * RSI_TICKS_PER_SECOND
/*@ Config store timeout */
#define RSI_CSTIMEOUT     1 * RSI_TICKS_PER_SECOND
/*@ Config get timeout */
#define RSI_CGTIMEOUT     1 * RSI_TICKS_PER_SECOND
/*@ Query BSSID/NW TYPE timeout */
#define RSI_QBSSIDNWTIMEOUT     6 * RSI_TICKS_PER_SECOND
#define RSI_QMACADDTIMEOUT      6 * RSI_TICKS_PER_SECOND
/*@ Query MAC ADDR timeout */
/*@ Get HTTP timeout */
#define RSI_GETHTTPTIMEOUT      6 * RSI_TICKS_PER_SECOND
/*@ Post HTTP timeout */
#define RSI_POSTHTTPTIMEOUT      6 * RSI_TICKS_PER_SECOND
/*@ Mode select timeout */
#define RSI_MODESEL_TIMEOUT      1 * RSI_TICKS_PER_SECOND
/*@ Feature select timeout */
#define RSI_FEATSEL_TIMEOUT      1 * RSI_TICKS_PER_SECOND

/** Command response timeout */
#define RSI_RESPONSE_TIMEOUT(A)    		RSI_RESET_TIMER3; \
	                                        while (rsi_checkPktIrq() != RSI_TRUE) \
                                                {                                  \
	                                        if (RSI_INC_TIMER_3 > A)  \
	                                           {							   \
		                                     retval = -1;				   \
		                                     break;					   \
	                                           }                               \
                                                }     



/*=======================================================================================*/
/**
 * Device Parameters
 */
#define RSI_MAXSOCKETS			        8		/// maximum number of open sockets

/**
 * Debugging Parameters
 */
#define RSI_DEBUG_DEVICE		        "UART_1"
#define RSI_MAX_PAYLOAD_SIZE			1400	       /// maximum data payload size
#define RSI_AP_SCANNED_MAX		        15	       /// maximum number of scanned acces points

/**
 * Things that are needed in this .h file
 */
#define RSI_FRAME_DESC_LEN		        16									
/// Length of the frame descriptor, for both read and write
#define RSI_FRAME_CMD_RSP_LEN			56									
/// Length of the command response buffer/frame
#define RSI_TXDATA_OFFSET_TCP			44									
/// required Tx data offset value for TCP, 44
#define RSI_TXDATA_OFFSET_UDP			32									
/// required Tx data offset value for UDP, 32
#define RSI_TXDATA_OFFSET_LUDP          26
/// required Tx data offset value for UDP, 26
#define RSI_RXDATA_OFFSET_TCP			38									
/// required Rx data offset value for TCP, 38
#define RSI_RXDATA_OFFSET_UDP			26									
/// required Rx data offset value for UDP, 26

#if RSI_63BYTE_PSK_SUPPORT
#define RSI_PSK_LEN			        63			 /// maximum length of PSK
#else
#define RSI_PSK_LEN			        32			 /// maximum length of PSK
#endif
#define RSI_PSK_64_BYTE_LEN         64
#define RSI_SSID_LEN			        32	     /// maximum length of SSID
#define RSI_IP_ADD_LEN                          4
#define RSI_MAC_ADD_LEN                         6
#define RSI_MGMT_PKT_TYPE                       0x04
#define RSI_DATA_PKT_TYPE                       0x02 
#define RSI_MAX_DOMAIN_NAME_LEN 		42
#define RSI_MAX_DNS_REPLY 			10




/**
 * Const declaration
 *
 */
#define RSI_BYTES_3                             3
#define RSI_BYTES_2                             2

extern const uint8			rsi_frameCmdBand[RSI_BYTES_3];			
/// Band Code=0x18, Data Frame bytes to transfer=0x04, Frame Type, 0x02=Data, 0x04=Management
extern const uint8			rsi_frameCmdInit[RSI_BYTES_3];			
/// Init Code=0x10, Data Frame bytes to transfer=0x04, Frame Type, 0x02=Data, 0x04=Management
extern const uint8			rsi_frameCmdScan[RSI_BYTES_3];				
/// 36 bytes, Scan Code=0x11, Data Frame bytes to transfer=0x25, Frame Type, 0x02=Data, 0x04=Management
extern const uint8			rsi_frameCmdJoin[RSI_BYTES_3];				
/// 72 bytes, Join Code=0x12, Data Frame bytes to transfer=0x47, Frame Type, 0x02=Data, 0x04=Management
extern const uint8			rsi_frameCmdPower[RSI_BYTES_3];				
/// Power Code=0x19, Data Frame bytes to transfer=0x04, Frame Type, 0x02=Data, 0x04=Management
extern const uint8			rsi_frameCmdFftaim1Upgd[RSI_BYTES_3];				
/// fftaim1 upgrade Code=0x13, Data Frame bytes to transfer=??, Frame Type, 0x02=Data, 0x04=Management
extern const uint8			rsi_frameCmdFftaim2Upgd[RSI_BYTES_3];				
/// fftaim1 upgrade Code=0x14, Data Frame bytes to transfer=??, Frame Type, 0x02=Data, 0x04=Management
extern const uint8			rsi_frameCmdFftadmUpgd[RSI_BYTES_3];				
/// fftaim1 upgrade Code=0x15, Data Frame bytes to transfer=??, Frame Type, 0x02=Data, 0x04=Management
extern const uint8          rsi_frameCmdBssidNwtype[RSI_BYTES_3];               
/// bssid nwtype get Code=0x23, Data Frame bytes to transfer=??, Frame Type, 0x02=Data, 0x04=Management
extern const uint8			rsi_frameCmdCfgEnable[RSI_BYTES_3];			// CfgEnable Code=0x2A, Data Frame bytes to transfer=0x04, Frame Type, 0x02=Data, 0x04=Management
extern const uint8			rsi_frameCmdCfgSave[RSI_BYTES_3];			// CfgSave Code=0x2B, Data Frame bytes to transfer=??, Frame Type, 0x02=Data, 0x04=Management
extern const uint8			rsi_frameCmdCfgGet[RSI_BYTES_3];			// CfgGet Code=0x2C, Data Frame bytes to transfer=??, Frame Type, 0x02=Data, 0x04=Management
extern const uint8          rsi_frameCmdModeSel[RSI_BYTES_3];
/// ModeSel Code=0x2D, Data Frame bytes to transfer=4 [3 padding], Frame Type, 0x02=Data, 0x04=Management
extern const uint8          rsi_frameCmdFeatSel[RSI_BYTES_3];
/// FeatSel Code=0x24, Data Frame bytes to transfer=4, Frame Type, 0x02=Data, 0x04=Management

/// Management Data Frames
extern const uint8			rsi_dFrmBand[RSI_BYTES_2];
extern const uint8			rsi_dFrmInit[RSI_BYTES_2];						
/// PRM says there isn't one, but without it, it hangs
extern const uint8			rsi_dFrmPower[RSI_BYTES_2];	// uint16 + padding
extern const uint8          rsi_dFrmCfgEnable[RSI_BYTES_2];
extern const uint8         rsi_dFrmCfgSave[RSI_BYTES_2];
/// Data-Control Commands
extern const uint8			rsi_frameCmdIpparam[RSI_BYTES_3];				
/// 16 bytes, Byte0=bits[7:0], byte1=bits[11:8] of data frame length, byte2=0x02, Data frame
extern const uint8			rsi_frameCmdSocket[RSI_BYTES_3];				
/// 12 bytes, Byte0=bits[7:0], byte1=bits[11:8] of data frame length, byte2=0x02, Data frame
extern const uint8			rsi_frameCmdSocketClose[RSI_BYTES_3];				
/// 4 bytes, 2 bytes command, 2 bytes socket descriptor
extern const uint8			rsi_frameCmdRssi[RSI_BYTES_3];				
/// 4 bytes, 2 bytes command, 2 bytes padding
extern const uint8			rsi_frameCmdSend[RSI_BYTES_3];  /// Length is variable
extern const uint8			rsi_frameCmdRecv[RSI_BYTES_3];  /// Length comes from the received frame descriptor
extern const uint8			rsi_frameCmdConnStatus[RSI_BYTES_3];				
/// 4 bytes, 2 bytes command, 2 bytes padding
extern const uint8			rsi_frameCmdQryNetParms[RSI_BYTES_3];				
/// 4 bytes, 2 bytes command, 2 bytes padding
extern const uint8			rsi_frameCmdDisconnect[RSI_BYTES_3];				
/// 4 bytes, 2 bytes command, 2 bytes padding
extern const uint8			rsi_frameCmdQryDhcpParms[RSI_BYTES_3];				
/// 4 bytes, 2 bytes command, 2 bytes padding
extern const uint8			rsi_frameCmdQryFwVer[RSI_BYTES_3];				
/// 4 bytes, 2 bytes command, 2 bytes padding
extern const uint8			rsi_frameCmdSetMacAddr[RSI_BYTES_3];				
/// 8 bytes, 6 bytes MAC Address, 2 bytes padding
extern const uint8			rsi_frameCmdDnsGet[RSI_BYTES_3];					
/// 44 bytes, 42 bytes domain name and 2 bytes padding
extern const uint8			rsi_frameCmdSetListenInterval[RSI_BYTES_3];				
/// 4 bytes, 2 bytes listen Address, 2 bytes command
extern const uint8         rsi_frameCmdReqHttp[RSI_BYTES_3];
/// 1212 bytes, 2 bytes command, 2 bytes padding, 1208 bytes command data
extern const uint8			rsi_frameCmdQryMacAddress[RSI_BYTES_3];
/// Data-Control Data Frames
/// These can be used as-is, or copied into bytes 0/1 of a larger data frame
extern const uint8			rsi_dFrmIpparam[RSI_BYTES_2];
extern const uint8			rsi_dFrmSocket[RSI_BYTES_2];
extern const uint8			rsi_dFrmSend[RSI_BYTES_2];
extern const uint8			rsi_dFrmSocketClose[RSI_BYTES_2];
extern const uint8			rsi_dFrmRssi[RSI_BYTES_2];
extern const uint8			rsi_dFrmQryNetParms[RSI_BYTES_2];
extern const uint8			rsi_dFrmConnStatus[RSI_BYTES_2];
extern const uint8			rsi_dFrmDisconnect[RSI_BYTES_2];
extern const uint8			rsi_dFrmQryDhcpParms[RSI_BYTES_2];
extern const uint8			rsi_dFrmQryFwVer[RSI_BYTES_2];
extern const uint8			rsi_dFrmSetMacAddr[RSI_BYTES_2];
extern const uint8			rsi_dFrmDnsGet[RSI_BYTES_2];
extern const uint8			rsi_dFrmSetListenInterval[RSI_BYTES_2];
extern const uint8			rsi_dFrmGetHttp[RSI_BYTES_2];
extern const uint8			rsi_dFrmQryMacAddress[RSI_BYTES_2];

/// Management Response list
extern const uint8			rsi_mgmtRspList[];   /// 9 bytes, list of management frame response codes


/*===============================================*/
/**
 * Scan Structures
 */


/// The scan command argument union/variables

typedef union {
	struct {
		uint8				channel[4];			/// RF channel to scan, 0=All, 1-14 for 2.5GHz channels 1-14
		uint8				ssid[RSI_SSID_LEN];		/// uint8[32], ssid to scan for
	} scanFrameSnd;
	uint8 				uScanBuf[RSI_SSID_LEN + 4];		/// byte format to send to the spi interface, 36 bytes
} rsi_uScan;



/*===============================================*/
/**
 * Join Data Frame Structure
 */
typedef union {
	struct {
		uint8				nwType;				/// network type, 0=Ad-Hoc (IBSS), 1=Infrastructure
		uint8				securityType;			/// security type, 0=Open, 1=WPA1, 2=WPA2, 3=WEP
		uint8				dataRate;			/// data rate, 0=auto, 1=1Mbps, 2=2Mbps, 3=5.5Mbps, 4=11Mbps, 12=54Mbps
		uint8				powerLevel;			/// xmit power level, 0=low (6-9dBm), 1=medium (10-14dBm, 2=high (15-17dBm)
		uint8				psk[RSI_PSK_LEN];		/// pre-shared key, 32-byte string
		uint8				ssid[RSI_SSID_LEN];	        /// ssid of access point to join to, 32-byte string
		uint8				ibssMode;			/// Ad-Hoc Mode (IBSS), 0=Joiner, 1=Creator
		uint8				ibssChannel;			/// rf channel number for Ad-Hoc (IBSS) mode
#if RSI_63BYTE_PSK_SUPPORT
		uint8				padding[3];			/// data length sent should always be in multiples of 4 bytes
#else
		uint8				padding[2];			/// data length sent should always be in multiples of 4 bytes
#endif
	} joinFrameSnd;
#if RSI_63BYTE_PSK_SUPPORT
	uint8					uJoinBuf[RSI_SSID_LEN + RSI_PSK_LEN + 9];			
	/// byte format to send to the spi interface, 104 (0x68) bytes
#else
	uint8					uJoinBuf[RSI_SSID_LEN + RSI_PSK_LEN + 8];			
	/// byte format to send to the spi interface, 72 (0x48) bytes
#endif
} rsi_uJoin;

/*===============================================*/
// Store configuration 
/**
 * store config enable command
 */
// Command

typedef union {
  struct {						        
    uint8				uFlagval;									/// 0=Diasable, 1=Enable
	uint8				padding[3];
  } CfgEnableFrameSnd;
  uint8					uCfgEnableBuf[4];							/// 16 bytes, byte format to send to spi
} rsi_uCfgEnable;



/*===============================================*/
// Control Frames
/**
 * TCP/IP Configure
 */
// Command

typedef union {
	struct {
		uint8				ipparamCmd[2];			/// int16, TCPIP/NETWORK command code, 0x0001
		uint8				dhcpMode;		        /// 0=Manual, 1=Use DHCP
		uint8				ipaddr[4];			/// IP address of this module if in manual mode
		uint8				netmask[4];		        /// Netmask used if in manual mode
		uint8				gateway[4];			/// IP address of default gateway if in manual mode
		uint8                           dnsip[4];
		uint8				padding;
	} ipparamFrameSnd;
	uint8					uIpparamBuf[20];		/// 16 bytes, byte format to send to spi
} rsi_uIpparam;


/*===============================================*/
/**
 * Dns get request
 */
// Command

typedef union {
	struct {
		uint8				DnsGetCmd[2];                          /// uint16 dns get command 0x0011
		uint8 				DomainName[RSI_MAX_DOMAIN_NAME_LEN];   /// 42 bytes, Domain name like www.google.com 
	} dnsFrameSnd;
	uint8					uDnsBuf[44];			       /// 16 bytes, byte format to send to spi
} rsi_uDns;


/*===============================================*/
/**
 * Http get request
 */
// Command

typedef union {
    struct {
		uint8				HttpGetCmd[2];
        uint8               ipaddr_len[2];
        uint8               url_len[2];
        uint8               header_len[2];
        uint8               data_len[2];
        uint8               buffer[1200];
    } HttpReqFrameSnd;
    uint8                   uHttpReqBuf[1208];
} rsi_uHttpReq;


/*===================================================*/
/**
 * Socket Configure
 */
// Commad 
typedef union {
	struct {
		uint8				socketCmd[2];			       /// uint16, socket command code, 0x0002
		uint8				socketType[2];			       /// 0=TCP Client, 1=UDP Client, 2=TCP Server (Listening TCP)
		uint8				moduleSocket[2];		       /// Our local module port number
		uint8				destSocket[2];			       /// Port number of what we are connecting to
		uint8				destIpaddr[4];			       /// IP address of socket we are connecting to
	} socketFrameSnd;
	uint8					uSocketBuf[12];			       /// 12 bytes, byte format to send to spi
} rsi_uSocket;





/*===================================================*/
/**
 * Sockets Structure
 * Structure linking socket number to protocol
 */
typedef struct {
	uint8					socketDescriptor[2];			/// socket descriptor
	uint8					protocol;				/// PROTOCOL_UDP, PROTOCOL_TCP or PROTOCOL_UNDEFINED
	uint8         		                src_port[2];                            /// socket local port number
	uint8         		                dest_ip[4];                             /// Destination ipaddress
	uint8         		                dest_port[2];                           /// Destination port number  
} rsi_socketsStr;



/*===================================================*/
/**
 * Sockets Structure Array
 * Array of Structures linking socket number to protocol
 */
typedef struct {
	rsi_socketsStr				socketsArray[RSI_MAXSOCKETS+1];		/// Socket numbers are from 1 to 8
} rsi_sockets;


/*===================================================*/
/**
 * Socket Close
 */

typedef union {
	struct {
		uint8				socketCloseCmd[2];			/// uint16, command code
		uint8				socketDescriptor[2];		        /// uint16, socket descriptor to close
	} socketCloseFrameSnd;
	uint8 				uSocketCloseBuf[4];			        /// byte format to send to the spi interface, 8 bytes
} rsi_uSocketClose;



/*===================================================*/
/**
 * Set Mac Address
 */

typedef union {
	struct {
		uint8				setMacAddrCmd[2];		        /// uint16, command code
		uint8				macAddr[6];				/// byte array, mac address
	} setMacAddrFrameSnd;
	uint8 				setMacAddrBuf[8];			        /// byte format to send to the spi interface, 8 bytes

} rsi_uSetMacAddr;


/*===================================================*/
/**
 * Set Listen interval
 */
typedef union {
	struct {
		uint8				setListenIntervalCmd[2];		/// uint16, command code
		uint8				interval[2];				/// uint16, listen inteval
	} setListenIntervalFrameSnd;
	uint8 				setListenIntervalBuf[4];			/// byte format to send to the spi interface, 4 bytes
} rsi_uSetListenInterval;


/*===================================================*/
/**
 * Band
 */

typedef union {
	struct {
		uint8				bandVal;				/// uint8, band value to set
		uint8				padding[3];				/// needs to be 4 bytes long
	} bandFrameSnd;
	uint8 				uBandBuf[4];				        /// byte format to send to the spi interface, 4 bytes
} rsi_uBand;

/*===================================================*/
/**
 * Mode Select TCP/NO TCP
 */

typedef union {
	struct {
		uint8				ModeSelVal;				/// uint8, mode value to set
		uint8				padding[3];				/// needs to be 4 bytes long
	} ModeSelFrameSnd;
	uint8 				uModeSelBuf[4];			    /// byte format to send to the spi interface, 4 bytes
} rsi_uModeSel;

/*===================================================*/
/**
 * feature Select
 */

typedef union {
	struct {
		uint8				FeatSelBitmap[4];				/// uint8, mode value to set
	} FeatSelFrameSnd;
	uint8 				uFeatSelBuf[4];			    /// byte format to send to the spi interface, 4 bytes
} rsi_uFeatSel;


/*===================================================*/
/**
 * Power
 */

typedef union {
	struct {
		uint8				powerVal[2];				/// uint16, power value to set
	} powerFrameSnd;
	uint8 				uPowerBuf[2];					/// byte format to send to the spi interface, 4 bytes
} rsi_uPower;



/*===================================================*/
/**
 * SEND
 */
typedef union {
	struct {
		uint8				sendCmd[2];				/// send command code, 0x0003
		uint8				socketDescriptor[2];		        /// socket descriptor of the already opened socket connection
		uint8				sendBufLen[4];				/// length of the data to be sent
		uint8				sendDataOffsetSize[2];			/// Data Offset, TCP=44, UDP=32
		uint8				padding[RSI_TXDATA_OFFSET_TCP + RSI_MAX_PAYLOAD_SIZE + 2];	/// large enough for TCP or UDP frames
	} sendFrameSnd;
	struct {
		uint8				sendCmd[2];				/// send command code, 0x0003
		uint8				socketDescriptor[2];			/// socket descriptor of the already opened socket connection
		uint8				sendBufLen[4];				/// length of the data to be sent
		uint8				sendDataOffsetSize[2];			/// Data Offset, TCP=44, UDP=32
		uint8				sendDataOffsetBuf[RSI_TXDATA_OFFSET_UDP];	
		/// Empty offset buffer, UDP=32
		uint8				sendDataBuf[RSI_MAX_PAYLOAD_SIZE];			
		/// Data payload buffer, 1400 bytes max
		uint8				padding[2];								
		/// data length sent should always be in multiples of 4 bytes
	} sendFrameUdpSnd;
	struct {
		uint8				sendCmd[2];				/// send command code, 0x03
		uint8				socketDescriptor[2];			/// socket descriptor of the already opened socket connection
		uint8				sendBufLen[4];				/// length of the data to be sent
		uint8				sendDataOffsetSize[2];			/// Data Offset, 44 for TCP, 32 for UDP
		uint8				sendDataOffsetBuf[RSI_TXDATA_OFFSET_TCP];	
		/// Empty offset buffer, 44 for TCP
		uint8				sendDataBuf[RSI_MAX_PAYLOAD_SIZE];	/// Data payload buffer, 1400 bytes max
		uint8				padding[2];				/// data length sent should always be in multiples of 4 bytes
	} sendFrameTcpSnd;
		struct {
		uint8				sendCmd[2];				/// send command code, 0x0003
		uint8				socketDescriptor[2];			/// socket descriptor of the already opened socket connection
		uint8				sendBufLen[4];				/// length of the data to be sent
		uint8				sendDataOffsetSize[2];			/// Data Offset, TCP=44, UDP=32
		uint8               destPort[2];
		uint8               destIp[RSI_IP_ADD_LEN];
		uint8				sendDataOffsetBuf[RSI_TXDATA_OFFSET_LUDP];	
		/// Empty offset buffer, UDP=26
		uint8				sendDataBuf[RSI_MAX_PAYLOAD_SIZE];			
		/// Data payload buffer, 1400 bytes max
		uint8				padding[2];								
		/// data length sent should always be in multiples of 4 bytes
	} sendFrameLudpSnd;
	uint8					uSendBuf[RSI_MAX_PAYLOAD_SIZE + RSI_TXDATA_OFFSET_TCP + 12];		
	/// byte format to send to spi, TCP is the larger of the two, 1456 bytes
} rsi_uSend;


/*===================================================*/
/**
 * Frame Descriptor
 */
typedef union {
	struct {
		uint8				mgmtFrmBodyLenShort;		        /// If < 0xff, length of the management frame body
		uint8				mgmtPacketType;				
		/// Management packet type, Band, Init, Scan, Join, Power, fftaim1, fftaim2, fftadm
		uint8				padding1[6];			        /// Unused, set to 0x00
		uint8				mgmtFrmBodyLenLong[2];					
		/// If managementFrameBodyLenShort=0xff, these two bytes are the management frame body length
		uint8				padding2[4];				/// Unused, set to 0x00
		uint8				frameType[2];							
		/// 0x0004=Management Frame, 0x0002=Data Frame
	} frameDscMgmtSnd;
	struct {
		uint8				dataFrmBodyLen[2];						
		/// Data frame body length. Bits 15:12=0, Bits 11:0 are the length
		uint8				padding[12];				/// Unused, set to 0x00
		uint8				frameType[2];							
		/// 0x0004=Management Frame, 0x0002=Data Frame
	} frameDscDataSnd;
	struct {
		uint8				mgmtFrmBodyLenShort;			/// If < 0xff, length of the management frame body
		uint8				mgmtPacketType;							
		/// Management packet type, Band, Init, Scan, Join, Power, fftaim1, fftaim2, fftadm
		uint8				padding1;								
		/// Management frame descriptor response returns 0x00
		uint8				mgmtFrmDscRspStatus;					
		/// Management frame descriptor response status, 0x00=success, else error
		uint8				padding2[4];				/// Unknown
		uint8				mgmtFrmBodyLenLong[2];					
		/// If managementFrameBodyLenShort=0xff, these two bytes are the management frame body length
		uint8				padding3[4];				/// Uunknown
		uint8				frameType[2];				/// 0x0004=Management Frame, 0x0002=Data Frame
	} frameDscMgmtRsp;
	uint8					uFrmDscBuf[RSI_FRAME_DESC_LEN];		/// byte format for spi interface, 16 bytes
} rsi_uFrameDsc;



/*===================================================*/
/**
 * Command Response Frame Union
 */
typedef struct {
	uint8					rfChannel;				/// rf channel to us, 0=scan for all
	uint8					securityMode;				/// security mode, 0=open, 1=wpa1, 2=wpa2, 3=wep
	uint8					rssiVal;				/// absolute value of RSSI
	uint8					ssid[RSI_SSID_LEN];			/// 32-byte ssid of scanned access point
} rsi_scanInfo;

typedef struct {
    uint8				    nwType;										/// network type, 0=Ad-Hoc (IBSS), 1=Infrastructure
    uint8				    securityType;							/// security type, 0=Open, 1=WPA1, 2=WPA2, 3=WEP
    uint8				    dataRate;								/// data rate, 0=auto, 1=1Mbps, 2=2Mbps, 3=5.5Mbps, 4=11Mbps, 12=54Mbps
    uint8				    powerLevel;							/// xmit power level, 0=low (6-9dBm), 1=medium (10-14dBm, 2=high (15-17dBm)    
	uint8				    psk[RSI_PSK_64_BYTE_LEN];								/// pre-shared key, 64-byte string
    uint8				    ssid[RSI_SSID_LEN];								/// ssid of access point to join to, 32-byte string
    uint8				    ibssMode;									/// Ad-Hoc Mode (IBSS), 0=Joiner, 1=Creator
    uint8				    ibssChannel;								/// rf channel number for Ad-Hoc (IBSS) mode
    uint8				    reserved;
} rsi_joinInfo;

typedef struct {
	uint8					rfChannel;				/// rf channel to us, 0=scan for all
	uint8					securityMode;	        		/// security mode, 0=open, 1=wpa1, 2=wpa2, 3=wep
	uint8					rssiVal;	        		/// absolute value of RSSI
	uint8					ssid[RSI_SSID_LEN];			/// 32-byte ssid of scanned access point
	uint8                   uNetworkType;  
	uint8                   BSSID[6];
} rsi_bssid_nwtypeInfo;

typedef struct {
	uint8                   rspCode[4]; 
	uint8				    scanCount[4];				/// uint32, number of access points found
	rsi_scanInfo		    strScanInfo[RSI_AP_SCANNED_MAX];	/// 32 maximum responses from scan command
	uint8                   status;
} rsi_scanResponse;

typedef struct {
	uint8                   rspCode[2]; 
	uint8                   status;  
} rsi_mgmtResponse;   

typedef struct {
	uint8                       rspCode[2]; 
	uint8				        rssiVal[2];				
	/// uint16, RSSI value for the device the module is currently connected to
	uint8				        errCode[4];				/// uint32, 0=Success, !=0, Failure
} rsi_rssiFrameRcv;

typedef struct {
	uint8                       rspCode[2]; 
	uint8				        socketType[2];				/// uint16, type of socket created
	uint8				        socketDescriptor[2];			/// uinr16, socket descriptor, like a file handle, usually 0x00
	uint8				        moduleSocket[2];			/// uint16, Port number of our local socket
	uint8				        moduleIpaddr[4];			/// uint32, Our (module) IP Address
	uint8				        errCode[4];				/// uint32, 0=Success, 1=Error
} rsi_socketFrameRcv;

typedef struct {
	uint8                       rspCode[2]; 
	uint8				        socketDsc[2];				/// uint16, socket that was closed
	uint8				        errorCode[4];				/// uint32, error code, 0=success, non zero=Failure
} rsi_socketCloseFrameRcv;

typedef struct {
	uint8                       rspCode[2]; 
	uint8				        macAddr[6];				/// MAC address of this module
	uint8				        ipaddr[4];				/// Configured IP address
	uint8				        netmask[4];				/// Configured netmask
	uint8				        gateway[4];				/// Configured default gateway
	uint8				        errCode[4];				/// 0=Success, 1=Error
} rsi_ipparamFrameRcv;

typedef struct {
	uint8                   rspCode[2]; 
	uint8				    state[2];				/// uint16, connection state, 0=Not Connected, 1=Connected
	uint8				    errorCode[4];				/// uint32, error code, 0=Success, not zero = Failure
} rsi_conStatusFrameRcv;

typedef struct {
	uint8                   rspCode[2]; 
	uint8					leaseTime[4];				/// uint32, total lease time for dhcp connection
	uint8					leaseTimeLeft[4];			/// uint32, lease time left for dhcp connection
	uint8					renewTime[4];				/// uint32, time left for renewal of IP address
	uint8					rebindTime[4];				/// uint32, time left for rebind of IP address
	uint8					serverIpAddr[4];			/// uint8[4], IP address of DHCP server
	uint8					errorCode[4];				/// uint32, Error Code, 0=Success, Non Zero=Failure
} rsi_qryDhcpInfoFrameRcv;

typedef struct {
	uint8                   rspCode[2]; 
	uint8					wlanState[2];				/// uint16, 0=NOT Connected, 1=Connected
	uint8					ssid[32];				/// uint8[32], SSID of connected access point
	uint8					ipaddr[4];				/// uint8[4], Module IP Address
	uint8					subnetMask[4];				/// uint8[4], Module Subnet Mask
	uint8					gateway[4];				/// uint8[4], Gateway address for the Module
	uint8					dhcpMode[2];				/// uint16, 0=Manual IP Configuration,1=DHCP
	uint8					connType[2];				/// uint16, 0=AdHoc, 1=Infrastructure
	uint8					errorCode[4];				/// uint32, Error Code, 0=Success, Non Zero=Failure
} rsi_qryNetParmsFrameRcv;

typedef struct {
	uint8                   rspCode[2]; 
	uint8					fwversion[20];				/// uint8[20], firmware version text string, x.y.z as 1.3.0
} rsi_qryFwversionFrameRcv;

typedef struct {
	uint8                   rspCode[2]; 
	uint8					MacAddress[6];				/// uint8[6], MAC address hex 6 bytes
	uint8					errorCode[4];				/// uint32, error code, 0=Success, non zero=Failure
} rsi_qryMacFrameRcv;


typedef struct {
	uint8                   rspCode[2]; 
	uint8					errorCode[4];				/// uint32, error code, 0=Success, non zero=Failure
} rsi_disconnectFrameRcv;

typedef struct {
	uint8                   rspCode[2]; 
	uint8					errorCode[4];				/// uint32, error code, 0=Success, non-zero=Fail
	uint8					padding[2];				/// need to be on a 4-byte boundary
} rsi_setMacAddrFrameRcv;

typedef struct {
	uint8                   rspCode[2]; 
	uint8					recvSocket[2];				/// uint16, the socket number associated with this read event
	uint8					recvBufLen[4];				/// uint32, length of data received
	uint8					recvDataOffsetSize[2];			/// uint16, offset of data from start of buffer
	uint8					fromPortNum[2];				/// uint16, port number of the device sending the data to us
	uint8					fromIpaddr[4];				/// uint32, IP Address of the device sending the data to us
	uint8					recvDataOffsetBuf[RSI_RXDATA_OFFSET_UDP];
	/// uint8, empty offset buffer, 26 for UDP, 42 bytes from beginning of buffer
	uint8					recvDataBuf[RSI_MAX_PAYLOAD_SIZE];	/// uint8, buffer with received data
	uint8					padding[2];				/// data length received should always be in multiples of 4 bytes
} rsi_recvFrameUdp;

typedef struct {
	uint8                   rspCode[2]; 
	uint8					recvSocket[2];				/// uint16, the socket number associated with this read event
	uint8					recvBufLen[4];				/// uint32, length of payload data received
	uint8					recvDataOffsetSize[2];			/// uint16, offset of data from start of buffer
	uint8					fromPortNum[2];				/// uint16, port number of the device sending the data to us
	uint8					fromIpaddr[4];				/// uint32, IP Address of the device sending the data to us
	uint8					recvDataOffsetBuf[RSI_RXDATA_OFFSET_TCP];	
	/// uint8, empty offset buffer, 38 for TCP
	uint8					recvDataBuf[RSI_MAX_PAYLOAD_SIZE];	/// uint8, buffer with received data
	uint8					padding[2];				/// data length received should always be in multiples of 4 bytes
} rsi_recvFrameTcp;

typedef struct {
	uint8                   rspCode[2]; 
	uint8					socket[2];				/// uint16, socket handle for the terminated connection
	uint8					errCode[4];				/// uint32, error code, 0=Success, non-zero=Fail
} rsi_recvRemTerm;

typedef struct {
	uint8                   rspCode[2]; 
	uint8					socket[2];				/// uint16, socket handle 
	uint8					fromPortNum[2];				/// uint16, remote port number 
	uint8					fromIpaddr[4];				/// uint32, remote IP Address 
	uint8					errCode[4];				/// uint32, error code, 0=Success no failure case.
} rsi_recvLtcpEst;

typedef struct { 
	uint8                           rspCode[2]; 
	uint8               			uipcount[2];  
	uint8               			ipaddr[RSI_MAX_DNS_REPLY][4];
	uint8               			errCode[4]; 
} rsi_dnsFrameRcv;

typedef struct {
	uint8                			rspCode[4]; 
	uint8	             			scanCount[4];	                        /// uint32, number of access points found
	rsi_bssid_nwtypeInfo            strBssid_NwtypeInfo[RSI_AP_SCANNED_MAX];
	/// 32 maximum responses from scan command
	uint8                			status;
} rsi_bssid_nwtypeFrameRecv;

typedef struct {
    uint8				 rspCode[2];								/// uint16, 0=Success, 1=Flash access Failure, 2=Not Connected
    uint8        errCode[2];
} rsi_CfgEnableFrameRcv;

typedef struct {
    uint8				 rspCode[2];								/// uint16, 0=Success, 1=Flash access Failure, 2=Not Connected
    uint8        errCode[2];
} rsi_CfgSaveFrameRcv;

typedef struct { 
	uint8				         rspCode[2];
	uint8                channelnum;                      /* Channel number */ 
	rsi_joinInfo         fc_cfg_join; 
	uint8                dhcp_enable;                     /* 0- DHCP disable, 1- DHCP enable */ 
	uint8                ip[RSI_IP_ADD_LEN];               /* Module IP address */ 
	uint8                sn_mask[RSI_IP_ADD_LEN];          /* Sub-net mask */ 
	uint8                dgw[RSI_IP_ADD_LEN];              /* Default gateway */
	uint8                feature_sel[4];
	uint8                valid_flag[2];                   /* Is this configuration valid or not ! */ 
	uint8                status;
} rsi_CfgGetFrameRcv; 

typedef struct {
     uint8                more[4];
     uint8                offset[4];
     int8                 nErrorCode[4];
} rsi_HttpGetFrameRcv; 

typedef union {
	uint8                     		rspCode[2];                    		/// command code response
	rsi_scanResponse          		scanResponse;
	rsi_mgmtResponse          		mgmtResponse;
	rsi_rssiFrameRcv          		rssiFrameRcv;
	rsi_socketFrameRcv        		socketFrameRcv;
	rsi_socketCloseFrameRcv   		socketCloseFrameRcv;
	rsi_ipparamFrameRcv       		ipparamFrameRcv;
	rsi_conStatusFrameRcv     		conStatusFrameRcv;
	rsi_qryDhcpInfoFrameRcv   		qryDhcpInfoFrameRcv;
	rsi_qryNetParmsFrameRcv   		qryNetParmsFrameRcv;
	rsi_qryFwversionFrameRcv  		qryFwversionFrameRcv;
	rsi_disconnectFrameRcv    		disconnectFrameRcv;
	rsi_setMacAddrFrameRcv    	 	setMacAddrFrameRcv;
	rsi_recvFrameUdp          		recvFrameUdp;
	rsi_recvFrameTcp          		recvFrameTcp;  
	rsi_recvRemTerm           		recvRemTerm;
	rsi_recvLtcpEst           		recvLtcpEst;
	rsi_dnsFrameRcv           		dnsFrameRcv;
	rsi_bssid_nwtypeFrameRecv 		bssid_nwtypeFrameRecv;
	rsi_CfgEnableFrameRcv           CfgEnableFrameRcv;
	rsi_CfgSaveFrameRcv             CfgSaveFrameRcv;
	rsi_CfgGetFrameRcv              CfgGetFrameRcv;
	rsi_HttpGetFrameRcv             HttpGetFrameRcv;
	rsi_qryMacFrameRcv              qryMacFrameRcv;
	uint8					        uCmdRspBuf[RSI_FRAME_CMD_RSP_LEN + RSI_MAX_PAYLOAD_SIZE];
} rsi_uCmdRsp;




/*===================================================*/
/**
 * Interrupt Handling Structure
 */
typedef struct {
	uint8					mgmtPacketPending;		        /// TRUE for management packet pending in module
	uint8					dataPacketPending;		        /// TRUE for data packet pending in module
	uint8					powerIrqPending;			/// TRUE for power interrupt pending in the module
	uint8					bufferFull;				/// TRUE=Cannot send data, FALSE=Ok to send data
	uint8					bufferEmpty;				/// TRUE, Tx buffer empty, seems broken on module
	uint8         			isrRegLiteFi;
} rsi_intStatus;




/*==============================================*/
/**
 * Main struct which defines all the paramaters of the API library
 * This structure is defined as a variable, the  pointer to, which is passed
 *  toall the functions in the API
 * The struct is initialized by a set of #defines for default values
 *  or for simple use cases
 */

typedef struct {
	uint8					band;					/// uint8, frequency band to use
	uint8					powerMode;				
	/// uint16, power mode, 0=power mode0, 1=power mode 1, 2=power mode 2
	uint8					macAddress[6];				/// 6 bytes, mac address
	__attribute__((aligned(4))) rsi_uScan				uScanFrame;				/// frame sent as the scan command
	rsi_uJoin				uJoinFrame;				/// frame sent as the join command
	rsi_uIpparam		    uIpparamFrame;				/// frame sent to set the IP parameters
	rsi_uDns          		uDnsFrame;
	rsi_uSocket				uSocketFrame;
	uint8             		listeninterval[2];
} rsi_api;
/*==================================================*/
/**
 * This structure maintain power save state.
 *
 */ 
typedef struct {
	uint8  current_mode;
	uint8  ack_pwsave;
}rsi_powerstate;


/// Debug Print Levels
//#define RSI_DEBUG_PRINT
//#define RSI_DEBUG_LVL ( RSI_PL15 | RSI_PL6 | RSI_PL5 | RSI_PL4 | RSI_PL3 | RSI_PL2 )
#define RSI_DEBUG_LVL RSI_PL4
/// These bit values may be ored to all different combinations of debug printing
#define RSI_PL0					0xffff
#define RSI_PL1					0x0001
#define RSI_PL2					0x0002
#define RSI_PL3					0x0004
#define RSI_PL4					0x0008
#define RSI_PL5					0x0010
#define RSI_PL6					0x0020
#define RSI_PL7					0x0040
#define RSI_PL8					0x0080
#define RSI_PL9					0x0100
#define RSI_PL10				0x0200
#define RSI_PL11				0x0400
#define RSI_PL12				0x0800
#define RSI_PL13				0x1000
#define RSI_PL14				0x2000
#define RSI_PL15				0x4000
#define RSI_PL16				0x8000

#define RSI_DPRINT(lvl, ...) if (lvl & RSI_DEBUG_LVL) printf( __VA_ARGS__),putchar('\n')

/**
 * Enumerations
 */
enum RSI_INTSOURCE {
	RSI_FROM_MODULE,
	RSI_FROM_CODE,
	RSI_FROM_UNDEFINED
};

enum RSI_INTERRUPT_TYPE {
	RSI_TXBUFFER_FULL			= 0x01,
	RSI_TXBUFFER_EMPTY			= 0x02,
	RSI_MGMT_PENDING			= 0x04,
	RSI_DATA_PENDING			= 0x08,
	RSI_PWR_MODE			 	= 0x10
};

enum RSI_PROTOCOL {
	RSI_PROTOCOL_UDP			= 0x00,
	RSI_PROTOCOL_TCP			= 0x01,
	RSI_PROTOCOL_UNDEFINED			= 0x02
};

enum RSI_MODULE_MODE {
	RSI_MODE_SPI				= RSI_FALSE,
	RSI_MODE_UART				= RSI_TRUE
};

extern volatile rsi_intStatus	 		rsi_strIntStatus;
// global array of 8 possible open sockets with their associated protocol type

#include "api_lib/rsi_hal.h"
#include "rsi_config.h"
#include "api_lib/rsi_spi_api.h"
#include "api_lib/rsi_lib_util.h"
#include "rsi_app_util.h"
#endif


