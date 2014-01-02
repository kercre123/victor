/*****************************************************************************
   @file
   @copyright All code copyright Movidius Ltd 2012, all rights reserved
               For License Warranty see: common/license.txt

   @brief     Definitions and types needed by the Hdmi Tx Ite operations library
 *****************************************************************************/

#ifndef _HDMI_TX_ITE_API_DEFINES_H_
#define _HDMI_TX_ITE_API_DEFINES_H_

/******************************************************************************
 1:  Source File Specific #defines
******************************************************************************/

// useful for generate different section for every function
#define SECTION1_NAME(x,name) name##x
#define SECTION1_NAMEs(y) #y
#define SECTION1_NAMEs2(j) SECTION1_NAMEs(j)

//HDMI Tx ITE FULL Module Sections
#define HDMITXITEFULL_CODE_SECTION(x) __attribute__((section(SECTION1_NAMEs2(SECTION1_NAME(x,.sectCode.hdmiTxIteFull.fct))))) x
#define HDMITXITEFULL_DATA_SECTION(x) __attribute__((section(SECTION1_NAMEs2(SECTION1_NAME(x,.sectData.hdmiTxIteFull.var))))) x


#define DelayMiliseconds(x) SleepMs(x);

#define HDMI_TX_ADDR_I2C (0x4C)
#define PLL_ADDR_I2C     (0x65)
#define CLK_SEL_PIN 46

#define PREVIEW_WIDTH 1280
#define PREVIEW_HIGHT 720

//hdmi defs
#define HDMI_BANK0 hdmi_i2c_w(REG_TX_BANK_CTRL, B_BANK0)
#define HDMI_BANK1 hdmi_i2c_w(REG_TX_BANK_CTRL, B_BANK1)

#define HDMI_TX_8BW   (0x98)
#define HDMI_TX_8BR   (HDMI_TX_8BW |  1)
#define HDMI_TX_7B    (HDMI_TX_8BW >> 1)
         

/*=============================================================================
      System Status Defines
 =============================================================================*/
#define     SUCCESS         0
#define     FAIL           -1
#define     ON              1
#define     OFF             0
#define     LO_ACTIVE       TRUE
#define     HI_ACTIVE       FALSE
#define     TX0DEV          0
#define     RXDEV           8
#define     TX0ADR          0x98
#define     RXADR           0x90
#define     HIGH            1
#define     LOW             0
#define     HPDON           1
#define     HPDOFF          0


/*=============================================================================
                Video Data Type
  =============================================================================*/
#define F_Tx_MODE_RGB24  0
#define F_Tx_MODE_YUV422 1
#define F_Tx_MODE_YUV444 2
#define F_Tx_MODE_CLRMOD_MASK 3


#define F_Tx_MODE_INTERLACE  1

#define F_VIDMODE_ITU709  (1<<4)
#define F_VIDMODE_ITU601  0

#define F_VIDMODE_0_255   0
#define F_VIDMODE_16_235  (1<<5)

#define F_VIDMODE_EN_UDFILT (1<<6) // output mode only,and loaded from EEPROM
#define F_VIDMODE_EN_DITHER (1<<7) // output mode only,and loaded from EEPROM

#define T_MODE_CCIR656 (1<<0)
#define T_MODE_SYNCEMB (1<<1)
#define T_MODE_INDDR (1<<2)


/*=============================================================================
              Audio
  =============================================================================*/
#define     AUDSF_32KHz         0x3
#define     AUDSF_44p1KHz       0x0
#define     AUDSF_48KHz         0x2
#define     AUDSF_88p2KHz       0x8
#define     AUDSF_96KHz         0xA
#define     AUDSF_176p4KHz      0xC
#define     AUDSF_192KHz        0xE
#define     AUDSF_Undefined     0xF

#define     AUD_I2S             0
#define     AUD_SPDIF           1

// Rev
#define     REV_UNKNOWN         0
#define     REV_IT6613          4
#define     REV_CAT6613         3
#define     REV_IT6610          2
#define     REV_CAT6611         1

#define B_AUDIO_ON    (1<<7)
#define B_HBRAUDIO    (1<<6)
#define B_DSDAUDIO    (1<<5)
#define B_AUDIO_LAYOUT     (1<<4)
#define M_AUDIO_CH         0xF
#define B_AUDIO_SRC_VALID_3 (1<<3)
#define B_AUDIO_SRC_VALID_2 (1<<2)
#define B_AUDIO_SRC_VALID_1 (1<<1)
#define B_AUDIO_SRC_VALID_0 (1<<0)

///////////////////////////////////////////////////////////////////////
// Video Data Type
///////////////////////////////////////////////////////////////////////
#define F_MODE_RGB24  0
#define F_MODE_RGB444  0
#define F_MODE_YUV422 1
#define F_MODE_YUV444 2
#define F_MODE_CLRMOD_MASK 3

#define F_MODE_INTERLACE  1

#define F_MODE_ITU709  (1<<4)
#define F_MODE_ITU601  0

#define F_MODE_0_255   0
#define F_MODE_16_235  (1<<5)

#define F_MODE_EN_UDFILT (1<<6) // output mode only,and loaded from EEPROM
#define F_MODE_EN_DITHER (1<<7) // output mode only,and loaded from EEPROM



#define O_CSC_SEL          0
#define M_CSC_SEL_MASK     3
#define B_CSC_BYPASS       0
#define B_CSC_RGB2YUV      2
#define B_CSC_YUV2RGB      3


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Packet and Info Frame Definition and Data Structure.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define     VENDORSPEC_INFOFRAME_TYPE       0x81
#define     AVI_INFOFRAME_TYPE              0x82
#define     SPD_INFOFRAME_TYPE              0x83
#define     AUDIO_INFOFRAME_TYPE            0x84
#define     MPEG_INFOFRAME_TYPE             0x85
#define     VENDORSPEC_INFOFRAME_VER        0x01

#define     AVI_INFOFRAME_VER               0x02
#define     SPD_INFOFRAME_VER               0x01
#define     AUDIO_INFOFRAME_VER             0x01
#define     MPEG_INFOFRAME_VER              0x01

#define     VENDORSPEC_INFOFRAME_LEN         8
#define     AVI_INFOFRAME_LEN               13
#define     SPD_INFOFRAME_LEN               25
#define     AUDIO_INFOFRAME_LEN             10
#define     MPEG_INFOFRAME_LEN              10

#define B_CAP_AUDIO_ON  (1 << 7)
#define B_CAP_HBR_AUDIO (1 << 6)
#define B_CAP_DSD_AUDIO (1 << 5)
#define B_LAYOUT        (1 << 4)
#define B_MULTICH       (1 << 4)
#define B_HBR_BY_SPDIF  (1 << 3)
#define B_SPDIF         (1 << 2)
#define B_CAP_LPCM      (1 << 0)


/******************************************************************************
  2: Typedefs (types, enums, structs)
******************************************************************************/

/*=============================================================================
  Used types
  =============================================================================*/
typedef                 int     BOOL;
typedef     unsigned    char    BYTE, *PBYTE;
typedef                 short   SHORT;
typedef     unsigned    short   USHORT,*PUSHORT;
typedef     unsigned    short   word,*pword;
typedef     unsigned    short   WORD;
typedef     unsigned    long    ULONG;
typedef     unsigned    long    DWORD;


typedef enum _SYS_STATUS
{
    ER_SUCCESS = 0,
    ER_FAIL,
    ER_RESERVED
} SYS_STATUS;

/*=============================================================================
  Audio Types
  =============================================================================*/
typedef union _RX_REG_AUDIO_CHSTS
{
    struct
    {
        #ifndef BIG_ENDIAN
        BYTE rev :1;
        BYTE ISLPCM :1;
        BYTE CopyRight : 1;
        BYTE AdditionFormatInfo :3;
        BYTE ChannelStatusMode :2;
        BYTE CategoryCode;
        BYTE SourceNumber :4;
        BYTE ChannelNumber :4;
        BYTE SamplingFreq :4;
        BYTE ClockAccuary :2;
        BYTE rev2 : 2;
        BYTE WorldLen : 4;
        BYTE OriginalSamplingFreq :4;
        #else //BIG_ENDIAN
        BYTE ChannelStatusMode :2;
        BYTE AdditionFormatInfo :3;
        BYTE CopyRight : 1;
        BYTE ISLPCM :1;
        BYTE rev :1;
        BYTE CategoryCode;
        BYTE ChannelNumber :4;
        BYTE SourceNumber :4;
        BYTE rev2 : 2;
        BYTE ClockAccuary :2;
        BYTE SamplingFreq :4;
        BYTE OriginalSamplingFreq :4;
        BYTE WorldLen : 4;
        #endif // BIG_ENDIAN
    } chstatus;

    struct
    {
        BYTE ChStat[5];
    } ChStatByte;

} RX_REG_AUDIO_CHSTS;

typedef struct
{
    BYTE AudioFlag ;
    BYTE AudSrcEnable ;
    BYTE SampleFreq ;
    RX_REG_AUDIO_CHSTS ChStat;
} AUDIO_CAPS ;


/*=============================================================================
     Info Frame Types
  =============================================================================*/
typedef union AviInfoFrameInformation
{
    #ifndef BIG_ENDIAN
    struct
    {
        BYTE Type;
        BYTE Ver;
        BYTE Len;

        BYTE Scan:2;
        BYTE BarInfo:2;
        BYTE ActiveFmtInfoPresent:1;
        BYTE ColorMode:2;
        BYTE FU1:1;

        BYTE ActiveFormatAspectRatio:4;
        BYTE PictureAspectRatio:2;
        BYTE Colorimetry:2;

        BYTE Scaling:2;
        BYTE FU2:6;

        BYTE VIC:7;
        BYTE FU3:1;

        BYTE PixelRepetition:4;
        BYTE FU4:4;

        short Ln_End_Top;
        short Ln_Start_Bottom;
        short Pix_End_Left;
        short Pix_Start_Right;
    } info;

    #else // BIG_ENDIAN

    struct
    {
        BYTE Type;
        BYTE Ver;
        BYTE Len;

        BYTE FU1:1;
        BYTE ColorMode:2;
        BYTE ActiveFmtInfoPresent:1;
        BYTE BarInfo:2;
        BYTE Scan:2;

        BYTE Colorimetry:2;
        BYTE PictureAspectRatio:2;
        BYTE ActiveFormatAspectRatio:4;

        BYTE FU2:6;
        BYTE Scaling:2;

        BYTE FU3:1;
        BYTE VIC:7;

        BYTE FU4:4;
        BYTE PixelRepetition:4;

        short Ln_End_Top;
        short Ln_Start_Bottom;
        short Pix_End_Left;
        short Pix_Start_Right;
    } info;

    #endif //BIG_ENDIAN

    struct
    {
        BYTE AVI_HB[3];
        BYTE AVI_DB[AVI_INFOFRAME_LEN];
    }
    PacketByte;
}
AviInfoFrameType;


typedef union AudioInfoFrameInformation
{
    #ifndef BIG_ENDIAN
    struct
    {
        BYTE Type;
        BYTE Ver;
        BYTE Len;

        BYTE AudioChannelCount:3;
        BYTE RSVD1:1;
        BYTE AudioCodingType:4;

        BYTE SampleSize:2;
        BYTE SampleFreq:3;
        BYTE Rsvd2:3;

        BYTE FmtCoding;

        BYTE SpeakerPlacement;

        BYTE Rsvd3:3;
        BYTE LevelShiftValue:4;
        BYTE DM_INH:1;
    } info;

    #else //BIG_ENDIAN

    struct
    {
        BYTE Type;
        BYTE Ver;
        BYTE Len;

        BYTE AudioCodingType:4;
        BYTE RSVD1:1;
        BYTE AudioChannelCount:3;

        BYTE Rsvd2:3;
        BYTE SampleFreq:3;
        BYTE SampleSize:2;

        BYTE FmtCoding;

        BYTE SpeakerPlacement;

        BYTE DM_INH:1;
        BYTE LevelShiftValue:4;
        BYTE Rsvd3:3;
    } info;
    #endif //BIG_ENDIAN

    struct
    {
        BYTE AUD_HB[3];
        BYTE AUD_DB[5];
    }
    PacketByte;
}
AudioInfoFrameType;

typedef enum HdmiDriverRunningModes
{
    ModeFullRepeater                = 0,
    ModePartialRepeater3D           = 1,
    ModePartialRepeaterCoversion    = 2, // 1080p24    -> 1080i60
    ModePartialRepeater3DCoversion  = 3, // 1080p24 FP -> 1080i60
    ModeTxIndependantVideoOnly      = 4,
    ModeTxIndependantAnalog         = 5,
    ModeAutomatedTesting            = 20
}
HdmiDriverMode;

typedef enum
{
    CLK_EXT_DISABLED,
    CLK_EXT_DISABLED_1080p,
    CLK_EXT_ENABLE,
    CLK_EXT_ENABLE_DIV2
} ExternalClockType;

typedef enum
{
    nbSUCCESS,      // Request successfully completed
    nbERROR,        // A failure occurred
    nbPENDING,      // Non-blocking request is pending (operation has not completed yet)
    nbINVALID       //
} eNBRESULT;


typedef enum Hdmi3DSupportedFormats  // Alex: See Bug 15692 -> Comment #2
{
    MODE_3D_NOTHING         = 0,    // 0 - No 3D format detected
    MODE_3D_FRAME_PACKED    = 1,    // 1 - Frame-Packed   VSI detected
    MODE_3D_SIDE_BY_SIDE    = 2,    // 2 - Side by Side   VSI detected
    MODE_3D_TOP_AND_BOTTOM  = 3,    // 3 - Top and Bottom VSI detected
    MODE_3D_ERROR           = 99
}
Hdmi3DFormat;

typedef struct HdmiDriverInformation
{    unsigned int TxResolutionIndex;                 //  4 // Current output resolution Lcd + Tx
     Hdmi3DFormat Tx3DInformation;                   //  6 // Stores the 3D information on the Tx (FP, SBS, TAB)
    unsigned int IndependentTx;                     //  7 // Used in Tx to split it from the Rx
    unsigned int IndependentRx;                     //  8 // Used in Rx to split it from the Tx
    unsigned int TvSupports3D;                      //  9 // If 1 then Tv is 3D capable
    unsigned int TvSupports1080p24;                 // 10 // If 1 then Tv is 1080p@24Hz capable
     volatile unsigned int TxVideoState;             // 14 // Shows the current video state of the Tx
    unsigned int TxAudioState;                      // 15 // Shows the current audio state of the Tx
    unsigned int TxHdcpState;                       // 16 // Shows the current HDCP state of the Tx
}
HdmiDriverStatus;

typedef enum TxVideoStatesInformation
{
    TxVStateReset               = 0,
    TxVStateUnplug              = 1,
    TxVStateHPD                 = 2,
    TxVStateWaitForVStable      = 3,
    TxVStateVideoInit           = 4,
    TxVStateVideoSetup          = 5, // 2008/09/04 added by jj_tseng@chipadvanced.com
    TxVStateInitPatternGen      = 6,
    TxVStateWaitForPGVStable    = 7,
    TxVStatePatternGenOff       = 8,
    TxVStateVideoOn             = 9, // ~jj_tseng@chipadvanced.com 2008/09/04
    TxVStateReserved            = 10
}
TxVideoStateType;

typedef enum TxAudioStatesInformation
{
    TxAStateOff         = 0,
    TxAStateCheck       = 1,
    TxAStateOn          = 2,
    TxAStateReserved    = 3
}
TxAudioStateType;

typedef enum _Tx_HDCP_State_Type
{
    TxHDCP_Off = 0,                 //  0
    TxHDCP_AuthRestart,             //  1
    TxHDCP_AuthStart,               //  2
    TxHDCP_Receiver,                //  3
    TxHDCP_Repeater,                //  4
    TxHDCP_CheckFIFORDY,            //  5
    TxHDCP_VerifyRevocationList,    //  6
    TxHDCP_CheckSHA,                //  7
    TxHDCP_Authenticated,           //  8
    TxHDCP_AuthFail,                //  9
    TxHDCP_RepeaterFail,            // 10
    TxHDCP_RepeaterSuccess,         // 11
    TxHDCP_Reserved                 // 12
} Tx_HDCP_State_Type;

typedef struct TxDeviceParameters
{
    TxVideoStateType     TxVState;
    TxAudioStateType     TxAState;
    Tx_HDCP_State_Type      TxHDCPState;

    WORD    TxVStateCounter;
    WORD    TxAStateCounter;
    WORD    TxHDCPStateCounter;

    BYTE    bChipRevision;

    BYTE    bTxVidReset             : 1;
    BYTE    bTxVidReady             : 1;
    BYTE    bTxAudReady             : 1;
    BYTE    bTxAudSel               : 1;
    BYTE    bTxAudioEnable          : 4;

    BYTE    bTxInputVideoMode       : 2;
    BYTE    bTxOutputVideoMode      : 2;
    BYTE    bTxVideoInputType       : 3;
    BYTE    bTxHDMIMode             : 1;

    BYTE    TxStableCnt             : 2;
    BYTE    bTxHDCP                 : 1;
    BYTE    bTxAuthenticated        : 1;
    BYTE    bUpExitVideoOn          : 1;
    BYTE    RxUpHDMIMode            : 1;
    BYTE    bDnHDMIMode             : 1;
    BYTE    bDnEDIDRdy              : 1;

    // Flag Area
    BYTE    TxEMEMStatus            : 1;
    BYTE    TxPatGen                : 1;

    BYTE    bSinkIsRepeater         : 1;

    BYTE    AucSrcLPCM              : 1;
    BYTE    AudSrcHBR               : 1;
    BYTE    AudSrcDSD               : 1;
    BYTE    bSrcAudChange           : 1;
    BYTE    bSrcAudREADY            : 1;

    // Audio Support Area
    BYTE    bDnBasicAudEn           : 1;
    BYTE    bDnLPCMFreq             : 7;

    BYTE    bDnLPCMSWL              : 3;
    BYTE    bDnLPCMChNum            : 3;
    BYTE    bDnSupportVidMode       : 2;

    BYTE    bDnSpeakValid           : 1;
    BYTE    bDnSpeakAlloc           : 7;

    BYTE    bOutAudChannelSrc       : 3;
    BYTE    bOutAudCompress         : 1;
    BYTE    bOutAudSampleFrequence  : 4;

    BYTE    bEDIDChange             : 1;
    BYTE    bRXHPDPULLED            : 1;
    BYTE    enableHDMISink          : 1;

} TxDevPara,*PTxDevPara;

/******************************************************************************
  3: Local const declarations     NB: ONLY const declarations go here
******************************************************************************/

#define FALSE        (0)
#define TRUE         (1)

#define TX0DEV 0
#endif // _HDMI_API_DEFINES_H_;
