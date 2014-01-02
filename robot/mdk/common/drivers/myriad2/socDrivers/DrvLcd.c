#include <mv_types.h>
#include "registersMyriad.h"
#include "DrvLcd.h"
#include "assert.h" 
#include "stdio.h"


/* ***********************************************************************//**
   *************************************************************************
LCD timing configuration
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdTiming // this means use ROM Version when available
void DrvLcdTiming(u32 lcdBase,
                    u32 hSize, u32 hPulseWidth,u32 hBackPorch, u32 hFrontPorch,
                    u32 vSize, u32 vPulseWidth,u32 vBackPorch, u32 vFrontPorch )
{
   // We control the horizontal,
   SET_REG_WORD(lcdBase + LCD_HSYNC_WIDTH_OFFSET,    hPulseWidth - 1);    //HSW
   SET_REG_WORD(lcdBase + LCD_H_BACKPORCH_OFFSET,    hBackPorch);         //HBP
   SET_REG_WORD(lcdBase + LCD_H_ACTIVEWIDTH_OFFSET,  hSize - 1);          //AVW
   SET_REG_WORD(lcdBase + LCD_H_FRONTPORCH_OFFSET,   hFrontPorch);        //HFP
   // and the vertical
   // odd fields
   SET_REG_WORD(lcdBase + LCD_VSYNC_WIDTH_OFFSET,    vPulseWidth - 1);    //VSW
   SET_REG_WORD(lcdBase + LCD_V_BACKPORCH_OFFSET,    vBackPorch);         //VBP
   SET_REG_WORD(lcdBase + LCD_V_ACTIVEHEIGHT_OFFSET, vSize - 1);          //AVH
   SET_REG_WORD(lcdBase + LCD_V_FRONTPORCH_OFFSET,   vFrontPorch);        //VFP

   SET_REG_WORD(lcdBase + LCD_VSYNC_START_OFFSET_OFFSET, 0);//hPulseWidth + hBackPorch + hSize + hFrontPorch - 1);
   SET_REG_WORD(lcdBase + LCD_VSYNC_END_OFFSET_OFFSET  , 0);//hPulseWidth + hBackPorch + hSize + hFrontPorch - 1);

   // even fields
   SET_REG_WORD(lcdBase + LCD_VSYNC_WIDTH_EVEN_OFFSET,   vPulseWidth - 1);     //VSW
   SET_REG_WORD(lcdBase + LCD_V_BACKPORCH_EVEN_OFFSET,    vBackPorch);   //VBP
   SET_REG_WORD(lcdBase + LCD_V_ACTIVEHEIGHT_EVEN_OFFSET, vSize - 1);//AVH
   SET_REG_WORD(lcdBase + LCD_V_FRONTPORCH_EVEN_OFFSET,    vFrontPorch  - 1);   //VFP

   SET_REG_WORD(lcdBase + LCD_VSYNC_START_OFFSET_EVEN_OFFSET, 10);
   SET_REG_WORD(lcdBase + LCD_VSYNC_END_OFFSET_EVEN_OFFSET, 10);

   // image size
   SET_REG_WORD(lcdBase + LCD_SCREEN_WIDTH_OFFSET,   hSize - 1);    //Screen width
   SET_REG_WORD(lcdBase + LCD_SCREEN_HEIGHT_OFFSET,  vSize - 1);    //Screen height

}


/* ***********************************************************************//**
   *************************************************************************
LCD basic control
@{
----------------------------------------------------------------------------
@}
@param
    lcdBase      - base address of the lcd
                  .
@param
    background    - [0:7] red
                  - [8:15] green
                  - [16:24] blue
                  .
@param
    outFormat    - value to be written in the output format register,
                  .
@param
    ctrl          - value to be written in the control register
                  .
@return
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdCtrl // this means use ROM Version when available
void DrvLcdCtrl(u32 lcdBase, u32 background, u32 outFormat, u32 ctrl)
{
   SET_REG_WORD(lcdBase + LCD_BG_COLOUR_LS_OFFSET,      background);           //Background color:
   SET_REG_WORD(lcdBase + LCD_BG_COLOUR_MS_OFFSET,      0);                    // TBD need to update this 
   SET_REG_WORD(lcdBase + LCD_OUT_FORMAT_CFG_OFFSET, outFormat);
   SET_REG_WORD(lcdBase + LCD_CONTROL_OFFSET             , ctrl);           //interlaced, RGB-out, enable Layer2
   SET_REG_WORD(lcdBase + LCD_TIMING_GEN_TRIG_OFFSET,         1);           //enable timing to reach LCD(autoclears)
}


/* ***********************************************************************//**
   *************************************************************************
LCD Layer cfg
@{
----------------------------------------------------------------------------
@}
@param
     lcdBase- base address of the lcd
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
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdLayer0 // this means use ROM Version when available
void DrvLcdLayer0(u32 lcdBase, u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord)
{
  //General operation:
  SET_REG_WORD(lcdBase + LCD_LAYER0_WIDTH_OFFSET,     width-1);
  SET_REG_WORD(lcdBase + LCD_LAYER0_HEIGHT_OFFSET,    hight-1);
  SET_REG_WORD(lcdBase + LCD_LAYER0_COL_START_OFFSET, x);
  SET_REG_WORD(lcdBase + LCD_LAYER0_ROW_START_OFFSET, y);
  SET_REG_WORD(lcdBase + LCD_LAYER0_CFG_OFFSET,       cfgWord);
}


/* ***********************************************************************//**
   *************************************************************************
LCD Layer DMA cfg
@{
----------------------------------------------------------------------------
@}
@param:
   lcdBase                - base address of the lcd
@param:
    adr                   - address where the data for the layer resides
@param:
    width                 - width of the layer in pixels
@param:
    height                - height of the layer in pixels
@param:
    bytesPP               - number of BYTES/pixel (1, 2, 4)
@param:
    verticalStride        - number of bytes from the start of a line to the next line in the mem
@param:
    ctrl                  - word to be written in the CFG register
@return
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdDmaLayer0 // this means use ROM Version when available
 void DrvLcdDmaLayer0(u32 lcdBase,u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl)
{
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_ADR_OFFSET,    adr);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_SHADOW_OFFSET, adr);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_LEN_OFFSET,          width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_LEN_SHADOW_OFFSET,   width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_LINE_WIDTH_OFFSET,   width*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_LINE_VSTRIDE_OFFSET, verticalStride);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_CFG_OFFSET,          ctrl);
}
#pragma weak DrvLcdDmaLayer0Pp // this means use ROM Version when available
 void DrvLcdDmaLayer0Pp(u32 lcdBase,
		              u32 adr, u32 adrShadow,
			      u32 width, u32 height,
			      u32 bytesPP,
			      u32 verticalStride,
			      u32 ctrl)
{
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_ADR_OFFSET,    adr);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_SHADOW_OFFSET, adrShadow);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_LEN_OFFSET,          width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_LEN_SHADOW_OFFSET,   width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_LINE_WIDTH_OFFSET,   width*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_LINE_VSTRIDE_OFFSET, verticalStride);
  SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_CFG_OFFSET,          ctrl);
}

/* ***********************************************************************//**
   *************************************************************************
LCD Layer cfg
@{
----------------------------------------------------------------------------
@}
@param:
      lcdBase - base address of the lcd
@param:
      width  - width of the layer
@param:
      hight  - hight of the layer
@param:
      x - position of the layer
@param:
      y - position of the layer
@param:
      cfgWord - configuration word for the layer
@return
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdLayer1 // this means use ROM Version when available
void DrvLcdLayer1(u32 lcdBase, u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord)
{
  //General operation:
  SET_REG_WORD(lcdBase + LCD_LAYER1_WIDTH_OFFSET,     width-1);
  SET_REG_WORD(lcdBase + LCD_LAYER1_HEIGHT_OFFSET,    hight-1);
  SET_REG_WORD(lcdBase + LCD_LAYER1_COL_START_OFFSET, x);
  SET_REG_WORD(lcdBase + LCD_LAYER1_ROW_START_OFFSET, y);
  SET_REG_WORD(lcdBase + LCD_LAYER1_CFG_OFFSET,       cfgWord);
}
#pragma weak DrvLcdDmaLayer1Pp // this means use ROM Version when available
 void DrvLcdDmaLayer1Pp(u32 lcdBase, u32 adr1, u32 adr2,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl)
{
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_ADR_OFFSET,    adr1);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_SHADOW_OFFSET, adr2);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_LEN_OFFSET,          width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_LEN_SHADOW_OFFSET,   width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_LINE_WIDTH_OFFSET,   width*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_LINE_VSTRIDE_OFFSET, verticalStride);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_CFG_OFFSET,          ctrl);
}


/* ***********************************************************************//**
   *************************************************************************
LCD Layer DMA cfg
@{
--------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdDmaLayer1 // this means use ROM Version when available
 void DrvLcdDmaLayer1(u32 lcdBase, u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl)
{
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_ADR_OFFSET,    adr);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_SHADOW_OFFSET, adr);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_LEN_OFFSET,          width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_LEN_SHADOW_OFFSET,   width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_LINE_WIDTH_OFFSET,   width*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_LINE_VSTRIDE_OFFSET, verticalStride);
  SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_CFG_OFFSET,          ctrl);
}

/* ***********************************************************************//**
   *************************************************************************
LCD Layer cfg
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdLayer2 // this means use ROM Version when available
void DrvLcdLayer2(u32 lcdBase, u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord)
{
  //General operation:
  SET_REG_WORD(lcdBase + LCD_LAYER2_WIDTH_OFFSET,     width-1);
  SET_REG_WORD(lcdBase + LCD_LAYER2_HEIGHT_OFFSET,    hight-1);
  SET_REG_WORD(lcdBase + LCD_LAYER2_COL_START_OFFSET, x);
  SET_REG_WORD(lcdBase + LCD_LAYER2_ROW_START_OFFSET, y);
  SET_REG_WORD(lcdBase + LCD_LAYER2_CFG_OFFSET,       cfgWord);
}


/* ***********************************************************************//**
   *************************************************************************
LCD Layer DMA cfg
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdDmaLayer2 // this means use ROM Version when available
 void DrvLcdDmaLayer2(u32 lcdBase, u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 ctrl)
{
  SET_REG_WORD(lcdBase + LCD_LAYER2_DMA_START_ADR_OFFSET,    adr);
  SET_REG_WORD(lcdBase + LCD_LAYER2_DMA_START_SHADOW_OFFSET, adr);
  SET_REG_WORD(lcdBase + LCD_LAYER2_DMA_LEN_OFFSET,          width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER2_DMA_LEN_SHADOW_OFFSET,   width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER2_DMA_LINE_WIDTH_OFFSET,   width*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER2_DMA_LINE_VSTRIDE_OFFSET, verticalStride);
  SET_REG_WORD(lcdBase + LCD_LAYER2_DMA_CFG_OFFSET,          ctrl);
}

/* ***********************************************************************//**
   *************************************************************************
LCD Layer cfg
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdLayer3 // this means use ROM Version when available
void DrvLcdLayer3(u32 lcdBase, u32 width, u32 hight,
                    u32 x, u32 y,
                    u32 cfgWord)
{
  //General operation:
  SET_REG_WORD(lcdBase + LCD_LAYER3_WIDTH_OFFSET,     width-1);
  SET_REG_WORD(lcdBase + LCD_LAYER3_HEIGHT_OFFSET,    hight-1);
  SET_REG_WORD(lcdBase + LCD_LAYER3_COL_START_OFFSET, x);
  SET_REG_WORD(lcdBase + LCD_LAYER3_ROW_START_OFFSET, y);
  SET_REG_WORD(lcdBase + LCD_LAYER3_CFG_OFFSET,       cfgWord);
}


/* ***********************************************************************//**
   *************************************************************************
LCD Layer DMA cfg
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdDmaLayer3 // this means use ROM Version when available
 void DrvLcdDmaLayer3(u32 lcdBase, u32 adr,
                         u32 width, u32 height,
                         u32 bytesPP,
                         u32 verticalStride,
                         u32 dmaCtrl)
{
  SET_REG_WORD(lcdBase + LCD_LAYER3_DMA_START_ADR_OFFSET,    adr);
  SET_REG_WORD(lcdBase + LCD_LAYER3_DMA_START_SHADOW_OFFSET, adr);
  SET_REG_WORD(lcdBase + LCD_LAYER3_DMA_LEN_OFFSET,          width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER3_DMA_LEN_SHADOW_OFFSET,   width*height*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER3_DMA_LINE_WIDTH_OFFSET,   width*bytesPP);
  SET_REG_WORD(lcdBase + LCD_LAYER3_DMA_LINE_VSTRIDE_OFFSET, verticalStride);
  SET_REG_WORD(lcdBase + LCD_LAYER3_DMA_CFG_OFFSET,          dmaCtrl);
}

/* ***********************************************************************//**
   *************************************************************************
LCD PWM0 ctrl
@{
----------------------------------------------------------------------------
@}
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
@{
info:
     bits in the control register are defined in the DrvLcdDefines.h
@}
 ************************************************************************* */
 #pragma weak DrvLcdPwm0 // this means use ROM Version when available
void DrvLcdPwm0(u32 lcdBase, u32 ctrl, u32 repeat, u32 leadIn, u32 high, u32 low)
{
   SET_REG_WORD(lcdBase + LCD_PWM0_CTRL_OFFSET,        ctrl);
   SET_REG_WORD(lcdBase + LCD_PWM0_RPT_LEADIN_OFFSET, ((leadIn <<16) & 0xFFFF0000) | (repeat & 0xFFFF));
   SET_REG_WORD(lcdBase + LCD_PWM0_HIGH_LOW_OFFSET,   ((low     <<16) & 0xFFFF0000) | (high   & 0xFFFF));
}

/* ***********************************************************************//**
   *************************************************************************
LCD PWM1 ctrl
@{
----------------------------------------------------------------------------
@}
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
@{
info:
     bits in the control register are defined in the DrvLcdDefines.h
@}
 ************************************************************************* */
 #pragma weak DrvLcdPwm1 // this means use ROM Version when available
void DrvLcdPwm1(u32 lcdBase, u32 ctrl, u32 repeat, u32 leadIn, u32 high, u32 low)
{
   SET_REG_WORD(lcdBase + LCD_PWM1_CTRL_OFFSET,        ctrl);
   SET_REG_WORD(lcdBase + LCD_PWM1_RPT_LEADIN_OFFSET, ((leadIn <<16) & 0xFFFF0000) | (repeat & 0xFFFF));
   SET_REG_WORD(lcdBase + LCD_PWM1_HIGH_LOW_OFFSET,   ((low     <<16) & 0xFFFF0000) | (high   & 0xFFFF));
}

/* ***********************************************************************//**
   *************************************************************************
LCD PWM2 ctrl
@{
----------------------------------------------------------------------------
@}
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
@{
info:
     bits in the control register are defined in the DrvLcdDefines.h
@}
 ************************************************************************* */
 #pragma weak DrvLcdPwm2 // this means use ROM Version when available
void DrvLcdPwm2(u32 lcdBase, u32 ctrl, u32 repeat, u32 leadIn, u32 high, u32 low)
{
   SET_REG_WORD(lcdBase + LCD_PWM2_CTRL_OFFSET,        ctrl);
   SET_REG_WORD(lcdBase + LCD_PWM2_RPT_LEADIN_OFFSET, ((leadIn <<16) & 0xFFFF0000) | (repeat & 0xFFFF));
   SET_REG_WORD(lcdBase + LCD_PWM2_HIGH_LOW_OFFSET,   ((low     <<16) & 0xFFFF0000) | (high   & 0xFFFF));
}


/* ***********************************************************************//**
   *************************************************************************
LCD layer scale
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdVl1Scale // this means use ROM Version when available
void DrvLcdVl1Scale(u32 lcdBase, u32 hn, u32 hd, u32 vn, u32 vd)
{
   SET_REG_WORD(lcdBase + LCD_LAYER0_SCALE_CFG_OFFSET, (hn        & 0x1F   )|
                                          ((hd << 5  )& 0x3E0  )|
                                          ((vn << 10 )& 0x7C00 )|
                                          ((vd << 15 )& 0xF8000));
}


/* ***********************************************************************//**
   *************************************************************************
LCD layer scale
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdVl2Scale // this means use ROM Version when available
void DrvLcdVl2Scale(u32 lcdBase, u32 hn, u32 hd, u32 vn, u32 vd)
{
   SET_REG_WORD(lcdBase + LCD_LAYER1_SCALE_CFG_OFFSET, (hn        & 0x1F   )|
                                          ((hd << 5  )& 0x3E0  )|
                                          ((vn << 10 )& 0x7C00 )|
                                          ((vd << 15 )& 0xF8000));
}


/* ***********************************************************************//**
   *************************************************************************
LCD layer scale
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdGl1Scale // this means use ROM Version when available
void DrvLcdGl1Scale(u32 lcdBase, u32 hn, u32 hd, u32 vn, u32 vd)
{
   SET_REG_WORD(lcdBase + LCD_LAYER2_SCALE_CFG_OFFSET, (hn        & 0x1F   )|
                                          ((hd << 5  )& 0x3E0  )|
                                          ((vn << 10 )& 0x7C00 )|
                                          ((vd << 15 )& 0xF8000));
}


/* ***********************************************************************//**
   *************************************************************************
LCD layer scale
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdGl2Scale // this means use ROM Version when available
void DrvLcdGl2Scale(u32 lcdBase, u32 hn, u32 hd, u32 vn, u32 vd)
{
   SET_REG_WORD(lcdBase + LCD_LAYER3_SCALE_CFG_OFFSET, (hn        & 0x1F   )|
                                          ((hd << 5  )& 0x3E0  )|
                                          ((vn << 10 )& 0x7C00 )|
                                          ((vd << 15 )& 0xF8000));
}


/* ***********************************************************************//**
   *************************************************************************
LCD
@{
----------------------------------------------------------------------------
@}
@param
   lcdBase          - base address of the lcd
@return
@{
info:
       sample code of haw to programm the LCD using this functions
@}
 ************************************************************************* */
 #pragma weak DrvLcdDummyInit // this means use ROM Version when available
void DrvLcdDummyInit(u32 lcdBase)
{
// configure timings (NTSC size in this example)
    DrvLcdTiming(lcdBase,
                   /*width*/ 720 , /*sink*/ 9, /*backportch*/ 115, /*frontportch*/ 14,
                   /*height*/ 240, /*sink*/ 2, /*backportch*/  17, /*frontportch*/ 4 );

// enable video layer 1
    DrvLcdLayer0(lcdBase, 75, 60, 5, 5, D_LCD_LAYER_FIFO_50 |
                                  D_LCD_LAYER_32BPP   |
                                  D_LCD_LAYER_FORMAT_RGBA8888);

// enable video layer 2
    DrvLcdLayer1(lcdBase, 90, 50, 200, 5, D_LCD_LAYER_FIFO_50 |
                                    D_LCD_LAYER_32BPP   |
                                    D_LCD_LAYER_FORMAT_RGBA8888);

// configure , background to black, output format and basic config
   DrvLcdCtrl(lcdBase, 0x00000000 , D_LCD_OUTF_FORMAT_YCBCR444,
                 D_LCD_CTRL_OUTPUT_RGB       |
                 D_LCD_CTRL_TIM_GEN_ENABLE   |
                 D_LCD_CTRL_ALPHA_BOTTOM_GL2 |
                 D_LCD_CTRL_ALPHA_MIDDLE_GL1 |
                 D_LCD_CTRL_ALPHA_TOP_VL2    |
                 D_LCD_CTRL_ALPHA_BLEND_VL1  |
                 D_LCD_CTRL_ENABLE           |
                 D_LCD_CTRL_PROGRESSIVE      |
                 D_LCD_CTRL_VL1_ENABLE       |
                 D_LCD_CTRL_VL2_ENABLE);

// start DMA after the controller was started , or else ... (image is displaced)
    DrvLcdDmaLayer0(lcdBase, 0xA0000000, 75, 60, 4, 300, D_LCD_DMA_LAYER_ENABLE |
                                                    D_LCD_DMA_LAYER_AUTO_UPDATE |
                                                    D_LCD_DMA_LAYER_CONT_UPDATE |
                                                    D_LCD_DMA_LAYER_AXI_BURST_16);

    DrvLcdDmaLayer1(lcdBase, 0xA0050000, 90, 50, 4, 360, D_LCD_DMA_LAYER_ENABLE      |
                                                    D_LCD_DMA_LAYER_AUTO_UPDATE |
                                                    D_LCD_DMA_LAYER_CONT_UPDATE |
                                                    D_LCD_DMA_LAYER_AXI_BURST_16);



}


/* ***********************************************************************//**
   *************************************************************************
LCD csc for the layer
@{
----------------------------------------------------------------------------
@}
@param
   lcdBase          - base address of the lcd
@param
   layerNo - number of the layer that would use this csc coefficients, 0..3
@param
   table - a table that has the csc values in the folowing order: M11, M12, M13, M21, M22, ...., M32, M33, OFF1, OFF2, OFF3

@return
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdCsc // this means use ROM Version when available
void DrvLcdCsc(u32 lcdBase, u32 layerNo, u32 *table)
{
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_COEFF11_OFFSET, table[0]);
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_COEFF12_OFFSET, table[1]);
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_COEFF13_OFFSET, table[2]);
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_COEFF21_OFFSET, table[3]);
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_COEFF22_OFFSET, table[4]);
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_COEFF23_OFFSET, table[5]);
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_COEFF31_OFFSET, table[6]);
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_COEFF32_OFFSET, table[7]);
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_COEFF33_OFFSET, table[8]);
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_OFF1_OFFSET, table[9]);
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_OFF2_OFFSET, table[10]);
    SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_CSC_OFF3_OFFSET, table[11]);
}


/* ***********************************************************************//**
   *************************************************************************
LCD lut
@{
----------------------------------------------------------------------------
@}
@param
   lcdBase - base address of the lcd
@param
   layerNo - number of the layer that would use the LUT, 2..3
@param
   table - LUT entries
@param
   noEnt - number of entries

@return
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdLut // this means use ROM Version when available
void DrvLcdLut(u32 lcdBase, u32 layerNo, u32 *table, u32 noEnt)
{
     u32 i;
     if (layerNo == 2)
     { 
       for (i=0 ; i<noEnt ; ++i)
           SET_REG_WORD(lcdBase + LCD_LAYER2_CLUT0_OFFSET + 4*i, table[i]);
     } 
     else
     if (layerNo == 3)
     { 
       for (i=0 ; i<noEnt ; ++i)
           SET_REG_WORD(lcdBase + LCD_LAYER3_CLUT0_OFFSET + 4*i, table[i]);
     }           
}

/* ***********************************************************************//**
   *************************************************************************
LCD planar addressing mode addresses 
@{
----------------------------------------------------------------------------
@}
@param
   lcdBase - base address of the lcd
@param
   layerNo - number of the layer that would use the LUT, 0..1
@param   
   cbAddress - address for the cb
@param   
   crAddress - address for cr
@param   
   cbLineWidth - line width in bites
@param   
   cbVstride - vstride in bytes
@param   
   crLineWidth - line width in bites
@param   
   crVstride - vstride in bytes
@return
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdCbPlanar // this means use ROM Version when available
void DrvLcdCbPlanar(u32 lcdBase, u32 layerNo,
                       u32 address, u32 lineWidth, u32 vstride)
{
      if (layerNo == 0)
      {
         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_CB_ADR_OFFSET,    address);
         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_CB_SHADOW_OFFSET, address);

         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_CB_LINE_WIDTH_OFFSET,   lineWidth);
         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_CB_LINE_VSTRIDE_OFFSET, vstride);
      } else
      if (layerNo == 1)
      {
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_CB_ADR_OFFSET,    address);
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_CB_SHADOW_OFFSET, address);

         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_CB_LINE_WIDTH_OFFSET,   lineWidth);
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_CB_LINE_VSTRIDE_OFFSET, vstride);
      }
}

#pragma weak DrvLcdCrPlanar // this means use ROM Version when available
void DrvLcdCrPlanar(u32 lcdBase, u32 layerNo,
                       u32 address, u32 lineWidth, u32 vstride)
{
      if (layerNo == 0)
      {

         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_CR_ADR_OFFSET,    address);   
         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_CR_SHADOW_OFFSET, address); 

         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_CR_LINE_WIDTH_OFFSET,   lineWidth);  
         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_CR_LINE_VSTRIDE_OFFSET, vstride);
      } else
      if (layerNo == 1)
      {
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_CR_ADR_OFFSET,    address);   
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_CR_SHADOW_OFFSET, address); 

         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_CR_LINE_WIDTH_OFFSET,   lineWidth);  
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_CR_LINE_VSTRIDE_OFFSET, vstride);
      }
}
#pragma weak DrvLcdCbPlanarPp // this means use ROM Version when available
void DrvLcdCbPlanarPp(u32 lcdBase, u32 layerNo,
                       u32 address, u32 address1, u32 lineWidth, u32 vstride)
{
      if (layerNo == 0)
      {
         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_CB_ADR_OFFSET,    address);
         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_CB_SHADOW_OFFSET, address1);

         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_CB_LINE_WIDTH_OFFSET,   lineWidth);
         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_CB_LINE_VSTRIDE_OFFSET, vstride);
      } else
      if (layerNo == 1)
      {
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_CB_ADR_OFFSET,    address);
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_CB_SHADOW_OFFSET, address1);

         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_CB_LINE_WIDTH_OFFSET,   lineWidth);
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_CB_LINE_VSTRIDE_OFFSET, vstride);
      }
}

#pragma weak DrvLcdCrPlanarPp // this means use ROM Version when available
void DrvLcdCrPlanarPp(u32 lcdBase, u32 layerNo,
                       u32 address, u32 address1, u32 lineWidth, u32 vstride)
{
      if (layerNo == 0)
      {

         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_CR_ADR_OFFSET,    address);   
         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_START_CR_SHADOW_OFFSET, address1); 

         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_CR_LINE_WIDTH_OFFSET,   lineWidth);  
         SET_REG_WORD(lcdBase + LCD_LAYER0_DMA_CR_LINE_VSTRIDE_OFFSET, vstride);
      } else
      if (layerNo == 1)
      {
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_CR_ADR_OFFSET,    address);   
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_START_CR_SHADOW_OFFSET, address1); 

         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_CR_LINE_WIDTH_OFFSET,   lineWidth);  
         SET_REG_WORD(lcdBase + LCD_LAYER1_DMA_CR_LINE_VSTRIDE_OFFSET, vstride);
      }
}


/* ***********************************************************************//**
   *************************************************************************
LCD transparency pixel , and alpha belnding
@{
----------------------------------------------------------------------------
@}
@param
   lcdBase - base address of the lcd
@param
   layerNo - number of the layer
@param
   transpColor - color that is transparent
@param   
   alpha - alpha value for the layer 
   
@return
@{
info:
@}
 ************************************************************************* */
 // this is updated for fragrak
 
 #pragma weak DrvLcdTransparency // this means use ROM Version when available
void DrvLcdTransparency(u32 lcdBase, u32 layerNo, u32 transpColor, u32 alpha)
{
           SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_ALPHA_OFFSET, alpha);
           SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_TRANS_COLOUR_LS_OFFSET, transpColor); 
           SET_REG_WORD(lcdBase + layerNo*0x400 + LCD_LAYER0_TRANS_COLOUR_MS_OFFSET, 0);    // has to be updated         
}                                                 

/* ***********************************************************************//**
   *************************************************************************
LCD chnage position af a layer 
@{
----------------------------------------------------------------------------
@}
@param
   lcdBase - base address of the lcd
@param
   layerNo - number of the layer (0..3)
@param
   x - horisontal position
@param   
   y - vertical position
   
@return
@{
info:
@}
 ************************************************************************* */
 #pragma weak DrvLcdLayerPos // this means use ROM Version when available
void DrvLcdLayerPos(u32 lcdBase, u32 layerNo, u32 x, u32 y)
{
  SET_REG_WORD(lcdBase + 0x400*layerNo + LCD_LAYER0_COL_START_OFFSET, x);
  SET_REG_WORD(lcdBase + 0x400*layerNo + LCD_LAYER0_ROW_START_OFFSET, y);
}
