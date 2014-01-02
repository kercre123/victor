///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Display configuration for a CEA Standard 1080p24 interface
///

#include "LcdApiDefines.h"
#include "DrvLcd.h"

LCDDisplayCfg lcdSpec1080p24 = {
            .width          = 1920,
            .hPulseWidth    = 44,
            .hBackP         = 638,
            .hFrontP        = 148,
            .height         = 1080,
            .vPulseWidth    = 5,
            .vBackP         = 36,
            .vFrontP        = 4,
            .outputFormat   = D_LCD_OUTF_FORMAT_YCBCR444 | D_LCD_OUTF_PCLK_EXT,
            .control        = D_LCD_CTRL_OUTPUT_RGB | D_LCD_CTRL_PROGRESSIVE,
            .pixelClockkHz  = 74250
};

