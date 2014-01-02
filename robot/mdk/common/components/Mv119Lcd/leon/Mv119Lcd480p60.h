#include "LcdApiDefines.h"
#include "DrvLcd.h"

LCDDisplayCfg lcdSpecMv119 = {
            .width          = 800,
            .hPulseWidth    = 1,
            .hBackP         = 215,
            .hFrontP        = 40,
            .height         = 480,
            .vPulseWidth    = 1,
            .vBackP         = 31,
            .vFrontP        = 0,
            .outputFormat   = D_LCD_OUTF_FORMAT_RGB888 |
            				  D_LCD_OUTF_BGR_ORDER,
            .control        = D_LCD_CTRL_OUTPUT_RGB       |
            				  D_LCD_CTRL_TIM_GEN_ENABLE   |
            				  D_LCD_CTRL_ALPHA_BOTTOM_GL2 |
            				  D_LCD_CTRL_ALPHA_MIDDLE_GL1 |
            				  D_LCD_CTRL_ALPHA_TOP_VL1    |
            				  D_LCD_CTRL_ENABLE           |
            				  D_LCD_CTRL_PROGRESSIVE,
            .pixelClockkHz  = 33264
};
