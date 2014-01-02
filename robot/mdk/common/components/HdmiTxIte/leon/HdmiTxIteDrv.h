///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Definitions and types needed by the Hdmi Tx Ite operations library.
///
 
#ifndef _HDMI_TX_ITE_DRV_H_
#define _HDMI_TX_ITE_DRV_H_

// 1: Defines
// ----------------------------------------------------------------------------
// I2C address
#define _80MHz 80000000
#define HDMI_TX_I2C_SLAVE_ADDR (0x98 >> 1)

// Register offset
#define REG_TX_SW_RST       0x04
    #define B_ENTEST    (1<<7)
    #define B_REF_RST_HDMITX (1<<5)
    #define B_AREF_RST (1<<4)
    #define B_VID_RST_HDMITX (1<<3)
    #define B_AUD_RST_HDMITX (1<<2)
    #define B_TX_HDMI_RST (1<<1)
    #define B_HDCP_RST_HDMITX (1<<0)

#define REG_TX_INT_CTRL 0x05
    #define B_INTPOL_ACTL 0
    #define B_INTPOL_ACTH (1<<7)
    #define B_INT_PUSHPULL 0
    #define B_INT_OPENDRAIN (1<<6)

#define REG_TX_INT_STAT1    0x06
    #define B_INT_AUD_OVERFLOW  (1<<7)
    #define B_INT_ROMACQ_NOACK  (1<<6)
    #define B_INT_RDDC_NOACK    (1<<5)
    #define B_INT_DDCFIFO_ERR   (1<<4)
    #define B_INT_ROMACQ_BUS_HANG   (1<<3)
    #define B_INT_DDC_BUS_HANG  (1<<2)
    #define B_INT_RX_SENSE  (1<<1)
    #define B_INT_HPD_PLUG  (1<<0)

#define REG_TX_INT_MASK1    0x09
    #define B_AUDIO_OVFLW_MASK (1<<7)
    #define B_DDC_NOACK_MASK (1<<5)
    #define B_DDC_FIFO_ERR_MASK (1<<4)
    #define B_DDC_BUS_HANG_MASK (1<<2)
    #define B_RXSEN_MASK (1<<1)
    #define B_HPD_MASK (1<<0)

#define REG_TX_INT_MASK2    0x0A
    #define B_PKT_AVI_MASK (1<<7)
    #define B_PKT_VID_UNSTABLE_MASK (1<<6)
    #define B_PKT_ACP_MASK (1<<5)
    #define B_PKT_NULL_MASK (1<<4)
    #define B_PKT_GEN_MASK (1<<3)
    #define B_KSVLISTCHK_MASK (1<<2)
    #define B_T_AUTH_DONE_MASK (1<<1)
    #define B_AUTH_FAIL_MASK (1<<0)

#define REG_TX_INT_MASK3    0x0B
    #define B_HDCP_SYNC_DET_FAIL_MASK (1<<6)
    #define B_AUDCTS_MASK (1<<5)
    #define B_VSYNC_MASK (1<<4)
    #define B_VIDSTABLE_MASK (1<<3)
    #define B_PKT_MPG_MASK (1<<2)
    #define B_PKT_SPD_MASK (1<<1)
    #define B_PKT_AUD_MASK (1<<0)

#define REG_TX_INT_CLR0      0x0C
    #define B_CLR_PKTACP    (1<<7)
    #define B_CLR_PKTNULL   (1<<6)
    #define B_CLR_PKTGENERAL    (1<<5)
    #define B_CLR_KSVLISTCHK    (1<<4)
    #define B_CLR_AUTH_DONE  (1<<3)
    #define B_CLR_AUTH_FAIL  (1<<2)
    #define B_CLR_RXSENSE   (1<<1)
    #define B_CLR_HPD       (1<<0)

#define REG_TX_INT_CLR1       0x0D
    #define B_CLR_VSYNC (1<<7)
    #define B_CLR_VIDSTABLE (1<<6)
    #define B_CLR_PKTMPG    (1<<5)
    #define B_CLR_PKTSPD    (1<<4)
    #define B_CLR_PKTAUD    (1<<3)
    #define B_CLR_PKTAVI    (1<<2)
    #define B_CLR_HDCP_SYNC_DET_FAIL  (1<<1)
    #define B_CLR_VID_UNSTABLE        (1<<0)

#define REG_TX_SYS_STATUS     0x0E
    // readonly
    #define B_INT_ACTIVE    (1<<7)
    #define B_HPDETECT      (1<<6)
    #define B_RXSENDETECT   (1<<5)
    #define B_TXVIDSTABLE   (1<<4)
    // read/write
    #define O_CTSINTSTEP    2
    #define M_CTSINTSTEP    (3<<2)
    #define B_CLR_AUD_CTS     (1<<1)
    #define B_INTACTDONE    (1<<0)

#define REG_TX_BANK_CTRL        0x0F
    #define B_BANK0 0
    #define B_BANK1 1

// DDC
#define REG_TX_DDC_READFIFO    0x17
#define REG_TX_ROM_STARTADDR   0x18
#define REG_TX_HDCP_HEADER 0x19
#define REG_TX_ROM_HEADER  0x1A
#define REG_TX_BUSHOLD_T   0x1B
#define REG_TX_ROM_STAT    0x1C
    #define B_ROM_DONE  (1<<7)
    #define B_ROM_ACTIVE	(1<<6)
    #define B_ROM_NOACK	(1<<5)
    #define B_ROM_WAITBUS	(1<<4)
    #define B_ROM_ARBILOSE	(1<<3)
    #define B_ROM_BUSHANG	(1<<2)

// HDCP
#define REG_TX_AUTHFIRE    0x21
#define REG_TX_LISTCTRL    0x22
    #define B_LISTFAIL  (1<<1)
    #define B_LISTDONE  (1<<0)

#define REG_TX_AFE_DRV_CTRL 0x61
    #define B_AFE_DRV_PWD    (1<<5)
    #define B_AFE_DRV_RST    (1<<4)
    #define B_AFE_DRV_PDRXDET    (1<<2)
    #define B_AFE_DRV_TERMON    (1<<1)
    #define B_AFE_DRV_ENCAL    (1<<0)

#define REG_TX_AFE_XP_CTRL 0x62
    #define B_AFE_XP_GAINBIT    (1<<7)
    #define B_AFE_XP_PWDPLL    (1<<6)
    #define B_AFE_XP_ENI    (1<<5)
    #define B_AFE_XP_ER0    (1<<4)
    #define B_AFE_XP_RESETB    (1<<3)
    #define B_AFE_XP_PWDI    (1<<2)
    #define B_AFE_XP_DEI    (1<<1)
    #define B_AFE_XP_DER    (1<<0)

#define REG_TX_AFE_ISW_CTRL  0x63
    #define B_AFE_RTERM_SEL  (1<<7)
    #define B_AFE_IP_BYPASS  (1<<6)
    #define M_AFE_DRV_ISW    (7<<3)
    #define O_AFE_DRV_ISW    3
    #define B_AFE_DRV_ISWK   7

#define REG_TX_AFE_IP_CTRL 0x64
    #define B_AFE_IP_GAINBIT    (1<<7)
    #define B_AFE_IP_PWDPLL    (1<<6)
    #define M_AFE_IP_CKSEL    (3<<4)
    #define O_AFE_IP_CKSEL    4
    #define B_AFE_IP_ER0    (1<<3)
    #define B_AFE_IP_RESETB    (1<<2)
    #define B_AFE_IP_ENC    (1<<1)
    #define B_AFE_IP_EC1    (1<<0)

#define REG_TX_AFE_RING    0x65
    #define B_AFE_CAL_UPDATE    (1<<7)
    #define B_AFE_CAL_MANUAL    (1<<6)
    #define M_AFE_CAL_CLK_MODE    (3<<4)
    #define O_AFE_CAL_CLK_MODE    4
    #define O_AFE_DRV_VSW   2
    #define M_AFE_DRV_VSW   (3<<2)
    #define B_AFE_RING_SLOW    (1<<1)
    #define B_AFE_RING_FAST    (1<<0)
#define REG_TX_AFE_TEST    0x66
    #define B_AFE_AFE_ENTEST    (1<<6)
    #define B_AFE_AFE_ENBIST    (1<<5)
    #define M_AFE_CAL_RTERM_MANUAL    0x1F
#define REG_TX_AFE_LFSR     0x67
    #define B_AFE_AFELFSR_VAL	(1<<7)
    #define B_AFE_DIS_AFELFSR	(1<<6)
    #define M_AFE_RTERM_VAOUE    0xF

// HDMI General Control Registers
#define REG_TX_HDMI_MODE   0xC0
    #define B_TX_HDMI_MODE 1
    #define B_TX_DVI_MODE  0
#define REG_TX_GCP     0xC1
    #define B_CLR_AVMUTE    0
    #define B_SET_AVMUTE    1
    #define B_TX_SETAVMUTE        (1<<0)
    #define B_BLUE_SCR_MUTE   (1<<1)
    #define B_NODEF_PHASE    (1<<2)
    #define B_PHASE_RESYNC   (1<<3)

    #define O_COLOR_DEPTH     4
    #define M_COLOR_DEPTH     7
    #define B_COLOR_DEPTH_MASK (M_COLOR_DEPTH<<O_COLOR_DEPTH)
    #define B_CD_NODEF  0
    #define B_CD_24     (4<<4)
    #define B_CD_30     (5<<4)
    #define B_CD_36     (6<<4)
    #define B_CD_48     (7<<4)

#define REG_TX_PKT_SINGLE_CTRL 0xC5
    #define B_SINGLE_PKT    1
    #define B_BURST_PKT
    #define B_SW_CTS    (1<<1)
#define REG_TX_PKT_GENERAL_CTRL    0xC6

#define REG_TX_NULL_CTRL 0xC9
#define REG_TX_ACP_CTRL 0xCA
#define REG_TX_ISRC1_CTRL 0xCB
#define REG_TX_ISRC2_CTRL 0xCC
#define REG_TX_AVI_INFOFRM_CTRL 0xCD
#define REG_TX_AUD_INFOFRM_CTRL 0xCE
#define REG_TX_SPD_INFOFRM_CTRL 0xCF
#define REG_TX_MPG_INFOFRM_CTRL 0xD0
    #define B_ENABLE_PKT    1
    #define B_REPEAT_PKT    (1<<1)


// Bank 1
#define REGPktAudCTS0 0x30  // 7:0
#define REGPktAudCTS1 0x31  // 15:8
#define REGPktAudCTS2 0x32  // 19:16
#define REGPktAudN0 0x33    // 7:0
#define REGPktAudN1 0x34    // 15:8
#define REGPktAudN2 0x35    // 19:16
#define REGPktAudCTSCnt0 0x35   // 3:0
#define REGPktAudCTSCnt1 0x36   // 11:4
#define REGPktAudCTSCnt2 0x37   // 19:12

// COMMON PACKET for NULL,ISRC1,ISRC2,SPD
#define REG_TX_AVIINFO_DB1 0x58
#define REG_TX_AVIINFO_DB2 0x59
#define REG_TX_AVIINFO_DB3 0x5A
#define REG_TX_AVIINFO_DB4 0x5B
#define REG_TX_AVIINFO_DB5 0x5C
#define REG_TX_AVIINFO_DB6 0x5E
#define REG_TX_AVIINFO_DB7 0x5F
#define REG_TX_AVIINFO_DB8 0x60
#define REG_TX_AVIINFO_DB9 0x61
#define REG_TX_AVIINFO_DB10 0x62
#define REG_TX_AVIINFO_DB11 0x63
#define REG_TX_AVIINFO_DB12 0x64
#define REG_TX_AVIINFO_DB13 0x65
#define REG_TX_AVIINFO_SUM 0x5D

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------
#endif // _HDMI_TX_ITE_DRV_H_
