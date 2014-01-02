#include <mv_types.h>
#include <isaac_registers.h>
#include "DrvCif.h"
#include "DrvCpr.h"
#include "stdio.h"
#include "assert.h"

void DrvCifInit(tyCIFDevice cifDevice)
{
    if (cifDevice == CAMERA_1)
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_CIF1);

    if (cifDevice == CAMERA_2)
        DrvCprSysDeviceAction(ENABLE_CLKS, DEV_CIF2);

    return;
}

void DrvCifReset(tyCIFDevice cifDevice)
{
    if (cifDevice == CAMERA_1)
        DrvCprSysDeviceAction(RESET_DEVICES, DEV_CIF1);

    if (cifDevice == CAMERA_2)
        DrvCprSysDeviceAction(RESET_DEVICES, DEV_CIF2);

    return;
}

u32 GreatestCommonDivisor(u32 first, u32 second)
{
	//Doing it through repetitive subtractions
	if (first==0){
		return 0;
	}else{
		while (second != 0){
			if (first>second){
				first=first-second;
			}else{
				second=second-first;
			}
		}
	}
	return first;
}

u32  GCD;
u32  targetFreqToSet;
u32  pllClockFreqToUse;

void DrvCifSetMclkFrequency(tyCIFDevice cifDevice,u32 targetFrequencyKhz)
{
    u32  auxClockSelect=AUX_CLK_MASK_CIF1;
    u32  pllClockFrequencyKhz;   
    if (cifDevice == CAMERA_1)
        auxClockSelect = AUX_CLK_MASK_CIF1;
    else if (cifDevice == CAMERA_2)
        auxClockSelect = AUX_CLK_MASK_CIF2;
    else
    {
        assert(FALSE);
    }

    pllClockFrequencyKhz = DrvCprGetClockFreqKhz(PLL_CLK,NULL);

    // Most likely cause of this assert failing is that DrvCprInit was never called
    assert(pllClockFrequencyKhz >= 12);

    GCD=GreatestCommonDivisor(targetFrequencyKhz,pllClockFrequencyKhz);
    targetFreqToSet=targetFrequencyKhz/GCD;
    pllClockFreqToUse=pllClockFrequencyKhz/GCD;

    //Just as a failsafe, to ensure no extremely bad fails happen
    //Do a double check of how big the value of both nominator and denominator are and if any is over
    //The maximum value (GCD was too small) use the old "jittery" mode. This should not happen
    //often though. Indeed it will probably not happen at all since if 2 is GCD there's already almost no chance
    //They wouldn't fit the register. And all values will have 2 and 5 or 3 as common divisors
    if ( (targetFreqToSet>0xFFFF) || (pllClockFreqToUse>0xFFFF) ){
    	DrvCprAuxClocksEnable(auxClockSelect, (targetFrequencyKhz / 1000), (pllClockFrequencyKhz/1000));
    }else{
    	DrvCprAuxClocksEnable(auxClockSelect, targetFreqToSet, pllClockFreqToUse);
    }

    return;
}

/* ***********************************************************************//**
   *************************************************************************
CIF set csc
@{
----------------------------------------------------------------------------
@}
@param
     cifBase     - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     cscCoefs    - pointer to an array of 16 values,
                    which will be placed in the csc in this order: 
                    csc11, csc12, csc13, 
                    csc21, csc22, csc23, 
                    csc31, csc32, csc33, 
                    csc41, csc42, csc43, 
                    cscoff1, cscoff2, cscoff3, cscoff4
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifSetCsc(u32 cifBase, u32 *cscCoefs)
{
    int i;
    for (i=0;i<16;i++)
    {
        SET_REG_WORD(cifBase + CIF_CSC_COEFF11_OFFSET + i*8 , (*cscCoefs) & 0xFFF );
        ++cscCoefs;
    }
}


/* ***********************************************************************//**
   *************************************************************************
CIF get csc
@{
----------------------------------------------------------------------------
@}
@param
     cifBase     - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     cscCoefs    - pointer to an array of 16 values,
                    which will be written with the csc coefs in this order: 
                    csc11, csc12, csc13, 
                    csc21, csc22, csc23, 
                    csc31, csc32, csc33, 
                    csc41, csc42, csc43, 
                    cscoff1, cscoff2, cscoff3, cscoff4
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifGetCsc(u32 cifBase, u32 *cscCoefs)
{
    int i;
    for (i=0;i<16;i++)
    {
        *cscCoefs = GET_REG_WORD_VAL(cifBase + CIF_CSC_COEFF11_OFFSET + i*8) & 0xFFF;
        ++cscCoefs;
    }
}

/* ***********************************************************************//**
   *************************************************************************
CIF set crc values
@{
----------------------------------------------------------------------------
@}
@param
     cifBase     - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     crcCoefs    - pointer to an array of 12 values,
                    which will be written in the crc registers in this order: 
                    crc11, crc12, crc13, 
                    crc21, crc22, crc23, 
                    crc31, crc32, crc33, 
                    crcoff1, crcoff2, crcoff3
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifSetCrc(u32 cifBase, u32 *crcCoefs)
{
    int i;
    for (i=0;i<12;i++)
    {
        SET_REG_WORD(cifBase + CIF_CCR_COEFF11_OFFSET + i*8 , (*crcCoefs) & 0xFFF );
        ++crcCoefs;
    }
}

/* ***********************************************************************//**
   *************************************************************************
CIF get crc values
@{
----------------------------------------------------------------------------
@}
@param
     cifBase     - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     crcCoefs    - pointer to an array of 12 values,
                    which will be written with the crc coefs in this order: 
                    crc11, crc12, crc13, 
                    crc21, crc22, crc23, 
                    crc31, crc32, crc33, 
                    crcoff1, crcoff2, crcoff3
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifGetCrc(u32 cifBase, u32 *crcCoefs)
{
    int i;
    for (i=0;i<12;i++)
    {
        *crcCoefs = GET_REG_WORD_VAL(cifBase + CIF_CCR_COEFF11_OFFSET + i*8) & 0xFFF;
        ++crcCoefs;
        
    }
}

/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg
@{
----------------------------------------------------------------------------
@}
@param
     cifBase      - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     dataBufAdr   - address where cif should store data
@param     
     width        -  width of the image provided by the cif [pixels]
@param     
     height       -  height of the image provided by the cif [pixels]
@param     
     bytesPP      -  number of bytes per pixel
@param     
     control      -  word to be written in the control register
@param     
     stride       -  offset in memory between the start of a line and the start of the next one [bytes]                   
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifDma3Cfg(u32 cifBase,
                      u32 dataBufAdr,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride)
{
   SET_REG_WORD(cifBase + CIF_DMA3_START_ADR_OFFSET,    dataBufAdr);
   SET_REG_WORD(cifBase + CIF_DMA3_START_SHADOW_OFFSET, dataBufAdr);
   
   SET_REG_WORD(cifBase + CIF_DMA3_LEN_OFFSET,          width*height*bytesPP);//CAM_F_W*CAM_F_H*2); //???
   SET_REG_WORD(cifBase + CIF_DMA3_LEN_SHADOW_OFFSET,   width*height*bytesPP);//CAM_F_W*CAM_F_H*2);
   
   SET_REG_WORD(cifBase + CIF_DMA3_LINE_WIDTH_OFFSET,   width*bytesPP);
   SET_REG_WORD(cifBase + CIF_DMA3_LINE_OFF_OFFSET  ,   stride);
   
   SET_REG_WORD(cifBase + CIF_DMA3_CFG_OFFSET,          control);//0x20D);
}



/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg
@{
----------------------------------------------------------------------------
@}
@param
     cifBase      - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     dataBufAdr1  - address where cif should store data
@param                    
     dataBufAdr2  - address where cif should store data, shadow
@param     
     width        -  width of the image provided by the cif [pixels]
@param     
     height       -  height of the image provided by the cif [pixels]
@param     
     bytesPP      -  number of bytes per pixel
@param     
     control      -  word to be written in the control register
@param     
     stride       -  offset in memory between the start of a line and the start of the next one [bytes]                   
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifDma3CfgPP(u32 cifBase,
                      u32 dataBufAdr1, u32 dataBufAdr2,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride)
{
   SET_REG_WORD(cifBase + CIF_DMA3_START_ADR_OFFSET,    dataBufAdr1);
   SET_REG_WORD(cifBase + CIF_DMA3_START_SHADOW_OFFSET, dataBufAdr2);
   
   SET_REG_WORD(cifBase + CIF_DMA3_LEN_OFFSET,          width*height*bytesPP);//CAM_F_W*CAM_F_H*2); //???
   SET_REG_WORD(cifBase + CIF_DMA3_LEN_SHADOW_OFFSET,   width*height*bytesPP);//CAM_F_W*CAM_F_H*2);
   
   SET_REG_WORD(cifBase + CIF_DMA3_LINE_WIDTH_OFFSET,   width*bytesPP);
   SET_REG_WORD(cifBase + CIF_DMA3_LINE_OFF_OFFSET  ,   stride);
   
   SET_REG_WORD(cifBase + CIF_DMA3_CFG_OFFSET,          control);//0x20D);
}


/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg
@{
----------------------------------------------------------------------------
@}
@param
     cifBase      - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     dataBufAdr   - address where cif should store data
@param     
     width        -  width of the image provided by the cif [pixels]
@param     
     height       -  height of the image provided by the cif [pixels]
@param     
     bytesPP      -  number of bytes per pixel
@param     
     control      -  word to be written in the control register
@param     
     stride       -  offset in memory between the start of a line and the start of the next one [bytes]                   
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifDma2Cfg(u32 cifBase,
                      u32 dataBufAdr,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride)
{
   SET_REG_WORD(cifBase + CIF_DMA2_START_ADR_OFFSET,    dataBufAdr);
   SET_REG_WORD(cifBase + CIF_DMA2_START_SHADOW_OFFSET, dataBufAdr);
   
   SET_REG_WORD(cifBase + CIF_DMA2_LEN_OFFSET,          width*height*bytesPP);//CAM_F_W*CAM_F_H*2); //???
   SET_REG_WORD(cifBase + CIF_DMA2_LEN_SHADOW_OFFSET,   width*height*bytesPP);//CAM_F_W*CAM_F_H*2);
   
   SET_REG_WORD(cifBase + CIF_DMA2_LINE_WIDTH_OFFSET,   width*bytesPP);
   SET_REG_WORD(cifBase + CIF_DMA2_LINE_OFF_OFFSET  ,   stride);
   
   SET_REG_WORD(cifBase + CIF_DMA2_CFG_OFFSET,          control);//0x20D);
}

void DrvCifDma2CfgPP(u32 cifBase,
                      u32 dataBufAdr, u32 dataBufAdrShadow,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride)
{
   SET_REG_WORD(cifBase + CIF_DMA2_START_ADR_OFFSET,    dataBufAdr);
   SET_REG_WORD(cifBase + CIF_DMA2_START_SHADOW_OFFSET, dataBufAdrShadow);
   
   SET_REG_WORD(cifBase + CIF_DMA2_LEN_OFFSET,          width*height*bytesPP);//CAM_F_W*CAM_F_H*2); //???
   SET_REG_WORD(cifBase + CIF_DMA2_LEN_SHADOW_OFFSET,   width*height*bytesPP);//CAM_F_W*CAM_F_H*2);
   
   SET_REG_WORD(cifBase + CIF_DMA2_LINE_WIDTH_OFFSET,   width*bytesPP);
   SET_REG_WORD(cifBase + CIF_DMA2_LINE_OFF_OFFSET  ,   stride);
   
   SET_REG_WORD(cifBase + CIF_DMA2_CFG_OFFSET,          control);//0x20D);
}

/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg
@{
----------------------------------------------------------------------------
@}
@param
     cifBase      - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     dataBufAdr   - address where cif should store data
@param     
     width        -  width of the image provided by the cif [pixels]
@param     
     height       -  height of the image provided by the cif [pixels]
@param     
     bytesPP      -  number of bytes per pixel
@param     
     control      -  word to be written in the control register
@param     
     stride       -  offset in memory between the start of a line and the start of the next one [bytes]                   
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifDma1Cfg(u32 cifBase,
                      u32 dataBufAdr,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride)
{
   SET_REG_WORD(cifBase + CIF_DMA1_START_ADR_OFFSET,    dataBufAdr);
   SET_REG_WORD(cifBase + CIF_DMA1_START_SHADOW_OFFSET, dataBufAdr);
   
   SET_REG_WORD(cifBase + CIF_DMA1_LEN_OFFSET,          width*height*bytesPP);//CAM_F_W*CAM_F_H*2); //???
   SET_REG_WORD(cifBase + CIF_DMA1_LEN_SHADOW_OFFSET,   width*height*bytesPP);//CAM_F_W*CAM_F_H*2);
   
   SET_REG_WORD(cifBase + CIF_DMA1_LINE_WIDTH_OFFSET,   width*bytesPP);
   SET_REG_WORD(cifBase + CIF_DMA1_LINE_OFF_OFFSET  ,   stride);
   
   SET_REG_WORD(cifBase + CIF_DMA1_CFG_OFFSET,          control);//0x20D);
}

void DrvCifDma1CfgPP(u32 cifBase,
                      u32 dataBufAdr, u32 dataBufAdrShadow,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride)
{
   SET_REG_WORD(cifBase + CIF_DMA1_START_ADR_OFFSET,    dataBufAdr);
   SET_REG_WORD(cifBase + CIF_DMA1_START_SHADOW_OFFSET, dataBufAdrShadow);
   
   SET_REG_WORD(cifBase + CIF_DMA1_LEN_OFFSET,          width*height*bytesPP);//CAM_F_W*CAM_F_H*2); //???
   SET_REG_WORD(cifBase + CIF_DMA1_LEN_SHADOW_OFFSET,   width*height*bytesPP);//CAM_F_W*CAM_F_H*2);
   
   SET_REG_WORD(cifBase + CIF_DMA1_LINE_WIDTH_OFFSET,   width*bytesPP);
   SET_REG_WORD(cifBase + CIF_DMA1_LINE_OFF_OFFSET  ,   stride);
   
   SET_REG_WORD(cifBase + CIF_DMA1_CFG_OFFSET,          control);//0x20D);
}

/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg
@{
----------------------------------------------------------------------------
@}
@param
     cifBase      - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     dataBufAdr   - address where cif should store data
@param     
     width        -  width of the image provided by the cif [pixels]
@param     
     height       -  height of the image provided by the cif [pixels]
@param     
     bytesPP      -  number of bytes per pixel
@param     
     control      -  word to be written in the control register
@param     
     stride       -  offset in memory between the start of a line and the start of the next one [bytes]                   
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifDma0Cfg(u32 cifBase,
                      u32 dataBufAdr,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride)
{
   SET_REG_WORD(cifBase + CIF_DMA0_START_ADR_OFFSET,    dataBufAdr);
   SET_REG_WORD(cifBase + CIF_DMA0_START_SHADOW_OFFSET, dataBufAdr);
   
   SET_REG_WORD(cifBase + CIF_DMA0_LEN_OFFSET,          width*height*bytesPP);//CAM_F_W*CAM_F_H*2); //???
   SET_REG_WORD(cifBase + CIF_DMA0_LEN_SHADOW_OFFSET,   width*height*bytesPP);//CAM_F_W*CAM_F_H*2);
   
   SET_REG_WORD(cifBase + CIF_DMA0_LINE_WIDTH_OFFSET,   width*bytesPP);
   SET_REG_WORD(cifBase + CIF_DMA0_LINE_OFF_OFFSET  ,   stride);
   
   SET_REG_WORD(cifBase + CIF_DMA0_CFG_OFFSET,          control);//0x20D);
}

void DrvCifDma0CfgPP(u32 cifBase,
                      u32 dataBufAdr, u32 dataBufAdrShadow,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride)
{
   SET_REG_WORD(cifBase + CIF_DMA0_START_ADR_OFFSET,    dataBufAdr);
   SET_REG_WORD(cifBase + CIF_DMA0_START_SHADOW_OFFSET, dataBufAdrShadow);
   
   SET_REG_WORD(cifBase + CIF_DMA0_LEN_OFFSET,          width*height*bytesPP);//CAM_F_W*CAM_F_H*2); //???
   SET_REG_WORD(cifBase + CIF_DMA0_LEN_SHADOW_OFFSET,   width*height*bytesPP);//CAM_F_W*CAM_F_H*2);
   
   SET_REG_WORD(cifBase + CIF_DMA0_LINE_WIDTH_OFFSET,   width*bytesPP);
   SET_REG_WORD(cifBase + CIF_DMA0_LINE_OFF_OFFSET  ,   stride);
   
   SET_REG_WORD(cifBase + CIF_DMA0_CFG_OFFSET,          control);//0x20D);
}
/* ***********************************************************************//**
   *************************************************************************
CIF timing config 
@{
----------------------------------------------------------------------------\
@}
@param
     cifBase      - base address for cif1 or cif2
                     cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                     
        width      - witdh of the image [cc]
@param        
        height     - height of the image  [cc]
@param        
        hBackP     - horizontal back porch [cc]
@param        
        hFrontP    - horizontal front porch [cc]
@param        
        vBackP     - vertical back porch  [cc]
@param        
        vFrontP    - vertical front porch [cc]
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifTimingCfg(u32 cifBase,
                        u32 width, u32 height,
                        u32 hBackP, u32 hFrontP,
                        u32 vBackP, u32 vFrontP)
{
   SET_REG_WORD(cifBase + CIF_H_BACKPORCH_OFFSET  ,      hBackP);
   SET_REG_WORD(cifBase + CIF_H_ACTIVEWIDTH_OFFSET,      width-1);
   SET_REG_WORD(cifBase + CIF_H_FRONTPORCH_OFFSET ,      hFrontP);
   
   SET_REG_WORD(cifBase + CIF_V_BACKPORCH_OFFSET   ,     vBackP);
   SET_REG_WORD(cifBase + CIF_V_ACTIVEHEIGHT_OFFSET,     height-1);
   SET_REG_WORD(cifBase + CIF_V_FRONTPORCH_OFFSET  ,     vFrontP);
}


/* ***********************************************************************//**
   *************************************************************************
CIF in out config
@{
----------------------------------------------------------------------------
@}
@param
     cifBase          - base address for cif1 or cif2
                        cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                        
     inputFormat      - word to be written in the input format register
@param     
     inputInterface   - word to be writen in the input interface register
@param     
     outputCfg        - word to be written in the output config register
@param     
     previewCfg       - word to be written in the preview configuration
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifInOutCfg(u32 cifBase,
                        u32 inputFormat,u32 inputInterface,
                        u32 outputCfg,
                        u32 previewCfg)
{
   SET_REG_WORD(cifBase + CIF_INPUT_FORMAT_OFFSET,  inputFormat    );
   SET_REG_WORD(cifBase + CIF_INPUT_IF_CFG_OFFSET,  inputInterface );
   SET_REG_WORD(cifBase + CIF_OUTPUT_CFG_OFFSET  ,  outputCfg      );
   SET_REG_WORD(cifBase + CIF_PREVIEW_CFG_OFFSET ,  previewCfg     );
}


/* ***********************************************************************//**
   *************************************************************************
CIF config 
@{
----------------------------------------------------------------------------
@}
@param
     cifBase  - base address for cif1 or cif2
                  cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                  
     ctrl      - word to be written in the control register 
@param     
    width      - witdh of the image [pixels]
@param    
    height     - height of the image  [pixels]
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifCtrlCfg(u32 cifBase,
                        u32 width,
                        u32 height,                       
                        u32 ctrl)
{
   SET_REG_WORD(cifBase + CIF_FRAME_WIDTH_OFFSET  ,      width-1);
   SET_REG_WORD(cifBase + CIF_FRAME_HEIGHT_OFFSET ,     height-1);

   SET_REG_WORD(cifBase + CIF_CONTROL_OFFSET      ,         ctrl);
}

/* ***********************************************************************//**
   *************************************************************************
CIF pwm 0 cfg
@{
----------------------------------------------------------------------------
@}
@param
     cifBase  - base address for cif1 or cif2
                  cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                  
         ctrl  - word to be written in the control register of the pwm channel
@param         
   repeatCnt   - number of times the wave will be repeated
@param   
      leadIn   - number of pix clk cicles until the signal goes high
@param      
    highTime   - number of pix clk cicles during the signal is high
@param    
     lowTime   - number of pix clk cicles during the signal is low
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifPwm0Ctrl(u32 cifBase,
                      u32 ctrl, 
                      u32 repeatCnt, u32 leadIn,
                      u32 highTime,  u32 lowTime)
{
   SET_REG_WORD(cifBase + CIF_PWM0_CTRL_OFFSET      , ctrl);
   SET_REG_WORD(cifBase + CIF_PWM0_RPT_LEADIN_OFFSET, (repeatCnt & 0xFFFF) | ((leadIn<<16 ) & 0xFFFF0000));
   SET_REG_WORD(cifBase + CIF_PWM0_HIGH_LOW_OFFSET  ,  (highTime & 0xFFFF) | ((lowTime<<16) & 0xFFFF0000));
}

/* ***********************************************************************//**
   *************************************************************************
CIF pwm 1 cfg
@{
----------------------------------------------------------------------------
@}
@param
     cifBase  - base address for cif1 or cif2
                  cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                  
         ctrl  - word to be written in the control register of the pwm channel
@param         
   repeatCnt   - number of times the wave will be repeated
@param   
      leadIn   - number of pix clk cicles until the signal goes high
@param      
    highTime   - number of pix clk cicles during the signal is high
@param    
     lowTime   - number of pix clk cicles during the signal is low
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifPwm1Ctrl(u32 cifBase,
                       u32 ctrl, 
                       u32 repeatCnt, u32 leadIn,
                       u32 highTime,  u32 lowTime)
{
   SET_REG_WORD(cifBase + CIF_PWM1_CTRL_OFFSET      , ctrl);
   SET_REG_WORD(cifBase + CIF_PWM1_RPT_LEADIN_OFFSET, (repeatCnt & 0xFFFF) | ((leadIn<<16 ) & 0xFFFF0000));
   SET_REG_WORD(cifBase + CIF_PWM1_HIGH_LOW_OFFSET  ,  (highTime & 0xFFFF) | ((lowTime<<16) & 0xFFFF0000));
}

/* ***********************************************************************//**
   *************************************************************************
CIF pwm 2 cfg
@{
----------------------------------------------------------------------------
@{
@param
     cifBase   - base address for cif1 or cif2
                  cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                  
         ctrl  - word to be written in the control register of the pwm channel
@param         
   repeatCnt   - number of times the wave will be repeated
@param   
      leadIn   - number of pix clk cicles until the signal goes high
@param      
    highTime   - number of pix clk cicles during the signal is high
@param    
     lowTime   - number of pix clk cicles during the signal is low
@return
@{
info:
@}
 ************************************************************************* */
void DrvCIFPwm2Ctrl(u32 cifBase,
                      u32 ctrl, 
                      u32 repeatCnt, u32 leadIn,
                      u32 highTime,  u32 lowTime)
{
   SET_REG_WORD(cifBase + CIF_PWM2_CTRL_OFFSET      , ctrl);
   SET_REG_WORD(cifBase + CIF_PWM2_RPT_LEADIN_OFFSET, (repeatCnt & 0xFFFF) | ((leadIn<<16 ) & 0xFFFF0000));
   SET_REG_WORD(cifBase + CIF_PWM2_HIGH_LOW_OFFSET  ,  (highTime & 0xFFFF) | ((lowTime<<16) & 0xFFFF0000));
}

/* ***********************************************************************//**
   *************************************************************************
CIF dummy inint function
@{
----------------------------------------------------------------------------
@}
@param
     cifBase  - base address for cif1 or cif2
                  cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@return
@{
info:
@}
 ************************************************************************* */
void drvCifDummyInit(u32 cifBase)
{
   DrvCifDma3Cfg(cifBase,
                    0xA0000000 /* address where the data will be placed*/, 
                           320 /*width in cc*/, 
                           240 /*height in cc*/,
                             2 /*bpp*/, 
         D_CIF_DMA_V_STRIDE_EN             |  /*control word*/
         D_CIF_DMA_AUTO_RESTART_CONTINUOUS |
         D_CIF_DMA_ENABLE,
                          320*2 /*stride*/ );

   DrvCifInOutCfg(cifBase,
                    D_CIF_INFORM_FORMAT_RGB_BAYER |  /*inputFormat*/
                    D_CIF_INFORM_BAYER_GRGR_BGBG  |
                    D_CIF_INFORM_DEMOSAIC_7_0,
                    D_CIF_IN_PIX_CLK_SENSOR  /*inputInterface*/,
                    0x0000                   /*outputCfg*/,
                    D_CIF_PREV_OUTF_RGB565     /*previewCfg*/);

   DrvCifTimingCfg(cifBase,
                       320 /*width*/, 
                       240 /*height*/,
                       0 /*hBackP*/,
                       0 /*hFrontP*/,
                       0 /*vBackP*/,
                       0 /*vFrontP*/);
 
   DrvCifCtrlCfg( cifBase ,
                   320 /*width*/,
                   240 /*height*/,                       
                   D_CIF_CTRL_ENABLE |
                   D_CIF_CTRL_RGB_BAYER_EN);
}

/* ***********************************************************************//**
   *************************************************************************
CIF  windowing 
@{
----------------------------------------------------------------------------
@}
@param
    cifBase -      cifBase  - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
    rowStart  - window starts on this row
@param    
    colStart  - window starts on this column
@param    
    width - with of the window
@param    
    height - height of the window
@return
@{
info:
@}
 ************************************************************************* */
void DrvCifWindowing(u32 cifBase, u32 rowStart, u32 colStart, u32 width, u32 height)
{
   SET_REG_WORD(cifBase + CIF_WIN_ROW_START_OFFSET, rowStart);
   SET_REG_WORD(cifBase + CIF_WIN_COL_START_OFFSET, colStart);
   SET_REG_WORD(cifBase + CIF_WIN_WIDTH_OFFSET    , width - 1);
   SET_REG_WORD(cifBase + CIF_WIN_HEIGHT_OFFSET   , height -1);
}


/* ***********************************************************************//**
   *************************************************************************
CIF scale
@{
----------------------------------------------------------------------------
@}
@param
    cifBase -      cifBase  - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
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
void DrvCifScale(u32 cifBase, u32 hn, u32 hd, u32 vn, u32 vd)
{
   SET_REG_WORD(cifBase + CIF_SUBSAMPLE_CFG_OFFSET, (hn        & 0x1F   )|
                                                    ((hd << 5  )& 0x3E0  )|
                                                    ((vn << 10 )& 0x7C00 )|
                                                    ((vd << 15 )& 0xF8000));
}

void drvCifPrevScale(u32 cifBase, u32 hn, u32 hd, u32 vn, u32 vd)
{
   SET_REG_WORD(cifBase + CIF_PREVIEW_SUBS_CFG_OFFSET, (hn        & 0x1F   )|
                                                    ((hd << 5  )& 0x3E0  )|
                                                    ((vn << 10 )& 0x7C00 )|
                                                    ((vd << 15 )& 0xF8000));
}
