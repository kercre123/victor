///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Hdmi Tx Functions.
///
/// This is the implementation of Hdmi library.
///

// 1: Includes
// ----------------------------------------------------------------------------
#include "HdmiTxIteApi.h"
#include "HdmiTxIteDrv.h"
#include "DrvSvu.h"
#include "DrvI2cMaster.h"
#include "assert.h"
#include <DrvTimer.h>

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
//#define hdmi_i2c_w(x, y) DrvIicWr2(IIC2_BASE_ADR, x, y)
//#define hdmi_i2c_r(x) DrvIicRd8ba(IIC2_BASE_ADR, x)
                
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------
static I2CM_Device * localHdmiI2cHandle;

static u8 hdmiWriteProto[] = I2C_PROTO_WRITE_8BA;
static u8 hdmiReadProto[]  = I2C_PROTO_READ_8BA;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
static int hdmi_i2c_w(int regAddr, u8 value);
static u8  hdmi_i2c_r(int regAddr);

// 6: Functions Implementation
// ----------------------------------------------------------------------------

int HDMI_TX_ITE_CODE_SECTION(HdmiSetup) (I2CM_Device * hdmiI2cHandle)
{
    if (hdmiI2cHandle == NULL)
        return -1;
    localHdmiI2cHandle = hdmiI2cHandle;
    return 0;
}
               
// Full HDMI setup
void HDMI_TX_ITE_CODE_SECTION(HdmiConfigure)()
{
    unsigned char timeout;
    unsigned char uc;
    unsigned char bColorDepth;
    unsigned char ucData;
    unsigned int TxEMEMStatus=0;
    unsigned int VIC= 4;
	unsigned char pixelrep = 0;


	// Init I2C2 used for HDMI
//	DrvIicTestInit(IIC2_BASE_ADR, HDMI_TX_ADDR_I2C);

    hdmi_i2c_w(REG_TX_INT_CTRL, 0x40);
    // Check EMEM status
    HDMI_BANK0;
    HDMI_BANK0;
    hdmi_i2c_w(0xF8, 0xC3);
    hdmi_i2c_w(0xF8, 0xA5);
    hdmi_i2c_w(0x22, 0x00);
    hdmi_i2c_w(0x10, 0x03);
    hdmi_i2c_w(0x11, 0xA0);
    hdmi_i2c_w(0x12, 0x00);
    hdmi_i2c_w(0x13, 0x08);
    hdmi_i2c_w(0x14, 0x00);
    hdmi_i2c_w(0x15, 0x00);

    for (timeout = 0; timeout < 250; timeout++)
    {
        // wait 1 ms
		SleepMs(10); 
        uc = hdmi_i2c_r(0x1c);
        if (0 != (0x80 & uc))
        {
            TxEMEMStatus = 0;
            break;
        }
        if (0 != (0x38 & uc))
        {
            TxEMEMStatus = 1;
            break;
        }
    }
    hdmi_i2c_w(0xF8, 0xFF);
    SleepMs(500);
    hdmi_i2c_w(REG_TX_SW_RST, B_REF_RST_HDMITX | B_VID_RST_HDMITX | B_AUD_RST_HDMITX | B_AREF_RST | B_HDCP_RST_HDMITX);
    SleepMs(1);
    hdmi_i2c_w(REG_TX_SW_RST, B_VID_RST_HDMITX | B_AUD_RST_HDMITX | B_AREF_RST | B_HDCP_RST_HDMITX);
    // Avoid power loading in un play status.
    hdmi_i2c_w( REG_TX_AFE_DRV_CTRL, B_AFE_DRV_RST | B_AFE_DRV_PWD);
    // Setup HDCP ROM
    HDMI_BANK0;
    hdmi_i2c_w( 0xF8, 0xC3);
    hdmi_i2c_w( 0xF8, 0xA5);
    if (TxEMEMStatus != 1)
	{
        // With internal eMem
        hdmi_i2c_w(REG_TX_ROM_HEADER, 0xE0);
        hdmi_i2c_w(REG_TX_LISTCTRL, 0x48);
    }
	else
	{
        // With external ROM
        hdmi_i2c_w(REG_TX_ROM_HEADER, 0xA0);
        hdmi_i2c_w(REG_TX_LISTCTRL, 0x00);
    }
    hdmi_i2c_w(0xF8, 0xFF);
    // Set interrupt mask,mask value 0 is interrupt available.

    hdmi_i2c_w(REG_TX_INT_MASK1, 0x30);
    hdmi_i2c_w(REG_TX_INT_MASK2, 0xF8);
    hdmi_i2c_w(REG_TX_INT_MASK3, 0x37);
    // Set color depth
    bColorDepth = B_CD_24;
    HDMI_BANK0;
    uc = hdmi_i2c_r(REG_TX_GCP);
    uc &= ~B_COLOR_DEPTH_MASK;
    uc |= bColorDepth&B_COLOR_DEPTH_MASK;
    hdmi_i2c_w(REG_TX_GCP, uc);

    // Enable video output
   // Sw reset
    hdmi_i2c_w(REG_TX_SW_RST, B_VID_RST_HDMITX|B_AUD_RST_HDMITX|B_AREF_RST|B_HDCP_RST_HDMITX);
    SleepMs(500);
    // Set AVI info frame
    HDMI_BANK1;
    hdmi_i2c_w(REG_TX_AVIINFO_DB1, 0x00);
    HDMI_BANK0;
    // Set AV mute
    HDMI_BANK0;
    uc = hdmi_i2c_r(REG_TX_GCP);
    uc &= ~B_TX_SETAVMUTE;
    uc |= 1 ? B_TX_SETAVMUTE : 0;
    hdmi_i2c_w(REG_TX_GCP, uc);
    hdmi_i2c_w(REG_TX_PKT_GENERAL_CTRL, B_ENABLE_PKT | B_REPEAT_PKT);

    // Set HDMI mode
    hdmi_i2c_w(REG_TX_HDMI_MODE, B_TX_HDMI_MODE);

    //Setup AFE
    hdmi_i2c_w(REG_TX_AFE_XP_CTRL,  0x18);
    hdmi_i2c_w(REG_TX_AFE_ISW_CTRL, 0x10);
    hdmi_i2c_w(REG_TX_AFE_IP_CTRL, 0x0C);

    // Sw reset
    SleepMs(500);
    uc = hdmi_i2c_r(REG_TX_SW_RST);
    uc &= ~(B_REF_RST_HDMITX | B_VID_RST_HDMITX);
    hdmi_i2c_w(REG_TX_SW_RST, uc);
    SleepMs(1);
    hdmi_i2c_w(REG_TX_SW_RST, B_AUD_RST_HDMITX | B_AREF_RST | B_HDCP_RST_HDMITX);

    // Sw reset
    SleepMs(500);
    hdmi_i2c_w(REG_TX_SW_RST, B_VID_RST_HDMITX | B_AUD_RST_HDMITX | B_AREF_RST | B_HDCP_RST_HDMITX);
    SleepMs(10);
    hdmi_i2c_w(REG_TX_SW_RST, B_AUD_RST_HDMITX | B_AREF_RST | B_HDCP_RST_HDMITX);
    SleepMs(150);

    // Fire AFE
    HDMI_BANK0;
    hdmi_i2c_w(REG_TX_INT_CLR0, 0);
    hdmi_i2c_w(REG_TX_AFE_DRV_CTRL, 0);
    hdmi_i2c_w(REG_TX_INT_CLR1, B_CLR_VIDSTABLE);
    hdmi_i2c_w(REG_TX_SYS_STATUS, B_INTACTDONE);

    // Enable AVI info frame
    HDMI_BANK1;
    hdmi_i2c_w(REG_TX_AVIINFO_DB1, 0x10);
    hdmi_i2c_w(REG_TX_AVIINFO_DB2, (8 | (2 << 4) | (2 << 6)));
    hdmi_i2c_w(REG_TX_AVIINFO_DB3, 0);
    hdmi_i2c_w(REG_TX_AVIINFO_DB4, VIC);
    hdmi_i2c_w(REG_TX_AVIINFO_DB5, (pixelrep & 3));
    hdmi_i2c_w(REG_TX_AVIINFO_DB6, 0);
    hdmi_i2c_w(REG_TX_AVIINFO_DB7, 0);
    hdmi_i2c_w(REG_TX_AVIINFO_DB8, 0);
    hdmi_i2c_w(REG_TX_AVIINFO_DB9, 0);
    hdmi_i2c_w(REG_TX_AVIINFO_DB10, 0);
    hdmi_i2c_w(REG_TX_AVIINFO_DB11, 0);
    hdmi_i2c_w(REG_TX_AVIINFO_DB12, 0);
    hdmi_i2c_w(REG_TX_AVIINFO_DB13, 0);
    ucData = 0 - (0x10 + (8 | (2 << 4) | (2 << 6)) + VIC + (pixelrep & 3) + 0x82 + 2 + 0x0D);
    hdmi_i2c_w(REG_TX_AVIINFO_SUM, ucData);
    HDMI_BANK0;
    hdmi_i2c_w(REG_TX_AVI_INFOFRM_CTRL, B_ENABLE_PKT | B_REPEAT_PKT);

    // Unset AV mute
    HDMI_BANK0;
    uc = hdmi_i2c_r(REG_TX_GCP);
    uc &= ~B_TX_SETAVMUTE;
    uc |= 0 ? B_TX_SETAVMUTE : 0;
    hdmi_i2c_w(REG_TX_GCP, uc);
    hdmi_i2c_w(REG_TX_PKT_GENERAL_CTRL, B_ENABLE_PKT | B_REPEAT_PKT);

}

static int HDMI_TX_ITE_CODE_SECTION(hdmi_i2c_w) (int regAddr, u8 value)
{
    (void)DrvI2cMTransaction(localHdmiI2cHandle,HDMI_TX_7B, regAddr ,hdmiWriteProto, &value , 1);
    return 0;
}

static u8 HDMI_TX_ITE_CODE_SECTION(hdmi_i2c_r) (int regAddr)
{
    u8 value;
    (void)DrvI2cMTransaction(localHdmiI2cHandle,HDMI_TX_7B, regAddr ,hdmiReadProto, &value , 1);
    return value;
}

