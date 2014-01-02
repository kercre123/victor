///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     CIF configuration low-level driver functionality
/// 
/// 
/// 

#ifndef _BRINGUP_SABRE_CIF_H_
#define _BRINGUP_SABRE_CIF_H_

#include "mv_types.h"
#include "DrvCifDefines.h"

/// Initialise the CIF interface
///
/// Must be called before using the relevant camera
/// @param cifDevice CAMERA_1 or CAMERA_2 to initialse the appropriate interface
void DrvCifInit(tyCIFDevice cifDevice);

/// Reset the targetted CIF Hardware block (handles any necessary reset delay)
///
/// @param cifDevice Device to reset (CAMERA_1 or CAMERA_2) 
void DrvCifReset(tyCIFDevice cifDevice);

/// Set the camera output MCLK frequency
/// @param cifDevice CAMERA_1 or CAMERA_2
/// @param targetFrequencyKhz target Frequency in Khz
void DrvCifSetMclkFrequency(tyCIFDevice cifDevice,u32 targetFrequencyKhz);

/*cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR   */
/* ***********************************************************************//**
   *************************************************************************
CIF set csc

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

 ************************************************************************* */
void DrvCifSetCsc(u32 cifBase, u32 *cscCoefs);

/* ***********************************************************************//**
   *************************************************************************
CIF get csc

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

 ************************************************************************* */
void DrvCifGetCsc(u32 cifBase, u32 *cscCoefs);

/* ***********************************************************************//**
   *************************************************************************
CIF get crc values

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

 ************************************************************************* */
void DrvCifSetCrc(u32 cifBase, u32 *crcCoefs);

/* ***********************************************************************//**
   *************************************************************************
CIF get crc values

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

 ************************************************************************* */
void DrvCifGetCrc(u32 cifBase, u32 *crcCoefs);

/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg

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

 ************************************************************************* */
void DrvCifDma3Cfg(u32 cifBase,
                      u32 dataBufAdr,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride);
					  

/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg

@param
     cifBase      - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     dataBufAdr1   - address where cif should store data
@param
     dataBufAdr2  - secondary address where cif should store data(double buffering)
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

 ************************************************************************* */
void DrvCifDma3CfgPP(u32 cifBase,
                      u32 dataBufAdr1, u32 dataBufAdr2,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride);
 
/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg

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

 ************************************************************************* */					  
void DrvCifDma2Cfg(u32 cifBase,
                      u32 dataBufAdr,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride);

/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg

@param
     cifBase      - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     dataBufAdr   - address where cif should store data
@param
     dataBufAdrShadow - shadow buffer address where cif should store data(double buffering)
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

 ************************************************************************* */
void DrvCifDma2CfgPP(u32 cifBase,
                      u32 dataBufAdr, u32 dataBufAdrShadow,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride);
					  
/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg

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

 ************************************************************************* */
void DrvCifDma1Cfg(u32 cifBase,
                      u32 dataBufAdr,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride);

/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg

@param
     cifBase      - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     dataBufAdr   - address where cif should store data
@param
     dataBufAdrShadow - shadow buffer address where cif should store data(double buffering)
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

 ************************************************************************* */
void DrvCifDma1CfgPP(u32 cifBase,
                      u32 dataBufAdr, u32 dataBufAdrShadow,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride);

/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg

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

 ************************************************************************* */					  
void DrvCifDma0Cfg(u32 cifBase,
                      u32 dataBufAdr,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride);
					  
/* ***********************************************************************//**
   *************************************************************************
CIF dma cfg

@param
     cifBase      - base address for cif1 or cif2
                    cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                    
     dataBufAdr   - address where cif should store data
@param
     dataBufAdrShadow - shadow buffer address where cif should store data(double buffering)
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

 ************************************************************************* */
void DrvCifDma0CfgPP(u32 cifBase,
                      u32 dataBufAdr, u32 dataBufAdrShadow,
                      u32 width,u32 height,
                      u32 bytesPP,
                      u32 control,
                      u32 stride);
					  
/* ***********************************************************************//**
   *************************************************************************
CIF timing config 

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

 ************************************************************************* */
void DrvCifTimingCfg(u32 cifBase,
                        u32 width, u32 height,
                        u32 hBackP, u32 hFrontP,
                        u32 vBackP, u32 vFrontP);
						
/* ***********************************************************************//**
   *************************************************************************
CIF in out config

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

 ************************************************************************* */
void DrvCifInOutCfg(u32 cifBase,
                        u32 inputFormat,u32 inputInterface,
                        u32 outputCfg,
                        u32 previewCfg);

/* ***********************************************************************//**
   *************************************************************************
CIF config 

@param
     cifBase  - base address for cif1 or cif2
                  cifBase is one of this  MXI_CAM1_BASE_ADR, MXI_CAM2_BASE_ADR
@param                  
     ctrl      - word to be written in the control register 
@param     
    width      - witdh of the image [pixels]
@param    
    height     - height of the image  [pixels]

 ************************************************************************* */
void DrvCifCtrlCfg(u32 cifBase,
                        u32 width,
                        u32 height,                       
                        u32 ctrl);
						
/* ***********************************************************************//**
   *************************************************************************
CIF pwm 0 cfg

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

 ************************************************************************* */
void DrvCifPwm0Ctrl(u32 cifBase,
                      u32 ctrl, 
                      u32 repeatCnt, u32 leadIn,
                      u32 highTime,  u32 lowTime);
					  
/* ***********************************************************************//**
   *************************************************************************
CIF pwm 1 cfg

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

 ************************************************************************* */
void DrvCifPwm1Ctrl(u32 cifBase,
                       u32 ctrl, 
                       u32 repeatCnt, u32 leadIn,
                       u32 highTime,  u32 lowTime);
					   
/* ***********************************************************************//**
   *************************************************************************
CIF pwm 2 cfg

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

 ************************************************************************* */					   
void DrvCIFPwm2Ctrl(u32 cifBase,
                      u32 ctrl, 
                      u32 repeatCnt, u32 leadIn,
                      u32 highTime,  u32 lowTime);
					  
/* ***********************************************************************//**
   *************************************************************************
CIF  windowing 

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

 ************************************************************************* */
void DrvCifWindowing(u32 cifBase, u32 rowStart, u32 colStart, u32 width, u32 height);

/* ***********************************************************************//**
   *************************************************************************
CIF scale

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

 ************************************************************************* */
void DrvCifScale(u32 cifBase, u32 hn, u32 hd, u32 vn, u32 vd);
#endif

