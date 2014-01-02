///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Definitions and types needed by the Hdmi Tx Ite operations library
///

#ifndef _HDMI_TX_ITE_API_DEFINES_H_
#define _HDMI_TX_ITE_API_DEFINES_H_

// 1: Defines
// ----------------------------------------------------------------------------
#ifndef HDMI_TX_ITE_DATA_SECTION
#define HDMI_TX_ITE_DATA_SECTION(x) x
#endif

#ifndef HDMI_TX_ITE_CODE_SECTION
#define HDMI_TX_ITE_CODE_SECTION(x) x
#endif

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
         

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------
#endif // _HDMI_API_DEFINES_H_;
