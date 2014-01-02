///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Low-level driver for LCD interface 
/// 
/// 
/// 

#ifndef _BRINGUP_SABRE_LCD_H_
#define _BRINGUP_SABRE_LCD_H_

#include "mv_types.h"
#include "DrvLcdDefines.h"

#define RGB(r,g,b) (( r & 0xFF )|(( g & 0xFF )<< 8 )|(( b & 0xFF )<< 16))
#define YUV(y,u,v) RGB(y,u,v)

/* lcdBase    is one of        MXI_DISP1_BASE_ADR        and            MXI_DISP2_BASE_ADR*/


/* ***********************************************************************//**
   *************************************************************************
LCD timing configuration



@param
   lcdBase          - base address of the lcd
@param
   hSize            - screen width (horizontal size), actual value written in the register will be -1
@param
   hPulseWidth     - horizontal pulse width , actual value written in the register will be -1
@param
   hBackPorch      - horisontal back porch
@param
   hFrontPorch     - horizontal front porch
@param
   vSize            - screen height (vertical size), actual value written in the register will be -1
@param
   vPulseWidth     - vertical pulse width, actual value written in the register will be -1
@param
   vBackPorch      - vertical back porch
@param
   vFrontPorch     - vertical front porch
@return

info:
 ************************************************************************* */
void DrvLcdTiming(u32 lcdBase,
                    u32 hSize, u32 hPulseWidth,u32 hBackPorch, u32 hFrontPorch,
                    u32 vSize, u32 vPulseWidth,u32 vBackPorch, u32 vFrontPorch );

/* ***********************************************************************//**
   *************************************************************************
LCD basic control



@param
    lcdBase      - base address of the lcd
@param
    background    
				  - [0:7] red
                  - [8:15] green
                  - [16:24] blue
                  .
@param
    outFormat    - value to be written in the output format register,
@param
    ctrl          - value to be written in the control register

	info:

 ************************************************************************* */
void DrvLcdCtrl(u32 lcdBase, u32 background, u32 outFormat, u32 ctrl);

/* ***********************************************************************//**
   *************************************************************************
LCD Layer cfg



@param
     lcdBase - base address of the lcd
@param
      width  - width of the layer
@param
      hight  - hight of the layer
@param
      x - position of the layer
@param
      y - position of the layer
@param
      cfgWord - configuration word for the layer
@return

info

 ************************************************************************* */
void DrvLcdLayer0(u32 lcdBase,u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord);

/* ***********************************************************************//**
   *************************************************************************
LCD Layer DMA cfg



@param
   lcdBase                - base address of the lcd
@param
    adr                   - address where the data for the layer resides
@param
    width                 - width of the layer in pixels
@param
    height                - height of the layer in pixels
@param
    bytesPP               - number of BYTES/pixel (1, 2, 4)
@param
    verticalStride        - number of bytes from the start of a line to the next line in the mem
@param
    ctrl                  - word to be written in the CFG register
@return

info

 ************************************************************************* */
void DrvLcdDmaLayer0(u32 lcdBase,u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl);

/* ***********************************************************************//**
   *************************************************************************
LCD Layer DMA cfg



@param
   lcdBase                - base address of the lcd
@param
    adr1                  - address where the data for the layer resides
@param
    adr2                  - secondary address where the data for the layer resides(double buffering)
@param
    width                 - width of the layer in pixels
@param
    height                - height of the layer in pixels
@param
    bytesPP               - number of BYTES/pixel (1, 2, 4)
@param
    verticalStride        - number of bytes from the start of a line to the next line in the mem
@param
    ctrl                  - word to be written in the CFG register
@return

info

 ************************************************************************* */
void DrvLcdDmaLayer0PP(u32 lcdBase, u32 adr1, u32 adr2,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl);
                         
/* ***********************************************************************//**
   *************************************************************************
LCD Layer cfg



@param
      lcdBase - base address of the lcd
@param
      width  - width of the layer
@param
      hight  - hight of the layer
@param
      x - position of the layer
@param
      y - position of the layer
@param
      cfgWord - configuration word for the layer
@return

info

 ************************************************************************* */
void DrvLcdLayer1(u32 lcdBase,u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord);

/* ***********************************************************************//**
   *************************************************************************
LCD Layer DMA cfg



@param
   lcdBase                - base address of the lcd
@param
    adr                   - address where the data for the layer resides
@param
    width                 - width of the layer in pixels
@param
    height                - height of the layer in pixels
@param
    bytesPP               - number of BYTES/pixel (1, 2, 4)
@param
    verticalStride        - number of bytes from the start of a line to the next line in the mem
@param
    ctrl                  - word to be written in the CFG register
@return

info:

 ************************************************************************* */
void DrvLcdDmaLayer1(u32 lcdBase,u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl);

/* ***********************************************************************//**
   *************************************************************************
LCD Layer DMA cfg



@param
   lcdBase                - base address of the lcd
@param
    adr1                  - address where the data for the layer resides
@param
    adr2                  - secondary address where the data for the layer resides(double buffering)
@param
    width                 - width of the layer in pixels
@param
    height                - height of the layer in pixels
@param
    bytesPP               - number of BYTES/pixel (1, 2, 4)
@param
    verticalStride        - number of bytes from the start of a line to the next line in the mem
@param
    ctrl                  - word to be written in the CFG register
@return

info

 ************************************************************************* */
void DrvLcdDmaLayer1PP(u32 lcdBase, u32 adr1, u32 adr2,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl);

/* ***********************************************************************//**
   *************************************************************************
LCD Layer cfg



@param
      lcdBase - base address of the lcd
@param
      width  - width of the layer
@param
      hight  - hight of the layer
@param
      x - position of the layer
@param
      y - position of the layer
@param
      cfgWord - configuration word for the layer
@return

info:

 ************************************************************************* */
void DrvLcdLayer2(u32 lcdBase,u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord);

/* ***********************************************************************//**
   *************************************************************************
LCD Layer DMA cfg



@param
   lcdBase               - base address of the lcd
@param
    adr                   - address where the data for the layer resides
@param
    width                 - width of the layer in pixels
@param
    height                - height of the layer in pixels
@param
    bytesPP              - number of BYTES/pixel (1, 2, 4)
@param
    verticalStride       - number of bytes from the start of a line to the next line in the mem
@param
    ctrl                  - word to be written in the CFG register
@return

info:

 ************************************************************************* */
void DrvLcdDmaLayer2(u32 lcdBase,u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl);

/* ***********************************************************************//**
   *************************************************************************
LCD Layer cfg



@param
      lcdBase - base address of the lcd
@param
      width  - width of the layer
@param
      hight  - hight of the layer
@param
      x - position of the layer
@param
      y - position of the layer
@param
      cfgWord - configuration word for the layer
@return

info:

 ************************************************************************* */
void DrvLcdLayer3(u32 lcdBase,u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord);

/* ***********************************************************************//**
   *************************************************************************
LCD Layer DMA cfg



@param
    lcdBase              - base address of the lcd
@param
    adr                   - address where the data for the layer resides
@param
    width                 - width of the layer in pixels
@param
    height                - height of the layer in pixels
@param
    bytesPP              - number of BYTES/pixel (1, 2, 4)
@param
    verticalStride       - number of bytes from the start of a line to the next line in the mem
@param
    dmaCtrl              - word to be written in the dma cfg register register
@return

info:

 ************************************************************************* */
void DrvLcdDmaLayer3(u32 lcdBase,u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 dmaCtrl);

/* ***********************************************************************//**
   *************************************************************************
LCD PWM0 ctrl



@param
      lcdBase- base address of the lcd
@param
      ctrl    - word to be written in the control register
@param
      repeat  - number of repetitions
@param
      leadIn - low lead time
@param
      high    - high time in cc
@param
      low     - low time in cc
@return

info:
     bits in the control register are defined in the DrvLcdDefines.h

 ************************************************************************* */
void DrvLcdPwm0(u32 lcdBase,u32 ctrl, u32 repeat, u32 leadIn, u32 high, u32 low);

/* ***********************************************************************//**
   *************************************************************************
LCD PWM1 ctrl



@param
      lcdBase- base address of the lcd
@param
      ctrl    - word to be written in the control register
@param
      repeat  - number of repetitions
@param
      leadIn - low lead time
@param
      high    - high time in cc
@param
      low     - low time in cc
@return

info:
     bits in the control register are defined in the DrvLcdDefines.h

 ************************************************************************* */
void DrvLcdPwm1(u32 lcdBase,u32 ctrl, u32 repeat, u32 leadIn, u32 high, u32 low);

/* ***********************************************************************//**
   *************************************************************************
LCD PWM2 ctrl



@param
      lcdBase- base address of the lcd
@param
      ctrl    - word to be written in the control register
@param
      repeat  - number of repetitions
@param
      leadIn - low lead time
@param
      high    - high time in cc
@param
      low     - low time in cc
@return

info:
     bits in the control register are defined in the DrvLcdDefines.h

 ************************************************************************* */
void DrvLcdPwm2(u32 lcdBase,u32 ctrl, u32 repeat, u32 leadIn, u32 high, u32 low);

/* ***********************************************************************//**
   *************************************************************************
LCD layer scale



@param
      lcdBase - base address of the lcd
@param
     hn - horizontal scale numerator
@param
     hd - horizontal scale denominator
@param
     vn - vertical scale numerator
@param
     vd - vertical scale denominator
@return

info:

 ************************************************************************* */
void DrvLcdVl1Scale(u32 lcdBase,u32 hn, u32 hd, u32 vn, u32 vd);

/* ***********************************************************************//**
   *************************************************************************
LCD layer scale



@param
     lcdBase - base address of the lcd
@param
     hn - horizontal scale numerator
@param
     hd - horizontal scale denominator
@param
     vn - vertical scale numerator
@param
     vd - vertical scale denominator
@return

info:

 ************************************************************************* */
void DrvLcdVl2Scale(u32 lcdBase,u32 hn, u32 hd, u32 vn, u32 vd);

/* ***********************************************************************//**
   *************************************************************************
LCD layer scale



@param
     lcdBase - base address of the lcd
@param
     hn - horizontal scale numerator
@param
     hd - horizontal scale denominator
@param
     vn - vertical scale numerator
@param
     vd - vertical scale denominator
@return

info:

 ************************************************************************* */
void DrvLcdGl1Scale(u32 lcdBase,u32 hn, u32 hd, u32 vn, u32 vd);

/* ***********************************************************************//**
   *************************************************************************
LCD layer scale



@param
     lcdBase - base address of the lcd
@param
     hn - horizontal scale numerator
@param
     hd - horizontal scale denominator
@param
     vn - vertical scale numerator
@param
     vd - vertical scale denominator
@return

info:

 ************************************************************************* */
void DrvLcdGl2Scale(u32 lcdBase,u32 hn, u32 hd, u32 vn, u32 vd);

/* ***********************************************************************//**
   *************************************************************************
LCD csc for the layer



@param
   lcdBase          - base address of the lcd
@param
   layerNo - number of the layer that would use this csc coefficients, 0..3
@param
   table - a table that has the csc values in the folowing order: M11, M12, M13, M21, M22, ...., M32, M33, OFF1, OFF2, OFF3

@return

info:

 ************************************************************************* */
void DrvLcdCsc(u32 lcdBase, u32 layerNo, u32 *table);

/* ***********************************************************************//**
   *************************************************************************
LCD lut



@param
   lcdBase - base address of the lcd
@param
   layerNo - number of the layer that would use the LUT, 2..3
@param
   table - LUT entries
@param
   noEnt - number of entries

@return

info:

 ************************************************************************* */
void DrvLcdLut(u32 lcdBase, u32 layerNo, u32 *table, u32 noEnt);

//!@{
/* ***********************************************************************//**
   *************************************************************************
LCD planar addressing mode addresses 



@param
   lcdBase - base address of the lcd
@param
   layerNo - number of the layer that would use the LUT, 0..1
@param   
   address - address for the cb
@param   
   lineWidth - line width in bytes
@param   
   vstride - vstride in bytes
@return

info:

 ************************************************************************* */
void DrvLcdCbPlanar(u32 lcdBase, u32 layerNo,
                       u32 address, u32 lineWidth, u32 vstride);
void DrvLcdCrPlanar(u32 lcdBase, u32 layerNo,
                       u32 address, u32 lineWidth, u32 vstride);
//!@}

//!@{
/* ***********************************************************************//**
   *************************************************************************
LCD planar addressing mode addresses 



@param
   lcdBase - base address of the lcd
@param
   layerNo - number of the layer that would use the LUT, 0..1
@param   
   address - address for the cb
@param
   address1 - secondary address for the cb(double buffering)
@param   
   lineWidth - line width in bytes
@param   
   vstride - vstride in bytes
@return

info:

 ************************************************************************* */
void DrvLcdCbPlanarPP(u32 lcdBase, u32 layerNo,
                       u32 address, u32 address1, u32 lineWidth, u32 vstride);
void DrvLcdCrPlanarPP(u32 lcdBase, u32 layerNo,
                       u32 address, u32 address1, u32 lineWidth, u32 vstride);
//!@}



/* ***********************************************************************//**
   *************************************************************************
LCD transparency pixel , and alpha blending



@param
   lcdBase - base address of the lcd
@param
   layerNo - number of the layer
@param
   transpColor - colour that is transparent
@param   
   alpha - alpha value for the layer 
   
@return

info:

 ************************************************************************* */
void DrvLcdTransparency(u32 lcdBase, u32 layerNo, u32 transpColor, u32 alpha);

/* ***********************************************************************//**
   *************************************************************************
LCD chnage position af a layer 



@param
   lcdBase - base address of the lcd
@param
   layerNo - number of the layer (0..3)
@param
   x - horizontal position
@param   
   y - vertical position
   
@return

info:

 ************************************************************************* */
void DrvLcdLayerPos(u32 lcdBase, u32 layerNo, u32 x, u32 y);


#endif

