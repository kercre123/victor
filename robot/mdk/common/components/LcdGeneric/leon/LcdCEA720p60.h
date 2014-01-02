///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Display configuration for a CEA Standard 720p60 interface
///

#include "LcdApiDefines.h"
#include "DrvLcd.h"

LCDDisplayCfg lcdSpec720p60 = {
            .width          = 1280,
            .hPulseWidth    = 40,
            .hBackP         = 220,
            .hFrontP        = 110,
            .height         = 720,
            .vPulseWidth    = 5,
            .vBackP         = 20,
            .vFrontP        = 5,
            .outputFormat   = D_LCD_OUTF_FORMAT_YCBCR444 | D_LCD_OUTF_PCLK_EXT,
            .control        = D_LCD_CTRL_OUTPUT_RGB | D_LCD_CTRL_PROGRESSIVE,
            .pixelClockkHz  = 74250
};

