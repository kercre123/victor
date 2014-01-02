#ifndef _BRINGUP_SABRE_LCD_H_
#define _BRINGUP_SABRE_LCD_H_

#include "mv_types.h"
#include "DrvLcdDefines.h"

#define RGB(r,g,b) (( r & 0xFF )|(( g & 0xFF )<< 8 )|(( b & 0xFF )<< 16))
#define YUV(y,u,v) RGB(y,u,v)

/* lcdBase    is one of        MXI_DISP1_BASE_ADR        and            MXI_DISP2_BASE_ADR*/



void DrvLcdTiming(u32 lcdBase,
                    u32 hSize, u32 hPulseWidth,u32 hBackPorch, u32 hFrontPorch,
                    u32 vSize, u32 vPulseWidth,u32 vBackPorch, u32 vFrontPorch );
void DrvLcdCtrl(u32 lcdBase, u32 background, u32 outFormat, u32 ctrl);

void DrvLcdLayer0(u32 lcdBase,u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord);

 void DrvLcdDmaLayer0(u32 lcdBase,u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl);
void DrvLcdDmaLayer0PP(u32 lcdBase, u32 adr1, u32 adr2,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl);
                         
void DrvLcdLayer1(u32 lcdBase,u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord);

void DrvLcdDmaLayer1(u32 lcdBase,u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl);
 void DrvLcdDmaLayer1PP(u32 lcdBase, u32 adr1, u32 adr2,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl);
void DrvLcdLayer2(u32 lcdBase,u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord);

void DrvLcdDmaLayer2(u32 lcdBase,u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl);
void DrvLcdLayer3(u32 lcdBase,u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord);
void DrvLcdDmaLayer3(u32 lcdBase,u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 dmaCtrl);

void DrvLcdPwm0(u32 lcdBase,u32 ctrl, u32 repeat, u32 leadIn, u32 high, u32 low);
void DrvLcdPwm1(u32 lcdBase,u32 ctrl, u32 repeat, u32 leadIn, u32 high, u32 low);
void DrvLcdPwm2(u32 lcdBase,u32 ctrl, u32 repeat, u32 leadIn, u32 high, u32 low);

void DrvLcdVl1Scale(u32 lcdBase,u32 hn, u32 hd, u32 vn, u32 vd);
void DrvLcdVl2Scale(u32 lcdBase,u32 hn, u32 hd, u32 vn, u32 vd);
void DrvLcdGl1Scale(u32 lcdBase,u32 hn, u32 hd, u32 vn, u32 vd);
void DrvLcdGl2Scale(u32 lcdBase,u32 hn, u32 hd, u32 vn, u32 vd);

void DrvLcdCsc(u32 lcdBase, u32 layerNo, u32 *table);
void DrvLcdLut(u32 lcdBase, u32 layerNo, u32 *table, u32 noEnt);
void DrvLcdCbPlanar(u32 lcdBase, u32 layerNo,
                       u32 address, u32 lineWidth, u32 vstride);
void DrvLcdCrPlanar(u32 lcdBase, u32 layerNo,
                       u32 address, u32 lineWidth, u32 vstride);
void DrvLcdCbPlanarPP(u32 lcdBase, u32 layerNo,
                       u32 address, u32 address1, u32 lineWidth, u32 vstride);
void DrvLcdCrPlanarPP(u32 lcdBase, u32 layerNo,
                       u32 address, u32 address1, u32 lineWidth, u32 vstride);
void DrvLcdTransparency(u32 lcdBase, u32 layerNo, u32 transpColor, u32 alpha);
void DrvLcdLayerPos(u32 lcdBase, u32 layerNo, u32 x, u32 y);


#endif

