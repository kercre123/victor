/*****************************************************************************
   @file   <HdmiTxIte.c>
   @copyright All code copyright Movidius Ltd 2012, all rights reserved.
            For License Warranty see: common/license.txt
   @brief     Hdmi Tx Functions

   @fileversion: ITE_HDMI1.4_AVR_SAMPLECODE_2.04

 *****************************************************************************/

/******************************************************************************
 1: Included types first then Apis from other modules
******************************************************************************/

#include "HdmiTxIteApi.h"
#include "HdmiTxIteDrv.h"
#include "TxHdcp.h"
#include "DrvSvu.h"
#include "DrvI2cMaster.h"
#include "DrvI2c.h"
#include "assert.h"
#include "CEA861DDefines.h"
#include "HdmiTxItePrivate.h"

/******************************************************************************
  2: Local module extern's
******************************************************************************/

HdmiDriverStatus    HdmiDriverInfo;
TxDevPara    TxDev[1];

/******************************************************************************
 3:  Source File Specific #defines
******************************************************************************/
#ifdef _HDMITX_DEBUG_PRINT_
    #define HDMITX_DEBUG_PRINT(x) printf("Tx : "); printf x
#else // _HDMITX_DEBUG_PRINT_
    #define HDMITX_DEBUG_PRINT(x)
#endif

#ifdef _HDMITX_DEBUG_PRINT_
    #define HDMITX_DEBUG_VIDEO_STATES_PRINT(x) printf("Tx : "); printf x
#else // _HDMITX_DEBUG_PRINT_
    #define HDMITX_DEBUG_VIDEO_STATES_PRINT(x)
#endif

#ifdef ENABLE_UNSUPPORTED_AUDIO_MSG
#define DISABLE_UNSUPPORTED_AUDIO_MSG 0
#else
#define DISABLE_UNSUPPORTED_AUDIO_MSG 1
#endif

#ifdef PENDING_HPD_FORWARD
#define     TX_UNPLUG_TIMEOUT                   MS_TimeOut  (1000)
#else //PENDING_HPD_FORWARD
#define     TX_UNPLUG_TIMEOUT                   MS_TimeOut   (300)
#endif //PENDING_HPD_FORWARD

#define     TX_WAITVIDSTBLE_TIMEOUT             MS_TimeOut   (100)
#define     SIZEOF_CSCMTX          18


const BYTE TxVSIheader[4]={
//		0x81, 0x01, 0x05, 0x00
		0x00, 0x00, 0x00, 0x00
};
const BYTE TxVSIinfo[28]={
//		0x2A, 0x03, 0x0C, 0x00,
//		0x40, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
};
int AudioChannelArray[6];

/*=============================================================================
   Start of cat6611.c
  =============================================================================*/


#pragma region Color Space Conversion Data
// TX CSC Table
BYTE bCSCOffset_16_235[]=
{
    0x00,0x80,0x00
};

BYTE bCSCOffset_0_255[]=
{
    0x10,0x80,0x10
};

//#ifdef SUPPORT_INPUTRGB
BYTE bCSCMtx_RGB2YUV_ITU601_16_235[]=
{
    0xB2,0x04,0x64,0x02,0xE9,0x00,
    0x93,0x3C,0x16,0x04,0x56,0x3F,
    0x49,0x3D,0x9F,0x3E,0x16,0x04
};

BYTE bCSCMtx_RGB2YUV_ITU601_0_255[]=
{
    0x09,0x04,0x0E,0x02,0xC8,0x00,
    0x0E,0x3D,0x83,0x03,0x6E,0x3F,
    0xAC,0x3D,0xD0,0x3E,0x83,0x03
};

BYTE bCSCMtx_RGB2YUV_ITU709_16_235[]=
{
    0xB8,0x05,0xB4,0x01,0x93,0x00,
    0x49,0x3C,0x16,0x04,0x9F,0x3F,
    0xD9,0x3C,0x10,0x3F,0x16,0x04
};

BYTE bCSCMtx_RGB2YUV_ITU709_0_255[]=
{
    0xE5,0x04,0x78,0x01,0x81,0x00,
    0xCE,0x3C,0x83,0x03,0xAE,0x3F,
    0x49,0x3D,0x33,0x3F,0x83,0x03
};
//#endif

//#ifdef SUPPORT_INPUTYUV

BYTE bCSCMtx_YUV2RGB_ITU601_16_235[]=
{
    0x00,0x08,0x6A,0x3A,0x4F,0x3D,
    0x00,0x08,0xF7,0x0A,0x00,0x00,
    0x00,0x08,0x00,0x00,0xDB,0x0D
};

BYTE bCSCMtx_YUV2RGB_ITU601_0_255[]=
{
    0x4F,0x09,0x81,0x39,0xDF,0x3C,
    0x4F,0x09,0xC2,0x0C,0x00,0x00,
    0x4F,0x09,0x00,0x00,0x1E,0x10
};

BYTE bCSCMtx_YUV2RGB_ITU709_16_235[]=
{
    0x00,0x08,0x53,0x3C,0x89,0x3E,
    0x00,0x08,0x51,0x0C,0x00,0x00,
    0x00,0x08,0x00,0x00,0x87,0x0E
};

BYTE bCSCMtx_YUV2RGB_ITU709_0_255[]=
{
    0x4F,0x09,0xBA,0x3B,0x4B,0x3E,
    0x4F,0x09,0x56,0x0E,0x00,0x00,
    0x4F,0x09,0x00,0x00,0xE7,0x10
};
//#endif
#pragma endregion

/******************************************************************************
 4:  Global Data (Only if absolutely necessary)
******************************************************************************/

/******************************************************************************
 5:  Local variables
******************************************************************************/

unsigned int Vic = 0;
// Alex : Added this variable to change the resolution VIC in independant Tx running mode
unsigned int resolutionIndex = 32;
unsigned int VideoOnTime;

static I2CM_Device * localHdmiI2cHandle;

static u8 hdmiWriteProto[] = I2C_PROTO_WRITE_8BA;
static u8 hdmiReadProto[]  = I2C_PROTO_READ_8BA;

/******************************************************************************
  6: Static Function Prototypes
******************************************************************************/

#ifdef TX_PATGEN
static void TX_PatternGen_Process(BYTE Flag,BYTE RxClkXCNT);
#endif // TX_PATGEN



int HDMITX_WriteI2C_Byte (int RegAddr, u8 value);
u8 HDMITX_ReadI2C_Byte (int RegAddr);

void HdmiDriverInit (void);
void C6613_Check_EMEM_sts (void);
void HDMITX_IdentifyRev (void);
void InitHDMITX_HDCPROM (void);
void InitTxDev (int bPwrOnFlag);
void InitTxInt (void);
SYS_STATUS InitAudioMode (void);
SYS_STATUS InitVideoMode (void);
SYS_STATUS SetupTxVideo (unsigned int TxDriverMode);

void SetTXColordepth (void);
void MdkSetAVIInfoFrame (/*AviInfoFrameType *pAVIInfoFrame*/);

void Tx_SwitchVideoState (TxVideoStateType state, unsigned int TxDriverMode);
void TX_SwitchAudioState (TxAudioStateType state);

void PwrDnTxAFE (void);
void SetTxAVMute (void);
void ClrTxAVMute(void);

void SwReset();

/*=============================================================================
     7:  Function exported
  =============================================================================*/

// Function used in main to configure the i2c for tx driver
int HDMITXITEFULL_CODE_SECTION(HdmiSetup)(I2CM_Device * hdmiI2cHandle)
{
    if (hdmiI2cHandle == NULL)
        return -1;
    localHdmiI2cHandle = hdmiI2cHandle;
    return;
}

// Function used in main to configure the entire TX
void HDMITXITEFULL_CODE_SECTION(HdmiConfigure)()
{
	//Init the Tx driver
	HdmiDriverInit();

	// Set AV mute
	SetTxAVMute();

	//Setup AFE
	InitVideoMode();

	// Enable AVI info frame
	MdkSetAVIInfoFrame();

	// Unset AV mute
	ClrTxAVMute();

}

/******************************************************************************
 8: I2c tx Functions Implementation
******************************************************************************/


u8 HDMITX_ReadI2C_Byte (int RegAddr)
{
	int ret;
	u8 value;
	(void)DrvI2cMTransaction(localHdmiI2cHandle,HDMI_TX_7B, RegAddr ,hdmiReadProto, &value , 1);
	return value;
}

int HDMITX_WriteI2C_Byte (int RegAddr, u8 value)
{

	int ret;
	(void)DrvI2cMTransaction(localHdmiI2cHandle,HDMI_TX_7B, RegAddr ,hdmiWriteProto, &value , 1);
	return ret;
}

SYS_STATUS HDMITXITEFULL_CODE_SECTION(HDMITX_WriteI2C_ByteN) (SHORT RegAddr, BYTE *pData, int N)
{
    //printfI2CTrasfer ("RegAddr : 0x%X, I2CBaseAddress : 0x%X, I2CAddress : 0x%X, HdmiI2CAddress 0x%X \n", RegAddr, IIC1_BASE_ADR, (IIC1_BASE_ADR + IIC_TAR, HDMI_TX_ADDR_I2C));

    BOOL flag;
    int i;

    DrvIicEn (IIC2_BASE_ADR, 0); // Disable Module
    SET_REG_WORD (IIC2_BASE_ADR + IIC_TAR, HDMI_TX_ADDR_I2C);
    DrvIicEn (IIC2_BASE_ADR, 1); // Enable Module

    #ifdef HDMI_IO_CHECK_I2C_ACCESSES
    u32 tar;
    GET_REG_WORD (IIC2_BASE_ADR + IIC_TAR, tar);

    if (tar != HDMI_TX_I2C_SLAVE_ADDR)
    {
        //printfI2CTrasferCheck("***HDMITX_WriteI2C_ByteN fail i2c check!\n");
        asm volatile("t 0");
    }
    #endif

    for (i = 0; i < N; i++)
    {
        if (DrvIicWr2 (IIC2_BASE_ADR, RegAddr + i, *((BYTE*)(pData + i))))
        {
            return ER_FAIL;
        }
    }

    return ER_SUCCESS;
}

SYS_STATUS HDMITXITEFULL_CODE_SECTION(HDMITX_ReadI2C_ByteN) (BYTE RegAddr, BYTE *pData, int N)
{

    BOOL flag;
    int i;

    DrvIicEn (IIC2_BASE_ADR, 0); // Disable Module
    SET_REG_WORD (IIC2_BASE_ADR + IIC_TAR, HDMI_TX_ADDR_I2C);
    DrvIicEn (IIC2_BASE_ADR, 1); // Enable Module

    #ifdef HDMI_IO_CHECK_I2C_ACCESSES
    u32 tar;
    GET_REG_WORD (IIC2_BASE_ADR + IIC_TAR, tar);

    if (tar != HDMI_TX_I2C_SLAVE_ADDR)
    {
        //printfI2CTrasferCheck("****HDMITX_ReadI2C_ByteN fail i2c check!\n");
        asm volatile("t 0");
    }
    #endif

    for (i = 0; i < N; i++)
    {
        pData[i] = DrvIicRd8ba (IIC2_BASE_ADR, RegAddr + i);
    }

    return ER_SUCCESS;
}

/******************************************************************************
  9: Functions Implementation
******************************************************************************/

void HDMITXITEFULL_CODE_SECTION(HdmiDriverInit) (void)
{
	HdmiDriverInfo.TxResolutionIndex = 999;
	HdmiDriverInfo.Tx3DInformation = MODE_3D_ERROR;
	HdmiDriverInfo.TvSupports1080p24 = 999;
	HdmiDriverInfo.TvSupports3D = 999;
    HdmiDriverInfo.IndependentTx = 0;
    HdmiDriverInfo.TxResolutionIndex = 0;

	// Confugure I2C2 used for TX
	DrvIicTestInit(IIC2_BASE_ADR, HDMI_TX_ADDR_I2C);
	HDMITX_WriteI2C_Byte(REG_TX_INT_CTRL, 0x40);

    // Chip identifying
    InitTxDev(TRUE);

    // Set default interrupt mask for event handling
    InitTxInt();

    HDMITX_WriteI2C_Byte(REG_TX_HDMI_MODE, B_TX_HDMI_MODE);
}



/*=============================================================================
 Tx Hot plug detection
  =============================================================================*/

BOOL HDMITXITEFULL_CODE_SECTION(TxActiveCableIsPluggedIn) (void)
{
    if ((HDMITX_ReadI2C_Byte(REG_TX_SYS_STATUS) & (B_HPDETECT | B_RXSENDETECT)) != (B_HPDETECT | B_RXSENDETECT))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


/*=============================================================================
 Function: HDMITX_IdentifyRev - detect the HDMITX part
 Side-Effect: DDC master will set to be HOST
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(HDMITX_IdentifyRev) (void)
{
    BYTE uc ;

    TxDev[TX0DEV].bChipRevision = REV_UNKNOWN ;
    uc = HDMITX_ReadI2C_Byte(REG_TX_DEVICE_ID0) ;

    if( uc == 0x13 )
    {
        uc = HDMITX_ReadI2C_Byte(REG_TX_DEVICE_ID1) ;
        if( uc == 0x16 )
        {
            TxDev[TX0DEV].bChipRevision = REV_IT6613 ;
            //HDMITX_DEBUG_PRINT(("Chip is IT6613\n"));
        }
        else
        {
            TxDev[TX0DEV].bChipRevision = REV_CAT6613 ;
            //HDMITX_DEBUG_PRINT(("Chip is CAT6613\n"));
        }
    }
    else if( uc == 0x11 )
    {
        uc = HDMITX_ReadI2C_Byte(REG_TX_INT_CTRL) ;
        if( uc&(1<<5) )
        {
            TxDev[TX0DEV].bChipRevision = REV_IT6610 ;
            //HDMITX_DEBUG_PRINT(("Chip is CAT6612/IT6610\n"));
        }
        else
        {
            TxDev[TX0DEV].bChipRevision = REV_CAT6611 ;
            //HDMITX_DEBUG_PRINT(("Chip is CAT6611\n"));
        }
    }
}



/*=============================================================================
 Function: ClearDDCFIFO-  clear the DDC FIFO.
  Side-Effect: DDC master will set to be HOST.
  Alex: Used in Edid.c and TxHdcp.c
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(ClearDDCFIFO) (void)
{

    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERDDC|B_MASTERHOST);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_CMD,CMD_FIFO_CLR );
}


/*=============================================================================
   Function: AbortDDC - Force abort DDC and reset DDC bus
   Alex: Used in Edid.c
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(AdjustDDC) (BOOL bEnable)
{
    switch( TxDev[TX0DEV].bChipRevision)
    {
    case REV_IT6610:
    case REV_CAT6611:
        if( bEnable )
        {
            HDMITX_WriteI2C_Byte(0x65, 0x02) ;
        }
        else
        {
            HDMITX_WriteI2C_Byte(0x65, 0x00) ;
        }
        break ;
    case REV_CAT6613:
    case REV_IT6613:
        if( bEnable )
        {
            HDMITX_OrReg_Byte(REG_TX_INT_CTRL, (1<<1)) ;
        }
        else
        {
            HDMITX_AndReg_Byte(REG_TX_INT_CTRL, ~(1<<1)) ;
        }
        break ;
        break ;
    }
}


/*=============================================================================
   Alex: Used in Edid.c and TxHdcp.c
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(AbortDDC) (void)
{
    BYTE CPDesire,SWReset,DDCMaster;
    BYTE Watch_dog,abort_count;

    // save the SW reset,DDC master,and CP Desire setting.
    SWReset=HDMITX_ReadI2C_Byte(REG_TX_SW_RST);
    CPDesire=HDMITX_ReadI2C_Byte(REG_TX_HDCP_DESIRE);
    DDCMaster=HDMITX_ReadI2C_Byte(REG_TX_DDC_MASTER_CTRL);

    AdjustDDC(ON) ;


    HDMITX_WriteI2C_Byte(REG_TX_HDCP_DESIRE,CPDesire&(~B_CPDESIRE)); // @emily change order
    HDMITX_WriteI2C_Byte(REG_TX_SW_RST,SWReset|B_HDCP_RST);            // @emily change order
    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERDDC|B_MASTERHOST);

    for(abort_count=0;abort_count < 2; abort_count++){            //abort twice to fix BENQ ddc hang bug      02/13/08

        while((abort_count &0xF0)<0xF0)
        {
            HDMITX_WriteI2C_Byte(REG_TX_DDC_CMD,CMD_DDC_ABORT );
            Watch_dog=10;                                                //add watch_dog to prevent while loop hang     02/13/08
            while((HDMITX_ReadI2C_Byte(REG_TX_DDC_STATUS)&B_DDC_DONE)==0 ){
                if(Watch_dog--==0)
                    break;
            }

            if( 0xC0 == (HDMITX_ReadI2C_Byte(REG_TX_DDC_CMD) & 0xC0) )
            {
                break ;
            }
            abort_count += 0x10 ;
        }
        abort_count &= 0xF ;

    }

    AdjustDDC(OFF) ;
    HDMITX_WriteI2C_Byte(REG_TX_SW_RST,SWReset);
    HDMITX_WriteI2C_Byte(REG_TX_HDCP_DESIRE,CPDesire);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,DDCMaster);
}


/*=============================================================================
   Initialization process
  =============================================================================*/

void HDMITXITEFULL_CODE_SECTION(InitTxDev) (int bPwrOnFlag)
{

	if(bPwrOnFlag==TRUE)
	    {
	        // RstTxDev();
	        //HDMITX_DEBUG_PRINT(("InitTxDev\n"));
	        HDMITX_IdentifyRev() ;

	        #ifdef _ENCEC_
	            if(// I2CDEV==CECDEV)
	                {
	                    CECAdr[0]=0x00;
	                    CECAdr[1]=0x00;
	                }
	        #endif

	        TxDev[TX0DEV].bDnHDMIMode=TRUE;    //FALSE;
	        TxDev[TX0DEV].bDnBasicAudEn=TRUE;    //FALSE;

	        TxDev[TX0DEV].bDnSupportVidMode=0x0;

	        TxDev[TX0DEV].bDnLPCMFreq=0x00;
	        TxDev[TX0DEV].bDnLPCMChNum=0x1;
	        TxDev[TX0DEV].bDnLPCMSWL=0x0;

	        TxDev[TX0DEV].bDnSpeakValid=FALSE;
	        TxDev[TX0DEV].bDnSpeakAlloc=0x00;

	        TxDev[TX0DEV].bDnEDIDRdy=FALSE;

	        //#ifdef _COPYEDID_
	        // TODO: Alex: Commented this, replaced the dirver Edid with sepparate module.
	        //bEDIDCopyDone=FALSE;
	        //#endif

	        if( TxDev[TX0DEV].bChipRevision >= REV_CAT6613 )
	        {
	            InitHDMITX_HDCPROM();                                        //hermes 20080214
	        }
	    }

	    TxDev[TX0DEV].bTxVidReset=FALSE;
	    TxDev[TX0DEV].bTxVidReady=FALSE;
	    TxDev[TX0DEV].bTxAudReady=FALSE;
	/*  // #ifdef _AUD_COMPRESS_VIA_I2S_
	        if (routeOnI2S==1)
	        {
	            TxDev[TX0DEV].bTxAudSel=AUD_I2S;
	        }
	    //#else
	        else
	        {
	            TxDev[TX0DEV].bTxAudSel=AUD_SPDIF;
	        }
	    //#endif  --TODO ---remove this*/

	    TxDev[TX0DEV].bTxAudioEnable=0x0;

	    TxDev[TX0DEV].bTxInputVideoMode=F_Tx_MODE_RGB24;
	    TxDev[TX0DEV].bTxOutputVideoMode=F_Tx_MODE_RGB24;
	    TxDev[TX0DEV].bTxVideoInputType=0x0;
	    TxDev[TX0DEV].bUpExitVideoOn=TRUE;

	    TxDev[TX0DEV].bTxHDMIMode=TRUE;    //FALSE;
	    TxDev[TX0DEV].bTxAuthenticated=FALSE;

	#ifdef _ENABLE_HDCP_REPEATER_
	    TxDev[TX0DEV].bTxHDCP=TRUE;
	#else
	    #ifndef _DISABLE_HDCP_
	    TxDev[TX0DEV].bTxHDCP=TRUE;    //TRUE;
	    #else
	    TxDev[TX0DEV].bTxHDCP=FALSE;    //TRUE;
	    #endif
	#endif

	    TxDev[TX0DEV].RxUpHDMIMode = FALSE;
	    Tx_SwitchHDCPState(TxHDCP_Off);
	    Tx_SwitchVideoState(TxVStateUnplug, HdmiDriverInfo.IndependentTx);
	    TX_SwitchAudioState(TxAStateOff);
}

void HDMITXITEFULL_CODE_SECTION(InitTxInt) (void)
{
	// Set the REG_INT_Mode into Open-Drain mode
    HDMITX_WriteI2C_Byte(REG_TX_INT_CTRL,0x40);

    // Set default interrupt mask for event handling
    HDMITX_WriteI2C_Byte(REG_TX_INT_MASK1,0xD8);
    HDMITX_WriteI2C_Byte(REG_TX_INT_MASK2,0xF8);
    HDMITX_WriteI2C_Byte(REG_TX_INT_MASK3,0xFF);
    InitHDMITX_HDCPROM();                                        //hermes 20080214
}


BOOL HDMITXITEFULL_CODE_SECTION(IsTxSwRst) (void)
{
    if(HDMITX_ReadI2C_Byte(REG_TX_SW_RST)==0x1C && HDMITX_ReadI2C_Byte(REG_TX_AFE_DRV_CTRL)==0x30)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void HDMITXITEFULL_CODE_SECTION(RstTxDev) (void)
{
	HDMITX_WriteI2C_Byte(REG_TX_SW_RST,B_REF_RST);
    PwrDnTxAFE();
}


void HDMITXITEFULL_CODE_SECTION(DisTxAudio) (void)
{
    TX_SwitchAudioState(TxAStateOff);
}

void HDMITXITEFULL_CODE_SECTION(SetTXColordepth) (void)
{
    BYTE uc;
    BYTE ColorDepth;
    BYTE bColorDepth;

    if (TxDev[TX0DEV].bChipRevision < REV_CAT6613)
    {
        Switch_HDMITX_Bank(0);
        uc = HDMITX_ReadI2C_Byte(REG_TX_GCP);
        bColorDepth = B_CD_NODEF;
        HDMITX_WriteI2C_Byte(REG_TX_PKT_GENERAL_CTRL, B_ENABLE_PKT | B_REPEAT_PKT);
        return;
    }

    if (TxDev[TX0DEV].bDnHDMIMode)
    {
        Switch_HDMITX_Bank(0);
        uc = HDMITX_ReadI2C_Byte(REG_TX_GCP);
        bColorDepth = B_CD_24;
    }
    else
    {
        bColorDepth = B_CD_NODEF;
    }

    uc &= ~B_COLOR_DEPTH_MASK;
    uc |=  bColorDepth & B_COLOR_DEPTH_MASK;

    // Set the color depth
    HDMITX_WriteI2C_Byte (REG_TX_GCP, uc);
    // Repeat General Control packet
    HDMITX_WriteI2C_Byte (REG_TX_PKT_GENERAL_CTRL, B_ENABLE_PKT | B_REPEAT_PKT);
}


/*=============================================================================
   Function: SetInputMode
   Parameter: InputMode,VideoInputType
        InputMode - use [1:0] to identify the color space for reg70[7:6],
                    definition:
                       #define F_Tx_MODE_RGB24  0
                       #define F_Tx_MODE_YUV422 1
                       #define F_Tx_MODE_YUV444 2
                       #define F_Tx_MODE_CLRMOD_MASK 3
        VideoInputType - defined the CCIR656 D[0],SYNC Embedded D[1],and
                       DDR input in D[2].
   Remark: program Reg70 with the input value.
   Side-Effect: Reg70.
//=============================================================================*/
void HDMITXITEFULL_CODE_SECTION(SetInputMode) (BYTE InputMode,BYTE VideoInputType)
{
    BYTE d;

    d=HDMITX_ReadI2C_Byte(REG_TX_INPUT_MODE);

    d &=~(M_INCOLMOD|B_2X656CLK|B_SYNCEMB|B_INDDR);

    switch(InputMode & F_Tx_MODE_CLRMOD_MASK)
    {
        case F_Tx_MODE_YUV422:
            d |=B_IN_YUV422;
            break;
        case F_Tx_MODE_YUV444:
            d |=B_IN_YUV444;
            break;
        case F_Tx_MODE_RGB24:
        default:
            d |=B_IN_RGB;
            break;
    }

    if(VideoInputType & T_MODE_CCIR656)
    {
        d |=B_2X656CLK;
    }

    if(VideoInputType & T_MODE_SYNCEMB)
    {
        d |=B_SYNCEMB;
    }

    if(VideoInputType & T_MODE_INDDR)
    {
        d |=B_INDDR;
    }

    HDMITX_WriteI2C_Byte(REG_TX_INPUT_MODE,d);
}


/*=============================================================================
   Function: SetCSCScale
   Parameter: bInputMode -
               D[1:0] - Color Mode
               D[4] - Colorimetry 0: ITU_BT601 1: ITU_BT709
               D[5] - Quantization 0: 0_255 1: 16_235
               D[6] - Up/Dn Filter 'Required'
                      0: no up/down filter
                      1: enable up/down filter when csc need.
               D[7] - Dither Filter 'Required'
                      0: no dither enabled.
                      1: enable dither and dither free go "when required".
              bOutputMode -
               D[1:0] - Color mode.
   Remark: reg72~reg8D will be programmed depended the input with table.

//=============================================================================*/
void HDMITXITEFULL_CODE_SECTION(SetCSCScale) (BYTE bInputMode,BYTE bOutputMode)
{
    BYTE ucData,csc;
    BYTE filter=0;// filter is for Video CTRL DN_FREE_GO,EN_DITHER,and ENUDFILT
    // (1) YUV422 in,RGB/YUV444 output (Output is 8-bit,input is 12-bit)
    // (2) YUV444/422  in,RGB output (CSC enable,and output is not YUV422)
    // (3) RGB in,YUV444 output   (CSC enable,and output is not YUV422)
    //
    // YUV444/RGB24 <-> YUV422 need set up/down filter.
    HDMITX_DEBUG_PRINT(("SetCSCScale(%X,%X)\n",(int)bInputMode,(int)bOutputMode));
    switch(bInputMode&F_MODE_CLRMOD_MASK)
    {
//#ifdef SUPPORT_INPUTYUV444
    case F_MODE_YUV444:

        HDMITX_DEBUG_PRINT(("Input mode is YUV444\n"));
        switch(bOutputMode&F_MODE_CLRMOD_MASK)
        {
        case F_MODE_YUV444:
            HDMITX_DEBUG_PRINT(("Output mode is YUV444\n"));
            csc=B_CSC_BYPASS;
            break;

        case F_MODE_YUV422:
            HDMITX_DEBUG_PRINT(("Output mode is YUV422\n"));
            if(bInputMode & F_MODE_EN_UDFILT) // YUV444 to YUV422 need up/down filter for processing.
            {
                filter |=B_TX_EN_UDFILTER;
            }
            csc=B_CSC_BYPASS;
            break;
        case F_MODE_RGB444:
            HDMITX_DEBUG_PRINT(("Output mode is RGB24\n"));
            csc=B_CSC_YUV2RGB;
            if(bInputMode & F_MODE_EN_DITHER) // YUV444 to RGB24 need dither
            {
                filter |=B_TX_EN_DITHER | B_TX_DNFREE_GO;
            }

            break;
        }
        break;
//#endif

//#ifdef SUPPORT_INPUTYUV422
    case F_MODE_YUV422:
        HDMITX_DEBUG_PRINT(("Input mode is YUV422\n"));
        switch(bOutputMode&F_MODE_CLRMOD_MASK)
        {
        case F_MODE_YUV444:
            HDMITX_DEBUG_PRINT(("Output mode is YUV444\n"));
            csc=B_CSC_BYPASS;
            if(bInputMode & F_MODE_EN_UDFILT) // YUV422 to YUV444 need up filter
            {
                filter |=B_TX_EN_UDFILTER;
            }

            if(bInputMode & F_MODE_EN_DITHER) // YUV422 to YUV444 need dither
            {
                filter |=B_TX_EN_DITHER | B_TX_DNFREE_GO;
            }
            break;

        case F_MODE_YUV422:
            HDMITX_DEBUG_PRINT(("Output mode is YUV422\n"));
            csc=B_CSC_BYPASS;
            break;

        case F_MODE_RGB444:
            HDMITX_DEBUG_PRINT(("Output mode is RGB24\n"));
            csc=B_CSC_YUV2RGB;
            if(bInputMode & F_MODE_EN_UDFILT) // YUV422 to RGB24 need up/dn filter.
            {
                filter |=B_TX_EN_UDFILTER;
            }

            if(bInputMode & F_MODE_EN_DITHER) // YUV422 to RGB24 need dither
            {
                filter |=B_TX_EN_DITHER | B_TX_DNFREE_GO;
            }

            break;
        }
        break;
//#endif

//#ifdef SUPPORT_INPUTRGB
    case F_MODE_RGB444:
        HDMITX_DEBUG_PRINT(("Input mode is RGB24\n"));
        switch(bOutputMode&F_MODE_CLRMOD_MASK)
        {
        case F_MODE_YUV444:
            HDMITX_DEBUG_PRINT(("Output mode is YUV444\n"));
            csc=B_CSC_RGB2YUV;
            if(bInputMode & F_MODE_EN_DITHER) // RGB24 to YUV444 need dither
            {
                filter |=B_TX_EN_DITHER | B_TX_DNFREE_GO;
            }
            break;

        case F_MODE_YUV422:
            HDMITX_DEBUG_PRINT(("Output mode is YUV422\n"));
            if(bInputMode & F_MODE_EN_UDFILT) // RGB24 to YUV422 need down filter.
            {
                filter |=B_TX_EN_UDFILTER;
            }

            if(bInputMode & F_MODE_EN_DITHER) // RGB24 to YUV422 need dither
            {
                filter |=B_TX_EN_DITHER | B_TX_DNFREE_GO;
            }
            csc=B_CSC_RGB2YUV;
            break;

        case F_MODE_RGB444:
            HDMITX_DEBUG_PRINT(("Output mode is RGB24\n"));
            csc=B_CSC_BYPASS;
            break;
        }
        break;
//#endif
    }
//#ifdef SUPPORT_INPUTRGB
    // set the CSC metrix registers by colorimetry and quantization
    if(csc==B_CSC_RGB2YUV)
    {
        HDMITX_DEBUG_PRINT(("CSC=RGB2YUV %x\n",csc));
        switch(bInputMode&(F_MODE_ITU709|F_MODE_16_235))
        {
        case F_MODE_ITU709|F_MODE_16_235:
            HDMITX_DEBUG_PRINT(("ITU709 16-235\n"));
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_YOFF, bCSCOffset_16_235,3) ;
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_MTX11_L,bCSCMtx_RGB2YUV_ITU709_16_235,SIZEOF_CSCMTX);
            break;

        case F_MODE_ITU709|F_MODE_0_255:
            HDMITX_DEBUG_PRINT(("ITU709 0-255\n"));
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_YOFF, bCSCOffset_0_255,3) ;
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_MTX11_L,bCSCMtx_RGB2YUV_ITU709_0_255,SIZEOF_CSCMTX);
            break;

        case F_MODE_ITU601|F_MODE_16_235:
            HDMITX_DEBUG_PRINT(("ITU601 16-235\n"));
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_YOFF, bCSCOffset_16_235,3) ;
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_MTX11_L,bCSCMtx_RGB2YUV_ITU601_16_235,SIZEOF_CSCMTX);
            break;

        case F_MODE_ITU601|F_MODE_0_255:
        default:
            HDMITX_DEBUG_PRINT(("ITU601 0-255\n"));
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_YOFF, bCSCOffset_0_255,3) ;
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_MTX11_L,bCSCMtx_RGB2YUV_ITU601_0_255,SIZEOF_CSCMTX);
            break;
        }

    }
//#endif

//#ifdef SUPPORT_INPUTYUV
    if (csc==B_CSC_YUV2RGB)
    {
        HDMITX_DEBUG_PRINT(("CSC=YUV2RGB %x\n",csc));
        switch(bInputMode&(F_MODE_ITU709|F_MODE_16_235))
        {
        case F_MODE_ITU709|F_MODE_16_235:
            HDMITX_DEBUG_PRINT(("ITU709 16-235\n"));
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_YOFF, bCSCOffset_16_235,3) ;
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_MTX11_L,bCSCMtx_YUV2RGB_ITU709_16_235,SIZEOF_CSCMTX);
            break;

        case F_MODE_ITU709|F_MODE_0_255:
            HDMITX_DEBUG_PRINT(("ITU709 0-255\n"));
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_YOFF, bCSCOffset_0_255,3) ;
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_MTX11_L,bCSCMtx_YUV2RGB_ITU709_0_255,SIZEOF_CSCMTX);
            break;

        case F_MODE_ITU601|F_MODE_16_235:
            HDMITX_DEBUG_PRINT(("ITU601 16-235\n"));
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_YOFF, bCSCOffset_16_235,3) ;
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_MTX11_L,bCSCMtx_YUV2RGB_ITU601_16_235,SIZEOF_CSCMTX);
            break;

        case F_MODE_ITU601|F_MODE_0_255:
        default:
            HDMITX_DEBUG_PRINT(("ITU601 0-255\n"));
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_YOFF, bCSCOffset_0_255,3) ;
            HDMITX_WriteI2C_ByteN(REG_TX_CSC_MTX11_L,bCSCMtx_YUV2RGB_ITU601_0_255,SIZEOF_CSCMTX);
            break;
        }
    }
   // #endif

    ucData=HDMITX_ReadI2C_Byte(REG_TX_CSC_CTRL) & ~(M_CSC_SEL|B_TX_DNFREE_GO|B_TX_EN_DITHER|B_TX_EN_UDFILTER);
    ucData |=filter|csc;

    HDMITX_WriteI2C_Byte(REG_TX_CSC_CTRL,ucData);
    HDMITX_DEBUG_PRINT(("Reg[%X]=%X\n",REG_TX_CSC_CTRL,HDMITX_ReadI2C_Byte(REG_TX_CSC_CTRL)));

    // set output Up/Down Filter,Dither control

}


/*=============================================================================
   [Analog Front End]
   Function: SetupAFE
   Parameter: BOOL HighFreqMode
              FALSE - PCLK < 80Hz (for mode less than 1080p)
              TRUE  - PCLK > 80Hz (for 1080p mode or above)
   Remark: set reg62~reg65 depended on HighFreqMode
           reg61 have to be programmed at last and after video stable input.
   Side-Effect:
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(SetupAFE) (BOOL HighFreqMode)
{

    // @emily turn off reg61 before SetupAFE parameters.

	/* Reset signal for HDMI_TX_DRV */
    HDMITX_WriteI2C_Byte(REG_TX_AFE_DRV_CTRL,B_AFE_DRV_RST);/* 0x10 */

    // HDMITX_WriteI2C_Byte(REG_TX_AFE_DRV_CTRL,0x3);
    //ErrorF("SetupAFE()\n");

    //TMDS Clock < 80MHz    TMDS Clock > 80MHz
    //Reg61    0x03    0x03

    //Reg62    0x18    0x88
    //Reg63    Default    Default
    //Reg64    0x08    0x80
    //Reg65    Default    Default
    //Reg66    Default    Default
    //Reg67    Default    Default


    switch(TxDev[TX0DEV].bChipRevision)
    {
    case REV_CAT6613:
    case REV_IT6613:
        if(HighFreqMode)
        {
            HDMITX_WriteI2C_Byte(REG_TX_AFE_XP_CTRL,0x88); // reg62
            HDMITX_WriteI2C_Byte(REG_TX_AFE_ISW_CTRL,0x10); // reg63
            HDMITX_WriteI2C_Byte(REG_TX_AFE_IP_CTRL,0x84); // reg64
        }
        else
        {
            HDMITX_WriteI2C_Byte(REG_TX_AFE_XP_CTRL,0x18); // reg62
            HDMITX_WriteI2C_Byte(REG_TX_AFE_ISW_CTRL,0x10); // reg63
            HDMITX_WriteI2C_Byte(REG_TX_AFE_IP_CTRL,0x0C); // reg64
        }
        break ;
    case REV_IT6610:
        if( HighFreqMode )
        {
            HDMITX_WriteI2C_Byte(0x62,0x89) ;
            HDMITX_WriteI2C_Byte(0x63,0x01) ;
            HDMITX_WriteI2C_Byte(0x64,0x56) ;
            HDMITX_WriteI2C_Byte(0x65,0x00) ;
        }
        else
        {
            HDMITX_WriteI2C_Byte(0x62,0x19) ;
            HDMITX_WriteI2C_Byte(0x63,0x01) ;
            HDMITX_WriteI2C_Byte(0x64,0x1E) ;
            HDMITX_WriteI2C_Byte(0x65,0x00) ;
        }
        break ;
    case REV_CAT6611:
        if( HighFreqMode )
        {
            HDMITX_WriteI2C_Byte(0x62,0x88) ;
            HDMITX_WriteI2C_Byte(0x63,0x01) ;
            HDMITX_WriteI2C_Byte(0x64,0x56) ;
            HDMITX_WriteI2C_Byte(0x65,0x00) ;
        }
        else
        {
            HDMITX_WriteI2C_Byte(0x62,0x18) ;
            HDMITX_WriteI2C_Byte(0x63,0x01) ;
            HDMITX_WriteI2C_Byte(0x64,0x1E) ;
            HDMITX_WriteI2C_Byte(0x65,0x00) ;
        }
        break ;
    }

    HDMITX_AndReg_Byte(REG_TX_SW_RST,~(B_REF_RST|B_VID_RST));


}


/*=============================================================================
  Function: FireAFE
   Remark: write reg61 with 0x04
           When program reg61 with 0x04,then audio and video circuit work.
//=============================================================================*/
void HDMITXITEFULL_CODE_SECTION(FireAFE) (void)
{
    Switch_HDMITX_Bank(0);
    //HDMITX_WriteI2C_Byte(0x59,0x08);

    HDMITX_WriteI2C_Byte(REG_TX_AFE_DRV_CTRL,0x00);


    // Interrupt clear
    HDMITX_WriteI2C_Byte(REG_TX_INT_CLR0, 0);
    HDMITX_WriteI2C_Byte(REG_TX_INT_CLR1, B_VSYNC_MASK);

    // System status -make interrupt clear ACTIVE
    HDMITX_WriteI2C_Byte(REG_TX_SYS_STATUS,1);

    HDMITX_AndReg_Byte(REG_TX_INT_MASK3,~B_VIDSTABLE_MASK);

}

void HDMITXITEFULL_CODE_SECTION(DisableAFE) (void)
{
    HDMITX_WriteI2C_Byte(REG_TX_AFE_DRV_CTRL,B_AFE_DRV_RST);
}


void HDMITXITEFULL_CODE_SECTION(PwrDnTxAFE) (void)
{
    Switch_HDMITX_Bank(0);
    HDMITX_WriteI2C_Byte(REG_TX_AFE_DRV_CTRL,0x30);
    if( TxDev[TX0DEV].bChipRevision >= REV_CAT6613)
    {
        HDMITX_WriteI2C_Byte(REG_TX_AFE_XP_CTRL,0x44);
        HDMITX_WriteI2C_Byte(REG_TX_AFE_IP_CTRL,0x40);
        HDMITX_WriteI2C_Byte(REG_TX_AFE_RING,0x02);
    }
    HDMITX_OrReg_Byte(REG_TX_INT_CTRL,0x01);
    HDMITX_WriteI2C_Byte(REG_TX_AUDIO_CTRL0,0xD0);
}

void HDMITXITEFULL_CODE_SECTION(PwrOnTxAFE) (void)
{

    Switch_HDMITX_Bank(0);
    HDMITX_WriteI2C_Byte(REG_TX_AFE_DRV_CTRL,0x10);
    if(TxDev[TX0DEV].bChipRevision >= REV_CAT6613)
    {
        HDMITX_WriteI2C_Byte(REG_TX_AFE_XP_CTRL,0x18);
        HDMITX_WriteI2C_Byte(REG_TX_AFE_IP_CTRL,0x0C);
        HDMITX_WriteI2C_Byte(REG_TX_AFE_RING,0x00);
    }
    HDMITX_AndReg_Byte(REG_TX_INT_CTRL,0xFE);
}

/* Alex : Used in Rx.c  */
void HDMITXITEFULL_CODE_SECTION(SetAllTxAVMute) (void)
{
    if (TxDev[TX0DEV].TxVState == TxVStateVideoOn)
    {
        SetTxAVMute();
    }
}

/* Alex : Used in Rx.c  */
void HDMITXITEFULL_CODE_SECTION(ClrAllTxAVMute) (void)
{
    if (TxDev[TX0DEV].TxVState == TxVStateVideoOn)
    {
        ClrTxAVMute();
    }
}


/*=============================================================================
                                   Audio Output
  =============================================================================*/
/*=============================================================================
   Function: SetNCTS
   Parameter: PCLK - video clock in Hz.
              Fs - audio sample frequency in Hz
   Return: ER_SUCCESS if success
   Remark: set N value,the CTS will be auto generated by HW.
   Side-Effect: register bank will reset to bank 0.
//=============================================================================*/
void HDMITXITEFULL_CODE_SECTION(SetNCTS) (ULONG PCLK,ULONG Fs)
{
    ULONG n,MCLK;

    MCLK=Fs * 256;// MCLK=fs * 256;


    switch (Fs) {
        case AUDFS_32KHz:
            switch (PCLK) {
                case 74175000: n=11648; break;
                case 14835000: n=11648; break;
                default: n=4096;
            }
            break;
        case AUDFS_44p1KHz:
            switch (PCLK) {
                case 74175000: n=17836; break;
                case 14835000: n=8918; break;
                default: n=6272;
            }
            break;
        case AUDFS_48KHz:
            switch (PCLK) {
                case 74175000: n=11648; break;
                case 14835000: n=5824; break;
                default: n=6144;
            }
            break;
        case AUDFS_88p2KHz:
            switch (PCLK) {
                case 74175000: n=35672; break;
                case 14835000: n=17836; break;
                default: n=12544;
            }
            break;
        case AUDFS_96KHz:
            switch (PCLK) {
                case 74175000: n=23296; break;
                case 14835000: n=11648; break;
                default: n=12288;
            }
            break;
        case AUDFS_176p4KHz:
            switch (PCLK) {
                case 74175000: n=71344; break;
                case 14835000: n=35672; break;
                default: n=25088;
            }
            break;
        case AUDFS_192KHz:
            switch (PCLK) {
                case 74175000: n=46592; break;
                case 14835000: n=23296; break;
                default: n=24576;
            }
            break;
        default: n=MCLK / 2000;
    }

    Switch_HDMITX_Bank(1);



    HDMITX_WriteI2C_Byte(REGPktAudN0,(BYTE)((n)&0xFF));
    HDMITX_WriteI2C_Byte(REGPktAudN1,(BYTE)((n>>8)&0xFF));
    HDMITX_WriteI2C_Byte(REGPktAudN2,(BYTE)((n>>16)&0xF));
    Switch_HDMITX_Bank(0);

    HDMITX_WriteI2C_Byte(REG_TX_PKT_SINGLE_CTRL,0);// D[1]=0,HW auto count CTS
}


SYS_STATUS HDMITXITEFULL_CODE_SECTION(SetAudioFormat) (BYTE NumChannel, BYTE AudioEnable, BYTE bSampleFreq, BYTE AudSWL)
{
    BYTE fs = bSampleFreq;
    BYTE SWL;
    BYTE SourceValid;
    BYTE SoruceNum;

    if (NumChannel > 6)
    {
        SourceValid = B_AUD_ERR2FLAT | B_AUD_S3VALID | B_AUD_S2VALID | B_AUD_S1VALID;
        SoruceNum = 4;
    }
    else if (NumChannel > 4)
    {
        SourceValid = B_AUD_ERR2FLAT | B_AUD_S2VALID | B_AUD_S1VALID;
        SoruceNum = 3;
    }
    else if (NumChannel > 2)
    {
        SourceValid = B_AUD_ERR2FLAT | B_AUD_S1VALID;
        SoruceNum = 2;
    }
    else
    {
        SourceValid = B_AUD_ERR2FLAT;// only two channel.
        SoruceNum = 1;
    }

    AudioEnable &=~ (M_AUD_SWL | B_SPDIFTC);

    switch (AudSWL)
    {
        case 16:
            SWL = AUD_SWL_16;
            AudioEnable |= M_AUD_16BIT;
            break;

        case 18:
            SWL = AUD_SWL_18;
            AudioEnable |= M_AUD_16BIT;
            break;

        case 20:
            SWL = AUD_SWL_20;
            AudioEnable |= M_AUD_16BIT;
            break;

        case 24:
            SWL = AUD_SWL_24;
            AudioEnable |= M_AUD_16BIT;
            break;

        default:
            return ER_FAIL;
    }

    Switch_HDMITX_Bank(0);

    // Reset audio before setting audio
    HDMITX_OrReg_Byte(REG_TX_SW_RST, B_AUD_RST | B_AREF_RST);

    if (TxDev[TX0DEV].AudSrcHBR == TRUE)
    {
        #ifdef _HBR_I2S_
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_CTRL0,   0xcf);
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_CTRL1,   0x01); //
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_FIFOMAP, 0xE4); // 020609  Clive no swap
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_CTRL3,   0x00); // 061208  Clive Channel status from user define
        HDMITX_WriteI2C_Byte (0xE5,                 0x08);
        #else
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_CTRL0,   0xD1);
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_CTRL1,   0x01); // regE1 bOutputAudioMode should be loaded from ROM image.
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_FIFOMAP, 0x00);
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_CTRL3,   0x00); //copy channel status from spdif
        HDMITX_WriteI2C_Byte (0xE5,                 0x08);
        #endif
    }
    else if (TxDev[TX0DEV].AudSrcDSD == TRUE)
    {
       // Does not support dsd audio yet
    }
    else
    {
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_CTRL0,   AudioEnable);
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_CTRL1,   (B_AUDFMT_32BIT_I2S));  // Default 32bit I2S bus.
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_FIFOMAP, 0xE4 );                 // No fifo mapping
        HDMITX_WriteI2C_Byte (REG_TX_AUDIO_CTRL3,   0x00);                  // Channel status by user define
        HDMITX_WriteI2C_Byte (0xE5,                 0x00);
    }

    HDMITX_WriteI2C_Byte (REG_TX_AUD_SRCVALID_FLAT,   SourceValid);
    HDMITX_AndReg_Byte   (REG_TX_SW_RST,            ~(B_AREF_RST|B_AUD_RST));

    return ER_SUCCESS;
}


/*=============================================================================
   Function: InitAudioMode
   Remark: For evaluate.
       The function is for developing testing environment,
       set the value into the audio functions.
       For production,the function have to modify for actual status.
       Global variables TxAudioChannelNum and bTxAudioSampleFreq will affect the setting.

       1. Call SetNCTS() to set the N/CTS value by PCLK and AudioSampleClock.
       2. Call SetAudioFormat to set audio channel registers with sample width 24bits.
       3. Call configure AudioInfoFrm (for evaluating too.)
       4. enable Audio with set Reg04[2]=0
  =============================================================================*/
BOOL HDMITXITEFULL_CODE_SECTION(EnableAudioOutput) (BYTE bAudioSampleFreq, BYTE ChannelNumber, BYTE bAudSWL, BYTE bSPDIF)
{
    BYTE bAudioChannelEnable;
    ULONG N;

    HDMITX_DEBUG_PRINT(("EnableAudioOutput(0x%02X,0x%02X,0x%02X,0x%02X)\n",(int)bAudioSampleFreq,(int)ChannelNumber,(int)bAudSWL,(int)bSPDIF));

    switch(ChannelNumber)
    {
    case 7:
    case 8:
        bAudioChannelEnable=0xF;
        break;
    case 6:
    case 5:
        bAudioChannelEnable=0x7;
        break;
    case 4:
    case 3:
        bAudioChannelEnable=0x3;
        break;
    case 2:
    case 1:
    default:
        bAudioChannelEnable=0x1;
        break;
    }

    if(bSPDIF) bAudioChannelEnable |=B_AUD_SPDIF;

    switch(bAudioSampleFreq)
    {
        case AUDFS_32KHz:    N=4096;  break;
        case AUDFS_44p1KHz:  N=6272;  break;
        case AUDFS_48KHz:    N=6144;  break;
        case AUDFS_88p2KHz:  N=12544; break;
        case AUDFS_96KHz:    N=12288; break;
        case AUDFS_176p4KHz: N=25088; break;
        case AUDFS_192KHz:   N=24576; break;
        case AUDFS_768KHz:
        {
            if(TxDev[TX0DEV].AudSrcHBR == TRUE)
            {
                N=0x6000;
            }
            break;
        }
        default:
			N=6144;
		    if(TxDev[TX0DEV].AudSrcHBR == TRUE)
            {
                N=0x6000;
            }
			break;
		}

        Switch_HDMITX_Bank(1);

        HDMITX_WriteI2C_Byte(REGPktAudN0,(BYTE)((N)&0xFF));
        HDMITX_WriteI2C_Byte(REGPktAudN1,(BYTE)((N>>8)&0xFF));
        HDMITX_WriteI2C_Byte(REGPktAudN2,(BYTE)((N>>16)&0xF));


        Switch_HDMITX_Bank(0);
        HDMITX_WriteI2C_Byte(REG_TX_PKT_SINGLE_CTRL,0);// D[1]=0,HW auto count CTS

        SetAudioFormat(ChannelNumber ,bAudioChannelEnable  ,bAudioSampleFreq   ,bAudSWL	) ;

#ifdef DEBUG
    DumpCatHDMITXReg(cInstance);
#endif // DEBUG
    return TRUE;
}

SYS_STATUS HDMITXITEFULL_CODE_SECTION(InitAudioMode) (void)
{
    BYTE fs,bValidCh,ChannelNumber;

    if(TxDev[TX0DEV].bTxHDMIMode==TRUE)
    {

		fs = TxDev[TX0DEV].bOutAudSampleFrequence;
		bValidCh = TxDev[TX0DEV].bOutAudChannelSrc;


        if( bValidCh & B_AUDIO_SRC_VALID_3 )
        {
            ChannelNumber = 8 ;
        }
        else if( bValidCh & B_AUDIO_SRC_VALID_2 )
        {
            ChannelNumber = 6 ;
        }
        else if( bValidCh & B_AUDIO_SRC_VALID_1 )
        {
            ChannelNumber = 4 ;
        }
        else
        {
            ChannelNumber = 2 ;
        }

        #ifdef _enable_DownSample__
        switch(fs)
        {
            case AUDFS_88p2KHz:
            {
                HDMITX_WriteI2C_Byte(REG_TX_CLK_CTRL1,B_AUD_DIV2);
                fs=AUDFS_44p1KHz;
                break;
            }
            case AUDFS_96KHz:
            {
                HDMITX_WriteI2C_Byte(REG_TX_CLK_CTRL1,B_AUD_DIV2);
                fs=AUDFS_48KHz;
                break;
            }
            case AUDFS_176p4KHz:
            {
                HDMITX_WriteI2C_Byte(REG_TX_CLK_CTRL1,B_AUD_DIV4);
                fs=AUDFS_44p1KHz;
                break;
            }
            case AUDFS_192KHz:
            {
                HDMITX_WriteI2C_Byte(REG_TX_CLK_CTRL1,B_AUD_DIV4);
                fs=AUDFS_48KHz;
            }
                break;
            default:
                HDMITX_WriteI2C_Byte(REG_TX_CLK_CTRL1,0x0);
        }
        #endif

        EnableAudioOutput (fs,ChannelNumber, 24,TxDev[TX0DEV].bTxAudSel);
        //HDMITX_DEBUG_PRINT3(("Initialize Audio with fs = %d\n", fs));

       /* if (routeOnI2S==1)
        {
        	if (invalid_audio==0)
        	{
        		unsigned int AnalogMode=0;
        		g_sys_debug.LastPCMSamplingRate=fs;
        		if (HdmiDriverInfo.IndependentTx==ModeTxIndependantAnalog)
        			AnalogMode=1;
        		AudioStart(10, fs, AnalogMode);
        	}
        	else
        	{
        		//If we have invalid_audio then we should stop audio
        		AudioStop();
        	}
        }
        else  TODO remove this*/
        {
        	//AudioStop(); add this when audio will be implemented
        }
    }
    else
    {

    }
    return ER_SUCCESS;
}


/*=============================================================================
   Packet and InfoFrame
  =============================================================================*/

/*=============================================================================
   Function: SetTxAVMute()
   Remark: set AVMute as TRUE and enable GCP sending.
   Alex : Used in Rx.c
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(SetTxAVMute) (void)
{

    BYTE uc;
    Switch_HDMITX_Bank(0);
    uc=HDMITX_ReadI2C_Byte(REG_TX_GCP);
    uc &=~B_TX_SETAVMUTE;
    //uc |=bEnable?B_TX_SETAVMUTE:0;
    uc |=B_TX_SETAVMUTE;
    HDMITX_WriteI2C_Byte(REG_TX_GCP,uc);
    HDMITX_WriteI2C_Byte(REG_TX_PKT_GENERAL_CTRL,B_ENABLE_PKT|B_REPEAT_PKT);
    HDMITX_DEBUG_PRINT(("SetTxAVMute()\n")) ;
}

/*=============================================================================
   Function: ClrTxAVMute()
   Remark: clear AVMute as TRUE and enable GCP sending.
   Alex : Used in Rx.c
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(ClrTxAVMute) (void)
{

    BYTE uc;
    Switch_HDMITX_Bank(0);
    uc=HDMITX_ReadI2C_Byte(REG_TX_GCP);
    uc &=~B_TX_SETAVMUTE;
    //uc |=bEnable?B_TX_SETAVMUTE:0;
    HDMITX_WriteI2C_Byte(REG_TX_GCP,uc);
    HDMITX_WriteI2C_Byte(REG_TX_PKT_GENERAL_CTRL,B_ENABLE_PKT|B_REPEAT_PKT);
    HDMITX_DEBUG_PRINT(("ClrTxAVMute()\n")) ;
}



/*=============================================================================
   Function: SetAVIInfoFrame()
   Parameter: pAVIInfoFrame - the pointer to HDMI AVI Infoframe d
   Remark: Fill the AVI InfoFrame d,and count checksum,then fill into
           AVI InfoFrame registers. TODO:use this and rename it according
  =============================================================================*/
SYS_STATUS HDMITXITEFULL_CODE_SECTION(SetAVIInfoFrame) (AviInfoFrameType *pAVIInfoFrame)
{
    BYTE i;
    BYTE d;

    if (!pAVIInfoFrame)
    {
        return ER_FAIL;
    }

    Switch_HDMITX_Bank(1);

    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB1,  pAVIInfoFrame->PacketByte.AVI_DB[0]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB2,  pAVIInfoFrame->PacketByte.AVI_DB[1]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB3,  pAVIInfoFrame->PacketByte.AVI_DB[2]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB4,  pAVIInfoFrame->PacketByte.AVI_DB[3]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB5,  pAVIInfoFrame->PacketByte.AVI_DB[4]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB6,  pAVIInfoFrame->PacketByte.AVI_DB[5]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB7,  pAVIInfoFrame->PacketByte.AVI_DB[6]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB8,  pAVIInfoFrame->PacketByte.AVI_DB[7]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB9,  pAVIInfoFrame->PacketByte.AVI_DB[8]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB10, pAVIInfoFrame->PacketByte.AVI_DB[9]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB11, pAVIInfoFrame->PacketByte.AVI_DB[10]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB12, pAVIInfoFrame->PacketByte.AVI_DB[11]);
    HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB13, pAVIInfoFrame->PacketByte.AVI_DB[12]);

    HDMITX_DEBUG_PRINT(("SetAVIInfoFrame() "));

    for (i = 0, d = 0; i < 13; i++)
    {
        HDMITX_DEBUG_PRINT((" %02X",(int)pAVIInfoFrame->PacketByte.AVI_DB[i]));
        d -= pAVIInfoFrame->PacketByte.AVI_DB[i];
    }

    HDMITX_DEBUG_PRINT(("\n"));

    d -= AVI_INFOFRAME_VER + AVI_INFOFRAME_TYPE + AVI_INFOFRAME_LEN;

    HDMITX_WriteI2C_Byte(REG_TX_AVIINFO_SUM, d);

    Switch_HDMITX_Bank(0);
    ENABLE_AVI_INFOFRM_PKT();
    return ER_SUCCESS;
}

void HDMITXITEFULL_CODE_SECTION(MdkSetAVIInfoFrame) (/*AviInfoFrameType *pAVIInfoFrame*/)
{

	unsigned char pixelrep = 0;
	Switch_HDMITX_Bank(1);

	HDMITX_WriteI2C_Byte(REG_TX_AVIINFO_DB1, 0x10);
	HDMITX_WriteI2C_Byte(REG_TX_AVIINFO_DB2, (8 | (2 << 4) | (2 << 6)));
	HDMITX_WriteI2C_Byte(REG_TX_AVIINFO_DB3, 0);
	HDMITX_WriteI2C_Byte(REG_TX_AVIINFO_DB4, VIC);
	HDMITX_WriteI2C_Byte(REG_TX_AVIINFO_DB5, (pixelrep & 3));
	HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB6,  0);
	HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB7,  0);
	HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB8,  0);
	HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB9,  0);
	HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB10, 0);
	HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB11, 0);
	HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB12, 0);
	HDMITX_WriteI2C_Byte (REG_TX_AVIINFO_DB13, 0);

   /* HDMITX_DEBUG_PRINT(("SetAVIInfoFrame() "));

    for (i = 0, d = 0; i < 13; i++)
    {
        //HDMITX_DEBUG_PRINT((" %02X",(int)pAVIInfoFrame->PacketByte.AVI_DB[i]));
        d -= pAVIInfoFrame->PacketByte.AVI_DB[i];
    }

    //HDMITX_DEBUG_PRINT(("\n"));*/

    u32 d = 0 - (0x10 + (8 | (2 << 4) | (2 << 6)) + VIC + (pixelrep & 3) + 0x82 + 2 + 0x0D);

    HDMITX_WriteI2C_Byte(REG_TX_AVIINFO_SUM, d);

    Switch_HDMITX_Bank(0);
    HDMITX_WriteI2C_Byte(REG_TX_AVI_INFOFRM_CTRL, B_ENABLE_PKT | B_REPEAT_PKT);

    return;
}

/*=============================================================================
   Function: SetAudioInfoFrame()
   Parameter: pAudioInfoFrame - the pointer to HDMI Audio Infoframe d
   Remark: Fill the Audio InfoFrame d,and count checksum,then fill into
           Audio InfoFrame registers.
  =============================================================================*/
SYS_STATUS HDMITXITEFULL_CODE_SECTION(SetAudioInfoFrame) (AudioInfoFrameType *pAudioInfoFrame, unsigned int forceVersion)
{
    BYTE i;
    BYTE d;

    if (!pAudioInfoFrame)
    {
        return ER_FAIL;
    }

    pAudioInfoFrame->PacketByte.AUD_HB[0] = AUDIO_INFOFRAME_TYPE; // AUDIO InfoFrame
    // (Alex): HACK !!! - In independent Tx mode we read 0 from Rx so hardcodinf version to 1 always

    if (forceVersion)
    {
        pAudioInfoFrame->PacketByte.AUD_HB[1] = 0x01;
    }
    else
    {
        //TODO: how should I change this?
    	// pAudioInfoFrame->PacketByte.AUD_HB[1] = HDMIRX_ReadI2C_Byte(REG_RX_AUDIO_VER); // CB:  yes, this is an Rx register!
    }

	pAudioInfoFrame->PacketByte.AUD_HB[2] = AUDIO_INFOFRAME_LEN;

    Switch_HDMITX_Bank(1);

#ifdef _PRINT_HDMI_TX_
    HDMITX_DEBUG_PRINT(("pAudioInfoFrame->pktbyte.AUD_DB[0]=%X\n",(int)pAudioInfoFrame->pktbyte.AUD_DB[0]));
//    HDMITX_DEBUG_PRINT((" ***   audio channelcount==%X  *** \n"(int),pAudioInfoFrame->info.AudioChannelCount));
#endif

    HDMITX_WriteI2C_Byte (REG_TX_PKT_AUDINFO_CC,     pAudioInfoFrame->PacketByte.AUD_DB[0]);
    HDMITX_WriteI2C_Byte (REG_TX_PKT_AUDINFO_SF,     pAudioInfoFrame->PacketByte.AUD_DB[1]);
    HDMITX_WriteI2C_Byte (REG_TX_PKT_AUDINFO_CA,     pAudioInfoFrame->PacketByte.AUD_DB[3]);
    HDMITX_WriteI2C_Byte (REG_TX_PKT_AUDINFO_DM_LSV, pAudioInfoFrame->PacketByte.AUD_DB[4]);

#ifdef _PRINT_HDMI_TX_
    HDMITX_DEBUG_PRINT(("SECOND pAudioInfoFrame->pktbyte.AUD_DB[0]=%X\n",(int)pAudioInfoFrame->pktbyte.AUD_DB[0]));
#endif

	for(i = 0, d = 0; i < 5; i++)
    {
        d -= pAudioInfoFrame->PacketByte.AUD_DB[i];
    }

    d -= pAudioInfoFrame->PacketByte.AUD_HB[0] + pAudioInfoFrame->PacketByte.AUD_HB[1] + pAudioInfoFrame->PacketByte.AUD_HB[2];
    //printf ("\n\nAudInf     Sum : %X \n\n", d);

    HDMITX_WriteI2C_Byte(REG_TX_PKT_AUDINFO_SUM,d);

    Switch_HDMITX_Bank(0);
    ENABLE_AUD_INFOFRM_PKT();

    return ER_SUCCESS;

}


SYS_STATUS HDMITXITEFULL_CODE_SECTION(CopyVendorSpecificInfoFrame) (unsigned int TxDriverMode)
{
    int i;
    BYTE c;

    if (TxDriverMode != ModeFullRepeater)
    {
        // Alex : In any other mode except for the Full Repeater Scenario we do not forward the VSI
    	//Cristi Olar: In the rest of cases we fill a safe 2D VSI structure
        Switch_HDMITX_Bank(1);

        HDMITX_WriteI2C_Byte(REG_TX_PKT_HB00, TxVSIheader[0]);
        HDMITX_WriteI2C_Byte(REG_TX_PKT_HB01, TxVSIheader[1]);
        HDMITX_WriteI2C_Byte(REG_TX_PKT_HB02, TxVSIheader[2]);
        for (i = 0; i < 28; i++)
        {
            HDMITX_WriteI2C_Byte    (i + REG_TX_PKT_PB00, TxVSIinfo[i]);
        }
        Switch_HDMITX_Bank(0);
        ENABLE_NULL_PKT();
        return ER_FAIL;
    }

   /* if (!RxUpHDMIMode) TODO:use this?
    {
        DISABLE_NULL_PKT();
        return ER_FAIL;
    }*/

    Switch_HDMITX_Bank(1);

    HDMITX_WriteI2C_Byte(0x38, c);
    for (i = 1; i < (3 + 28); i++)
    {
       // c = HDMIRX_ReadI2C_Byte (i + REG_RX_GENPKT_HB0); TODO ???
       // HDMITX_WriteI2C_Byte    (i + REG_TX_PKT_HB00, c);
    }

    Switch_HDMITX_Bank(0);
    ENABLE_NULL_PKT();

    return ER_SUCCESS;
}
unsigned int TxVideoState = 0;
void HDMITXITEFULL_CODE_SECTION(Tx_SwitchVideoState) (TxVideoStateType state, unsigned int TxDriverMode)
{
    TxVideoState = state;
    HdmiDriverInfo.TxVideoState = state;
    //BoardDisableLcdDataLines();

    if (TxDev[TX0DEV].TxVState != state)
    {
        if (TxDev[TX0DEV].TxVState == TxVStateUnplug)
        {
            if ((state != TxVStateUnplug) && (state != TxVStateHPD))
            {
                return;
            }
        }

        HDMITX_DEBUG_VIDEO_STATES_PRINT(("Switch Video State (%d --> %d)\n", (int)TxDev[TX0DEV].TxVState, (int)state));
        HDMITX_DEBUG_VIDEO_STATES_PRINT(("Driver Mode : %d\n", TxDriverMode));

        TxDev[TX0DEV].TxVState = state;

        if (TxDev[TX0DEV].TxVState != TxVStateVideoOn)
        {
            TX_SwitchAudioState(TxAStateOff);
            // HDMITX_DEBUG_VIDEO_STATES_PRINT(("TxDev[TX0DEV].TxVState !=TxVStateVideoOn\n"));
        }

        switch (state)
        {
            case TxVStateUnplug:
                HDMITX_DEBUG_VIDEO_STATES_PRINT(("Unplug\n\n"));
                TxDev[TX0DEV].TxVStateCounter = TX_UNPLUG_TIMEOUT;
                TxDev[TX0DEV].bDnEDIDRdy = FALSE ;
                HDMITX_WriteI2C_Byte (REG_TX_SW_RST, B_AREF_RST | B_VID_RST | B_AUD_RST | B_HDCP_RST);

                //Adjusting_DDC_Driving();
                break;

            case TxVStateHPD:
                HDMITX_DEBUG_VIDEO_STATES_PRINT(("HPD\n\n"));
                break;

            case TxVStateWaitForVStable:
                HDMITX_DEBUG_VIDEO_STATES_PRINT(("WaitForVStable\n\n"));
                TxDev[TX0DEV].TxVStateCounter = TX_WAITVIDSTBLE_TIMEOUT;
                TxDev[TX0DEV].TxStableCnt = 0;
                break;

            case TxVStateVideoInit:
                HDMITX_DEBUG_VIDEO_STATES_PRINT(("VideoInit\n\n"));
                Tx_SwitchHDCPState(TxHDCP_Off);
                TxDev[TX0DEV].bTxVidReset = FALSE;
                break;

            case TxVStateVideoSetup:
                HDMITX_DEBUG_VIDEO_STATES_PRINT(("VideoSetup\n\n"));
                break;

            case TxVStateVideoOn:
                HDMITX_DEBUG_VIDEO_STATES_PRINT(("VideoOn\n\n"));

                Tx_SwitchHDCPState (TxHDCP_Off);
                #ifdef DEBUG_TIMER
                TxVideoOnTickCount = ucTickCount;
                #endif

            	//VideoOnTime=DRV_TIMER_free_timer_val();todo: change this
                TxDev[TX0DEV].TxVStateCounter = 200;
                break;
        }
    }
}


void HDMITXITEFULL_CODE_SECTION(TxVideoHandler) (unsigned int TxDriverMode)
{
    static unsigned int lcdDataLinesOn = 1;
    BYTE intclr;
    BYTE sys_stat;

    Switch_HDMITX_Bank(0);

    sys_stat = HDMITX_ReadI2C_Byte (REG_TX_SYS_STATUS);

    if (TxDev[TX0DEV].TxVState != TxVStateUnplug)
    {
        if ((sys_stat & (B_HPDETECT)) != (B_HPDETECT))
        {
            Tx_SwitchVideoState (TxVStateUnplug, HdmiDriverInfo.IndependentTx);
        }
    }

    switch (TxDev[TX0DEV].TxVState)
    {
        case TxVStateReset:
            HDMITX_DEBUG_PRINT(("RxVideoState = ResetTx \n"));
            if(!IsTxSwRst())
            {
                RstTxDev();
            }
            break;

        case TxVStateUnplug:
            //max7088 20080923 for Allion HDCP test,disable TurnOn_HDMIRX
        	if (sys_stat & B_HPDETECT)
        	{
        		Tx_SwitchVideoState(TxVStateHPD, HdmiDriverInfo.IndependentTx);
        	}
        	break;

        case TxVStateHPD:
            TxDev[TX0DEV].bEDIDChange = TRUE;
            if (TxDev[TX0DEV].bDnEDIDRdy == FALSE)
            {
                //Safe to reread TX EDID
                //ReadTxEdidFlag=1;  TODO: add the basic functionalities to read the TV EDID
               // if (EdidReadTx() == ER_FAIL) TODO:implement this when needed
                {
//                    printf("Falsing\n");
//                    TxDev[TX0DEV].bDnHDMIMode   = FALSE;
//                    TxDev[TX0DEV].bDnBasicAudEn = FALSE;
                	//If we got here we had an TX EDID read fail
                	//But we're not giving up! We just loaded our default EDID for sources and we
                	//are now going on
                }
            }

            InitTxInt();
            Tx_SwitchVideoState(TxVStateVideoInit, HdmiDriverInfo.IndependentTx);
            break;

        case TxVStateVideoInit:
            if (TxDev[TX0DEV].bTxVidReset == FALSE)
            {
                InitTxInt();
                SetTxAVMute();
                DisableAFE();
                PwrDnTxAFE();

                HDMITX_WriteI2C_Byte (REG_TX_SW_RST, B_VID_RST | B_AUD_RST | B_AREF_RST | B_HDCP_RST);
                TxDev[TX0DEV].bTxVidReset=TRUE;
            }

            if ((HdmiDriverInfo.IndependentTx == ModeTxIndependantVideoOnly) || (HdmiDriverInfo.IndependentTx == ModeTxIndependantAnalog))
            {
                #ifdef DEBUG_TIMER
                TxVideoInitCount1 = ucTickCount ;
                #endif

                PwrOnTxAFE();
                Tx_SwitchVideoState(TxVStateVideoSetup, HdmiDriverInfo.IndependentTx);
            }
            else;
                break;

        case TxVStateVideoSetup:
            #ifdef DEBUG_TIMER
            TxVideoSetupCount2 = ucTickCount;
            #endif
            TxDev[TX0DEV].bTxVidReady = FALSE;
            InitVideoMode();

            //TODO: Carefully tweak this delay here. It was enough to make FP
            //work for Sony Bluray player but it may be required to increase
            //for other devices TODO:should we remove it?
            SetupTxVideo(HdmiDriverInfo.IndependentTx);


            HDMITX_OrReg_Byte (REG_TX_SW_RST, B_VID_RST);
            HDMITX_WriteI2C_Byte (REG_TX_SW_RST, B_AUD_RST | B_AREF_RST | B_HDCP_RST);
            HDMITX_WriteI2C_Byte (REG_TX_SW_RST, B_VID_RST | B_AUD_RST  | B_AREF_RST | B_HDCP_RST);
            HDMITX_WriteI2C_Byte (REG_TX_SW_RST, B_AUD_RST | B_AREF_RST | B_HDCP_RST);

            Tx_SwitchVideoState (TxVStateWaitForVStable, HdmiDriverInfo.IndependentTx);
            #ifdef DEBUG_TIMER
            TxWaitForVStableCount3 = ucTickCount;
            #endif

        case TxVStateWaitForVStable:
            if ((HDMITX_ReadI2C_Byte(REG_TX_SYS_STATUS) & B_TXVIDSTABLE))
            {
                FireAFE();
                TxDev[TX0DEV].bUpExitVideoOn = FALSE;
                ClrTxAVMute(); //max7088 move to fireAFE          SetTXColordepth();//mingchih add

                //  Clear HPD plug and RX sense interrupt   20080306 Clive add fix tx dead lock
                HDMITX_OrReg_Byte (REG_TX_INT_CLR0, (B_INT_HPD_PLUG));

                intclr |= (HDMITX_ReadI2C_Byte(REG_TX_SYS_STATUS) & ~B_CLR_AUD_CTS) | B_INTACTDONE;
                HDMITX_WriteI2C_Byte(REG_TX_SYS_STATUS, intclr);// clear interrupt.

                Tx_SwitchVideoState(TxVStateVideoOn, HdmiDriverInfo.IndependentTx);
            }
            else
            {
                //HDMITX_DEBUG_PRINT(("did not stable ...\n")) ;
            }

            break;

        case TxVStateVideoOn:
            // Alex : Only Turn ON the LCD data lines where all is stable
            /*if (BoardGetLcdDataLinesOff() == 0)
            {
                if (lcdDataLinesOn == 0)
                {
                    BoardEnableLcdDataLines();
                    lcdDataLinesOn = 1;
                }
            } TODO:should we use this?
            else*/
            {
                lcdDataLinesOn = 0;
            }


/*          if (BoardGetLcdDataLinesStatus() == 50)
            {
                RememberModeFlag = TRUE;
            }*/

            if ((TxDriverMode == ModeTxIndependantAnalog) || (TxDriverMode == ModeTxIndependantVideoOnly))
            {
                if (TxDev[TX0DEV].bTxAuthenticated)
                {
                    if (TxDev[TX0DEV].bUpExitVideoOn == TRUE)
                    {
                        SetTxAVMute();
                        Tx_SwitchVideoState (TxVStateVideoInit, HdmiDriverInfo.IndependentTx);
                        break;
                    }
                }

                if (TxDev[TX0DEV].TxVStateCounter == 0)
                {
                    BYTE uc;
                    Switch_HDMITX_Bank(0);
                    uc = HDMITX_ReadI2C_Byte(REG_TX_GCP);

                    if (!(uc & B_TX_SETAVMUTE))
                    {
                        SetTxAVMute();
                    }

                    if (TxDev[TX0DEV].bChipRevision > REV_CAT6613)
                    {
                        if (CopyVendorSpecificInfoFrame(HdmiDriverInfo.IndependentTx) == ER_SUCCESS)
                        {
                            TxDev[TX0DEV].TxVStateCounter  = 1000;
                            //HDMITX_DEBUG_PRINT(("Copied VSI Infoframe.\n")) ;
                        }
                        else
                        {
                            TxDev[TX0DEV].TxVStateCounter  = 500;
                            //HDMITX_DEBUG_PRINT(("Did not copy VSI Infoframe.\n")) ;
                        }
                    }
                }
            }
            break;
    }
}


void HDMITXITEFULL_CODE_SECTION(SwReset)()
{
	HDMITX_OrReg_Byte (REG_TX_SW_RST, B_VID_RST);
    HDMITX_WriteI2C_Byte (REG_TX_SW_RST, B_AUD_RST | B_AREF_RST | B_HDCP_RST);
    HDMITX_WriteI2C_Byte (REG_TX_SW_RST, B_VID_RST | B_AUD_RST  | B_AREF_RST | B_HDCP_RST);
    HDMITX_WriteI2C_Byte (REG_TX_SW_RST, B_AUD_RST | B_AREF_RST | B_HDCP_RST);
}

void HDMITXITEFULL_CODE_SECTION(ResetTxVideo) (void)
{
    // Alex: IMPORTANT
    //HdmiStopSabreVideo(); TODO: implement this when needed
    Tx_SwitchVideoState(TxVStateVideoInit, HdmiDriverInfo.IndependentTx);
}


void HDMITXITEFULL_CODE_SECTION(TX_SwitchAudioState) (TxAudioStateType state)
{
    if (state != TxDev[TX0DEV].TxAState)
    {
        HDMITX_DEBUG_PRINT(("TxAState=%bX ",TxDev[TX0DEV].TxAState));

        if (TxDev[TX0DEV].TxAState != state)
        {
            HdmiDriverInfo.TxAudioState = state;

            switch (state)
            {
                case TxAStateOff:
                    Switch_HDMITX_Bank(0);
                    DISABLE_AUD_INFOFRM_PKT();
                    HDMITX_OrReg_Byte(REG_TX_SW_RST,    B_AUD_RST|B_AREF_RST);
                    break;

                case TxAStateCheck:
                    //SetupTxAud(); TODO: add this when audio will be implemented
                    InitAudioMode();
                    TxDev[TX0DEV].TxAStateCounter = 3;
                    break;

                case TxAStateOn:
                    //HDMITX_WriteI2C_Byte(REG_TX_AUDIO_CTRL0,0xC1);
                    ENABLE_AUD_INFOFRM_PKT();
                    TxDev[TX0DEV].bSrcAudChange = FALSE;
                    break;
            }

            TxDev[TX0DEV].TxAState = state;
        }

        HDMITX_DEBUG_PRINT(("=-=->%bX\n", TxDev[TX0DEV].TxAState));
    }
}


int HDMITXITEFULL_CODE_SECTION(isSPDIFSwitchConditionMet)(void)
{
    u32 status;

    //Switch bank to bank 1. Reg 0x0F is the second SYSTEM register of the TX chip
    Switch_HDMITX_Bank(1);

    //0x168 or 0x68 in bank 1 is the REGPktAudInfoCC register
    //For non PCM bits [2:0] are 0
    status=HDMITX_ReadI2C_Byte(0x68);

    //Now that we have our answer, switch bank to bank 0 again.
    Switch_HDMITX_Bank(0);
    if ( (status & 0x07) == 0 )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


void HDMITXITEFULL_CODE_SECTION(TX_AudioHandler) (void)
{
    if (TxDev[TX0DEV].TxVState != TxVStateVideoOn)
    {
        if (TxDev[TX0DEV].TxAState != TxAStateOff)
        {
            TX_SwitchAudioState(TxAStateOff);
        }
        return;
    }
    //Check the audio type and change routing via I2S based on input
    /*if (isSPDIFSwitchConditionMet()==1)
    {
        //Fill Digital
        g_sys_debug.IsDigitalAudio=1;
        //Just jump over Audio settings in this case so we stay on PCM
        if (EdidTvSupportsDigitalFormats()==1 || DISABLE_UNSUPPORTED_AUDIO_MSG)
        {
            //if the sink supports digital formats, everything is ok
            if (routeOnI2S==1)
            {
                routeOnI2S=0;
                TxDev[TX0DEV].bTxAudSel=AUD_SPDIF;
            }
            invalid_audio=0;
        }
        else
        {
            //We have invalid audio from source since our sink cannot support it
            invalid_audio=1;
        }
    }
    else
    {
        //Fill Digital
        g_sys_debug.IsDigitalAudio=0;
        if (routeOnI2S==0)
        {
            routeOnI2S=1;
            TxDev[TX0DEV].bTxAudSel=AUD_I2S;
        }
        //Audio is allways valid on PCM audio
        invalid_audio=0;
    }  TODO: uncomment this when audio will be implemented*/

    switch (TxDev[TX0DEV].TxAState)
    {
        case TxAStateOff:
            if (TxDev[TX0DEV].bTxHDMIMode == TRUE)
            {
                if (TxDev[TX0DEV].bSrcAudREADY == TRUE)
                {
                    TX_SwitchAudioState(TxAStateCheck);
                }
            }
            break;

        case TxAStateCheck:
            if (TxDev[TX0DEV].TxAStateCounter == 0)
            {
                TX_SwitchAudioState(TxAStateOn);
            }
            else
            {
                TxDev[TX0DEV].TxAStateCounter--;
            }
            break;

        case TxAStateOn:
            if (TxDev[TX0DEV].bSrcAudREADY == FALSE)
            {
                TX_SwitchAudioState(TxAStateOff);
            }
            else
            {
                if (TxDev[TX0DEV].bSrcAudChange == TRUE)
                {
                    TX_SwitchAudioState(TxAStateCheck);
                }
            }
            break;
    }
}


void HDMITXITEFULL_CODE_SECTION(SetTxStandardAviInfoFrame) (AviInfoFrameType *infoFrame, unsigned int resolutionVic)
{
	int status;
    /***************************************************
    ----------------------------------------------------
    Data Byte 1 :
    ----------------------------------------------------
    BYTE [Reserved, Y1, Y0, A0, B1, B0, S1, S0]
    ----------------------------------------------------
    Y0, Y1 - Color Space RGB or YCbCr
        A0 - Active Format Information Present
    B0, B1 - Bar Information
    S0, S1 - Scan Information (Overscan/Underscan)
    ----------------------------------------------------
    ----------------------------------------------------
    Data Byte 2 :
    ----------------------------------------------------
    BYTE [C1, C0, M1, M0, R3, R2, R1, R0]
    ----------------------------------------------------
    C0, C1 - Colorimetry
    M0, M1 - Picture Aspect Ratio
    R0, R3 - Active Format Aspect Ratio

    ***************************************************/

    //printf ("SetTxStandardAviInfoFrame (void)\n\n");

    // Set the InfoFrame Header - common to all resolutions
    infoFrame->PacketByte.AVI_HB[0] = 130;
    infoFrame->PacketByte.AVI_HB[1] =   2;
    infoFrame->PacketByte.AVI_HB[2] =  13;

    // Set the Common DataBytes
    infoFrame->PacketByte.AVI_DB [0] = 64;
    //                 AVI_DB [1] = Diffrent
    infoFrame->PacketByte.AVI_DB [2] = 0;
    //                 AVI_DB [3] = Diffrent
    infoFrame->PacketByte.AVI_DB [4] = 0;
    infoFrame->PacketByte.AVI_DB [5] = 0;
    infoFrame->PacketByte.AVI_DB [6] = 0;
    infoFrame->PacketByte.AVI_DB [7] = 0;
    infoFrame->PacketByte.AVI_DB [8] = 0;
    infoFrame->PacketByte.AVI_DB [9] = 0;
    infoFrame->PacketByte.AVI_DB[10] = 0;
    infoFrame->PacketByte.AVI_DB[11] = 0;
    infoFrame->PacketByte.AVI_DB[12] = 0;

    // Set diffrent Colorimetry
    if ((resolutionVic == MODE_480p60_16_9) || (resolutionVic == MODE_576p50_16_9))
    {
        // Default Colorimetry used for 480p, 480i, 576p, 576i, 240p and 288p is
        // SMPTE 170M / ITU601
        infoFrame->PacketByte.AVI_DB[1] = 104;
    }
    else
    {
        // Default Colorimetry used for 720p, 1080i and 1080p is
        // ITU709
        infoFrame->PacketByte.AVI_DB[1] = 168;
    }

	// Set the Vic
	infoFrame->PacketByte.AVI_DB[3] = resolutionVic;
}


void HDMITXITEFULL_CODE_SECTION(TxChangeVideoState) (TxVideoStateType state)
{
    TxDev[TX0DEV].TxVState = state;
    HdmiDriverInfo.TxVideoState = state;
}


inline void TxSetResolutionVicIndependantMode (unsigned int newVic)
{
    resolutionIndex = newVic;
}

// !!! Alex : Used in Rx.c - DONE
SYS_STATUS HDMITXITEFULL_CODE_SECTION(SetupTxVideo) (unsigned int TxDriverMode)
{
    // Alex: Added this in order to prevent unnecessary Sabre reconfigurations
    static unsigned int previousVic = 0;
    unsigned int localDVI_VIC = 0;

    AviInfoFrameType    TxAviInfoFrame;
    BYTE                bTxInputVideoMode   = 0;
    BYTE                bTxOutputVideoMode  = 0;

    if ((TxDev[TX0DEV].TxVState == TxVStateUnplug) || (TxDev[TX0DEV].TxVState == TxVStateReset))
    {
        return ER_SUCCESS;
    }

    if ((TxDriverMode == ModeTxIndependantVideoOnly) || (TxDriverMode == ModeTxIndependantAnalog))
    {
        TxDev[TX0DEV].bTxHDMIMode = TRUE && TxDev[TX0DEV].bDnHDMIMode;
        // CC: The null packet need to be enable to can generate 3D formats at output on AutomatedTest
        #ifndef AUTOMATED_TEST_APP
                DISABLE_NULL_PKT();
        #endif //AUTOMATED_TEST_APP
    }

    if ((TxDriverMode == ModeTxIndependantVideoOnly) || (TxDriverMode == ModeTxIndependantAnalog))
    {
    	unsigned int local_VIC=0;
    	if (local_VIC!=0)
    	{
    		//If local_VIC !- that means we have detected a DVI source
    		//so we use the decoded VIC of that source
    		SetTxStandardAviInfoFrame (&TxAviInfoFrame, local_VIC);
    	}
    	else
    	{
    		//No DVI source, we're in one of the independent modes so we're just setting the requested resolution
    		SetTxStandardAviInfoFrame (&TxAviInfoFrame, resolutionIndex);
    	}
    }


    // Alex : The Tx input from the Sabre Lcd is RGB24
    TxDev[TX0DEV].bTxInputVideoMode = F_Tx_MODE_RGB24;

    // Alex : The Tx input from the Sabre Lcd is Yuv444
    //TxDev[TX0DEV].bTxInputVideoMode = F_MODE_YUV444;

//    printf("TxDriverMode:%d\n",TxDriverMode);

    if (TxDev[TX0DEV].bTxHDMIMode == TRUE)
    {

    	TxDev[TX0DEV].bTxOutputVideoMode = F_Tx_MODE_RGB24;

        HDMITX_DEBUG_PRINT(("TxDev[TX0DEV].bTxInputVideoMode=0x%X\n", (int)TxDev[TX0DEV].bTxInputVideoMode));

        TxAviInfoFrame.info.ColorMode = TxDev[TX0DEV].bTxOutputVideoMode;
        TxAviInfoFrame.info.FU1 = 0;
        TxAviInfoFrame.info.FU2 = 0;
        TxAviInfoFrame.info.FU3 = 0;
        TxAviInfoFrame.info.FU4 = 0;

        if ( (localDVI_VIC!=0) || (TxDriverMode == ModeTxIndependantVideoOnly) || (TxDriverMode == ModeTxIndependantAnalog))
        {
            // Alex: Get the Vic Code before we change it here and save it in variable
            previousVic = Vic;
            Vic = TxAviInfoFrame.PacketByte.AVI_DB[3];
/*            if ( (!IsHDMIMode()) && (localDVI_VIC!=0) )
            {
            	//If Hdmi is down but there is a valid DVI resolution then take that DVI decoded VIC
            	//as being the correct one
            	Vic=localDVI_VIC;
            }*/

            #ifndef AUTOMATED_TEST_APP
            // Alex: TODO: Cleanup
            // Alex: When running in Compatibility Mode, meaning 1080p24 to 1080i60 conversion
            // we should forward the Avi InfoFrame with the exception of the VIC code.
          /*  if ((Vic == MODE_1080p24_16_9) && (!EdidTVSupports1080p24()))
            {
                TxAviInfoFrame.info.VIC = MODE_1080i60_16_9;
                TxAviInfoFrame.PacketByte.AVI_DB[3] = MODE_1080i60_16_9;
            } TODO: uncomment this when we implement EDID     */
            #endif //AUTOMATED_TEST_APP

            SetAVIInfoFrame (&TxAviInfoFrame);

            #ifdef AUTOMATED_TEST_APP
            // Alex : This is used only by the BugHound and Bist Applications
            // HdmiStartSabreVideoBugHoud (Vic, TxDriverMode);
            #else
            // Alex: Configure the Sabre Video mode here for valid VicCode
            if (Vic != 0)
            {
            	//printf("This printf makes it work\n");
            	//sleep_ms_no_rtc(1000);
                //HdmiStartSabreVideo (SetSabreVideoIO(Vic, TxDriverMode)); todo:implement this when needed
            }
            #endif

          //  AudioStart (10, TxDev[TX0DEV].bOutAudSampleFrequence, 0); TODO:implement this when needed
        }
        else
        {
            Switch_HDMITX_Bank(1);
            HDMITX_WriteI2C_Byte(REG_TX_AVIINFO_DB1, 0x00);

            Switch_HDMITX_Bank(0);
            TxDev[TX0DEV].bTxOutputVideoMode = F_Tx_MODE_RGB24;
            DISABLE_AVI_INFOFRM_PKT();
        }
    }
    else
    {
        Switch_HDMITX_Bank(1);
        HDMITX_WriteI2C_Byte(REG_TX_AVIINFO_DB1, 0x00);

        Switch_HDMITX_Bank(0);
        TxDev[TX0DEV].bTxOutputVideoMode = F_Tx_MODE_RGB24;
        DISABLE_AVI_INFOFRM_PKT();
    }

    TxDev[TX0DEV].bTxVidReady = TRUE;

    bTxInputVideoMode = TxDev[TX0DEV].bTxInputVideoMode;
    bTxInputVideoMode |= F_VIDMODE_ITU709;
    bTxInputVideoMode |= F_VIDMODE_16_235;
    bTxOutputVideoMode = TxDev[TX0DEV].bTxOutputVideoMode;
    HDMITX_DEBUG_PRINT(("bTxInputVideoMode=0x%X,bTxOutputVideoMode=0x%X\n", (int)bTxInputVideoMode, (int)bTxOutputVideoMode));

    SetInputMode(bTxInputVideoMode,TxDev[TX0DEV].bTxVideoInputType);

    SetCSCScale(bTxInputVideoMode, bTxOutputVideoMode);

    if (TxDev[TX0DEV].bTxHDMIMode == TRUE)
    {
        HDMITX_WriteI2C_Byte (REG_TX_HDMI_MODE, B_TX_HDMI_MODE);
        HDMITX_WriteI2C_Byte (REG_TX_PKT_GENERAL_CTRL, 0x03);
    }
    else
    {
        HDMITX_WriteI2C_Byte(REG_TX_HDMI_MODE, B_TX_DVI_MODE);
    }

    return ER_SUCCESS;
}



#if 1
SYS_STATUS HDMITXITEFULL_CODE_SECTION(SetupTxAud) (void)
{
    AudioInfoFrameType audioinfoframe;

/*
#ifdef _FIXED_TXAUDIO_
    //     TxDev[TX0DEV].bTxAudioSampleFreq = AUDSF_48KHz;
    //     TxDev[TX0DEV].bTxAudSel = AUD_I2S;
    TxDev[TX0DEV].bTxAudioEnable = 1;

    audioinfoframe.info.AudioChannelCount = 1;
    audioinfoframe.info.SpeakerPlacement = 0;
    audioinfoframe.info.RSVD1 = 0;
    audioinfoframe.info.AudioCodingType = 0;
    audioinfoframe.info.SampleSize = 0;
    audioinfoframe.info.SampleFreq = 0;
    audioinfoframe.info.Rsvd2 = 0;
    audioinfoframe.info.FmtCoding = 0;
    audioinfoframe.info.Rsvd3 = 0;
#else

 //   #ifdef _ENMULTICH_TX_
   if (routeOnI2S == 0)
    {
        TxDev[TX0DEV].bTxAudioEnable = 1;

        if(AudioCaps.AudioFlag&B_MULTICH && TxDev[TX0DEV].bDnLPCMChNum > 1)
        {
            if((audioinfoframe.info.SpeakerPlacement%4 != 0) && (AudioCaps.AudSrcEnable&0x02) == 0x02)
            {
                TxDev[TX0DEV].bTxAudioEnable |= 2;
            }


            if(((audioinfoframe.info.SpeakerPlacement > 3 && audioinfoframe.info.SpeakerPlacement < 20) ||
                audioinfoframe.info.SpeakerPlacement > 23) &&
                (AudioCaps.AudSrcEnable&0x04) == 0x04 )
            {
                TxDev[TX0DEV].bTxAudioEnable |= 4;
            }

            if((audioinfoframe.info.SpeakerPlacement > 11) && (AudioCaps.AudSrcEnable&0x08) == 0x08)
            {
                TxDev[TX0DEV].bTxAudioEnable |= 8;
            }
        }
        else
        {
            if(AudioCaps.AudioFlag&B_CAP_LPCM)
            {
                audioinfoframe.info.AudioChannelCount = 1;
            }
            else
            {
                audioinfoframe.info.AudioChannelCount = 0;
            }

            audioinfoframe.info.SpeakerPlacement = 0;
            audioinfoframe.info.AudioCodingType = 0;
            audioinfoframe.info.SampleSize = 0;
            audioinfoframe.info.SampleFreq = 0;
            audioinfoframe.info.FmtCoding = 0;
        }

        audioinfoframe.info.RSVD1 = 0;
        audioinfoframe.info.Rsvd2 = 0;
        audioinfoframe.info.Rsvd3 = 0;
    }
    else
    {
        TxDev[TX0DEV].bTxAudioEnable = 1;

        //audioinfoframe.info.AudioChannelCount = 1;
        //audioinfoframe.info.SpeakerPlacement = 0;
        //audioinfoframe.info.RSVD1 = 0;
        //audioinfoframe.info.AudioCodingType = 0;
        //audioinfoframe.info.SampleSize = 0;
        //audioinfoframe.info.SampleFreq = 0;
        //audioinfoframe.info.Rsvd2 = 0;
        //audioinfoframe.info.FmtCoding = 0;
        //audioinfoframe.info.Rsvd3 = 0;
    }
    //#endif    //_ENMULTICH_TX_
#endif  //end of _

    if(RxNewAUDInfoFrameF == TRUE)
    {
        //HDMITX_DEBUG_PRINT3(("SetAudioInfoFrame\n"));
        SetAudioInfoFrame(&audioinfoframe, 0); // Alex: Not sure this should be 0, we may need to pass the value of independentTx from hdmi_common.c


     }
    else
    {
        DISABLE_AUD_INFOFRM_PKT();
    }TODO: uncomment this when audio will be implemented*/

    TxDev[TX0DEV].bTxAudReady=TRUE;

     return ER_SUCCESS;
}
#endif

/*=============================================================================
   Function: InitVideoMode
   Remark: For evaluate
  =============================================================================*/
SYS_STATUS HDMITXITEFULL_CODE_SECTION(InitVideoMode) (void)
{
    BYTE uc;
    BOOL HighFreq;
    BYTE  RxClkXCNT;


    // TODO: Alex: Hardcoded Value for 720p 60 in Independant Tx Running Mode.
    //RxClkXCNT = 0x2F;
    RxClkXCNT = 47;// 0x30;

    TxDev[TX0DEV].bTxHDMIMode  =  TxDev[TX0DEV].bDnHDMIMode;

    if (TxDev[TX0DEV].bTxHDMIMode)
    {
    	//Set Color depth
        SetTXColordepth();//mingchih add
        uc = HDMITX_ReadI2C_Byte(REG_TX_GCP);

        uc >>= O_COLOR_DEPTH;
        uc  &= M_COLOR_DEPTH;
    }
    else
    {
        uc = B_CD_NODEF;
    }

    switch (uc)
    {
        case B_CD_30:
            HighFreq = (RxClkXCNT < 55);
            break;

        case B_CD_36:
            HighFreq = (RxClkXCNT < 65);
            break;

        default:
            HighFreq = (RxClkXCNT < 0x2B);
    }

        SetupAFE (HighFreq);// pass if High Freq request

       // SwReset();
       // SwReset();
        FireAFE();
        //We must reset Video after SetupAFE and Fire AFE
        SwReset();
    return ER_SUCCESS;
}


/*=============================================================================
   change 20080229 by hermes,detect 6613 rom type,set hdcp rom control register

   Function: InitHDMITX_HDCPROM()
   Remark:  init     REG_TX_LISTCTRL for HDCP mem
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(InitHDMITX_HDCPROM) (void)
{
    C6613_Check_EMEM_sts();          //02292008  detect 6613 packg  add by hermes

    Switch_HDMITX_Bank(0);
//    HDMITX_WriteI2C_Byte(0xF8,0xC3);//unlock register
//    HDMITX_WriteI2C_Byte(0xF8,0xA5);//unlock register
//    if(TxDev[TX0DEV].TxEMEMStatus==TRUE){
//            // with internal eMem
//#ifdef _PRINT_HDMI_TX_
//            //HDMITX_DEBUG_PRINT((" internal ROM    \n"));
//#endif
//            HDMITX_WriteI2C_Byte(REG_TX_ROM_HEADER,0xE0);
//            HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL,0x48);
//    }else{
//        // with external ROM
//#ifdef _PRINT_HDMI_TX_
//        //HDMITX_DEBUG_PRINT(("     external ROM \n"));
//#endif
//        HDMITX_WriteI2C_Byte(REG_TX_ROM_HEADER,0xA0);
//        HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL,0x00);
//    }
//    HDMITX_WriteI2C_Byte(0xF8,0xFF);//lock register
}



/*=============================================================================
   Function: C6613_Check_EMEM_sts()
   Remark:   chk c6613 HDCP use emem or extrom
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(C6613_Check_EMEM_sts) (void)
{

    Switch_HDMITX_Bank(0);
    HDMITX_WriteI2C_Byte(0xF8,0xC3);//unlock register
    HDMITX_WriteI2C_Byte(0xF8,0xA5);//unlock register

    HDMITX_WriteI2C_Byte(0x22,0x00);


    HDMITX_WriteI2C_Byte(0x10,0x03);

    HDMITX_WriteI2C_Byte(0x11,0xA0);
    HDMITX_WriteI2C_Byte(0x12,0x00);
    HDMITX_WriteI2C_Byte(0x13,0x08);
    HDMITX_WriteI2C_Byte(0x14,0x00);
    HDMITX_WriteI2C_Byte(0x15,0x00);

    if((0x80 & HDMITX_ReadI2C_Byte(0x1c)))
    {
    	//if 0x1c[7]==1 EXT_ROM
    	TxDev[TX0DEV].TxEMEMStatus=FALSE;
    	//HDMITX_DEBUG_PRINTF(("==Dev %X is ExtROM==\n",I2CDEV));
    }
    else
    {   //if 0x1c[1] !=1 EMEM
    	TxDev[TX0DEV].TxEMEMStatus=TRUE;
    	//HDMITX_DEBUG_PRINTF(("==Dev %X is EMEM==\n",I2CDEV));
    }

    if(TxDev[TX0DEV].TxEMEMStatus==TRUE)
    {
    	// with internal eMem
    	//HDMITX_DEBUG_PRINTF((" internal ROM    \n"));
    	HDMITX_WriteI2C_Byte(REG_TX_ROM_HEADER,0xE0);
    	HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL,0x48);
    }
    else
    {
    	// with external ROM
    	//HDMITX_DEBUG_PRINTF(("     external ROM \n"));
    	HDMITX_WriteI2C_Byte(REG_TX_ROM_HEADER,0xA0);
    	HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL,0x00);
    }

    HDMITX_WriteI2C_Byte(0xF8,0xFF);//lock register
}


/*=============================================================================
                            TX Audio Part
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(SetTXaudioChannelStatus) (RX_REG_AUDIO_CHSTS ChannelSts)
{
	BYTE temp;
	Switch_HDMITX_Bank(1);

#if 1
	temp = ChannelSts.chstatus.ChannelStatusMode;
	temp = (temp << 1) | ChannelSts.chstatus.CopyRight;
	temp = temp << 3;

	if(TxDev[TX0DEV].AudSrcHBR == TRUE)
	{
		temp |= 0x04;
	}
	else if (TxDev[TX0DEV].AucSrcLPCM == FALSE)
	{
		temp |= 0x04;
	}

	HDMITX_DEBUG_PRINT(("SetTXaudioChannelStatus %i - %i\n", ChannelSts.chstatus.OriginalSamplingFreq, ChannelSts.chstatus.WorldLen));
	// CB-hack: Hardcode word length to 16 bits
	// CB:  TODO... support 20 bit audio correctly
	//According to Cormac we should only do this if we have PCM over I2S audio
   /*if (routeOnI2S == 1)
	{
		ChannelSts.chstatus.WorldLen = 0x2;
	} TODO: uncomment this when we will support audio*/
	HDMITX_DEBUG_PRINT(("SetTXaudioChannelStatus %i - %i\n", ChannelSts.chstatus.OriginalSamplingFreq, ChannelSts.chstatus.WorldLen));

	HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_MODE, temp);
	AudioChannelArray[0] = temp;
	HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_CAT, ChannelSts.chstatus.CategoryCode) ;
	AudioChannelArray[1] = ChannelSts.chstatus.CategoryCode;
	HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_SRCNUM, ChannelSts.chstatus.SourceNumber) ;
	AudioChannelArray[2] = ChannelSts.chstatus.SourceNumber;
	HDMITX_WriteI2C_Byte (REG_TX_AUD0CHST_CHTNUM, ChannelSts.chstatus.ChannelNumber);
	AudioChannelArray[3] = ChannelSts.chstatus.ChannelNumber;
	temp = ChannelSts.chstatus.ClockAccuary;
	temp = (temp << 4) | ChannelSts.chstatus.SamplingFreq;
	HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_CA_FS, temp);
	AudioChannelArray[4] = temp;

	temp = ChannelSts.chstatus.OriginalSamplingFreq;
	temp = (temp << 4) | ChannelSts.chstatus.WorldLen;
	HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_OFS_WL, temp);
	AudioChannelArray[5] = temp;

#else
	if (TxDev[TX0DEV].AudSrcHBR == TRUE)
	{
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_MODE ,     (HDMITX_ReadI2C_Byte(REG_TX_AUDCHST_MODE) |0x04)); //set bit2
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_CAT,       0);
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_SRCNUM,    0);
		HDMITX_WriteI2C_Byte (REG_TX_AUD0CHST_CHTNUM,   0);
	}
	else if (TxDev[TX0DEV].AucSrcLPCM == TRUE)
	{
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_MODE,      0 | ((NumChannel == 1) ? 1 : 0)); // 2 audio channel without pre-emphasis, if NumChannel set it as 1.
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_CAT,       0);
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_SRCNUM,    SoruceNum);
		HDMITX_WriteI2C_Byte (REG_TX_AUD0CHST_CHTNUM,   0);
	}
	else
	{
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_MODE,      0x04);
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_CAT,       0);
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_SRCNUM,    0);
		HDMITX_WriteI2C_Byte (REG_TX_AUD0CHST_CHTNUM,   0);
	}

	if (TxDev[TX0DEV].AudSrcHBR == TRUE)
	{
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_CA_FS,     0x09); //set bit[3:0] 0x1001
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_OFS_WL,    0x62);
	}
	else
	{
		HDMITX_WriteI2C_Byte (REG_TX_AUDCHST_CA_FS,     0x00 | fs); // choose clock

		AudioOrgSampleFreq = ~AudioOrgSampleFreq ; // OFS is the one's complement of FS
		HDMITX_WriteI2C_Byte(REG_TX_AUDCHST_OFS_WL,     (AudioOrgSampleFreq << 4) | SWL);
	}
#endif

	Switch_HDMITX_Bank(0);
}


/*=====================================================================
  If input Audio is HBR,Hermes was proposed use I2S input.
  =====================================================================*/

//void SetTxAudioCapability (BOOL HBR_on,BOOL LPCM,BOOL SPDIF_in,BYTE channel,BYTE SampleFrequence)
void HDMITXITEFULL_CODE_SECTION(SetTxAudioCapability) (AUDIO_CAPS  srcAudiocaps)
{
	TxDev[TX0DEV].bOutAudChannelSrc      = srcAudiocaps.AudSrcEnable;
	TxDev[TX0DEV].bOutAudSampleFrequence = srcAudiocaps.SampleFreq;

	TxDev[TX0DEV].AucSrcLPCM = FALSE;
	TxDev[TX0DEV].AudSrcHBR  = FALSE;
	TxDev[TX0DEV].AudSrcDSD  = FALSE;

	/*//#ifdef _AUD_COMPRESS_VIA_I2S_
	if (routeOnI2S == 1)
	{
		TxDev[TX0DEV].bTxAudSel = AUD_I2S;
	}
	//#else
	else
	{
		TxDev[TX0DEV].bTxAudSel = AUD_SPDIF;
	}
	//#endif - TODO: uncomment this when we will support audio*/

	if (srcAudiocaps.AudioFlag & B_CAP_HBR_AUDIO)
	{
		TxDev[TX0DEV].AudSrcHBR = TRUE;

		/*//#ifdef _AUD_COMPRESS_VIA_I2S_
		if (routeOnI2S == 1)
		{
			TxDev[TX0DEV].bTxAudSel = AUD_I2S;
		}
		//#endif TODO: uncomment this when we will support audio*/

	}
	else if (srcAudiocaps.AudioFlag & B_CAP_DSD_AUDIO)
	{
		//do nothing
		TxDev[TX0DEV].AudSrcDSD = TRUE;
	}
	else if (srcAudiocaps.AudioFlag & B_CAP_LPCM)
	{
		TxDev[TX0DEV].AucSrcLPCM  = TRUE;

		/*//#ifdef _AUD_COMPRESS_VIA_I2S_
		if (routeOnI2S == 1)
		{
			TxDev[TX0DEV].bTxAudSel = AUD_I2S;
		}else{
			TxDev[TX0DEV].bTxAudSel = AUD_SPDIF;
		}
		//#endif TODO: uncomment this when we will support audio*/

	}

	SetTXaudioChannelStatus (srcAudiocaps.ChStat);

	TxDev[TX0DEV].bSrcAudChange = TRUE;
	TxDev[TX0DEV].bSrcAudREADY  = TRUE;

	HDMITX_DEBUG_PRINT(("SetTxAudioCapability(0x%x,0x%x,0x%x,0x%x,0x%x)\n",
			(int)TxDev[TX0DEV].AudSrcHBR,
			(int)TxDev[TX0DEV].AucSrcLPCM,
			(int)TxDev[TX0DEV].bTxAudSel,
			(int)TxDev[TX0DEV].bOutAudChannelSrc,
			(int)TxDev[TX0DEV].bOutAudSampleFrequence));
	return;
}

BOOL HDMITXITEFULL_CODE_SECTION(IsTxAudioTurnON) (void)
{

  return TxDev[TX0DEV].bSrcAudREADY;

}

void HDMITXITEFULL_CODE_SECTION(TxAudioTurnoff) (void)
{
     TxDev[TX0DEV].bSrcAudREADY = FALSE;
}

int HDMITXITEFULL_CODE_SECTION(HDMITxGetAudioStatus)()
{
    //This function will return audio status once we get to it
   /* return routeOnI2S;  - TODO: uncomment this when we will support audio*/
	return 1;
}


void HDMITXITEFULL_CODE_SECTION(TXSPDIF)(u32 val)
{
    u32 uAUDIOCTRL;
    Switch_HDMITX_Bank (0);

    uAUDIOCTRL = HDMITX_ReadI2C_Byte (REG_TX_AUDIO_CTRL0);
    if (val != 0)
    {
         uAUDIOCTRL = uAUDIOCTRL | B_AUD_SPDIF;
    }
    else
    {
        uAUDIOCTRL = uAUDIOCTRL & (~B_AUD_SPDIF);
    }
    HDMITX_WriteI2C_Byte (REG_TX_AUDIO_CTRL0,uAUDIOCTRL);

    return;
}


void  HDMITXITEFULL_CODE_SECTION(Dump_HDMITXReg) ()//max7088
{
#ifdef _DUMPTX_DEBUG_PRINT_
#pragma message("Dump_HDMITXReg() enabled.")

	int i,j;
	// BYTE reg;
	// BYTE bank;
	BYTE ucData;
	// BYTE CurDev,CurAdr;;
	// CurDev=I2CDEV;
	// CurAdr=I2CADR;
	// I2CADR=GetTxAdr(0);
	// I2CDEV=GetTxDev(0);

	DUMPTX_DEBUG_PRINT(("Tx register content\n"));

	for(j=0; j < 16; j++)
	{
		DUMPTX_DEBUG_PRINT((" %02X",(int)j));
		if((j==3)||(j==7)||(j==11))
		{
			DUMPTX_DEBUG_PRINT(("  "));
		}
	}
	DUMPTX_DEBUG_PRINT(("\n        -----------------------------------------------------\n"));
	Switch_HDMITX_Bank(0);

	for(i=0; i <= 0xFF; i+=16)
	{
		DUMPTX_DEBUG_PRINT(("[ %02X]  ",(int)i));
		for(j=0; j < 16; j++)
		{
			ucData=HDMITX_ReadI2C_Byte((BYTE)((i+j)&0xFF));
			DUMPTX_DEBUG_PRINT((" %02X",(int)ucData));
			if((j==3)||(j==7)||(j==11))
			{
				DUMPTX_DEBUG_PRINT((" -"));
			}
		}
		DUMPTX_DEBUG_PRINT(("\n"));
		if((i % 0x40)==0x30)
		{
			DUMPTX_DEBUG_PRINT(("        -----------------------------------------------------\n"));
		}
	}

	Switch_HDMITX_Bank(1);
	for(i=0x00; i <= 0xFF; i+=16)
	{
		DUMPTX_DEBUG_PRINT(("[1%02X]  ",i));
		for(j=0; j < 16; j++)
		{
			ucData=HDMITX_ReadI2C_Byte((BYTE)((i+j)&0xFF));
			DUMPTX_DEBUG_PRINT((" %02X",(int)ucData));
			if((j==3)||(j==7)||(j==11))
			{
				DUMPTX_DEBUG_PRINT((" -"));
			}
		}
		DUMPTX_DEBUG_PRINT(("\n"));
		if((i % 0x40)==0x30)
		{
			DUMPTX_DEBUG_PRINT(("        -----------------------------------------------------\n"));
		}
	}
	Switch_HDMITX_Bank(0);
#endif
}




