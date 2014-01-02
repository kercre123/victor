///  
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for the ROIC Module
/// 

#ifndef DRV_ROIC_H
#define DRV_ROIC_H 

#include "DrvRoicDefines.h"


/* ***********************************************************************//**
   *************************************************************************
DrvRoicInitCmd - initialise the dma channnel which will copy data to the CMD memory in the ROIC controller
@{
----------------------------------------------------------------------------
@}
@param
      commandAddress  - address in the memory where command data is stored
@param      
      commandLength - length in 32bit words 
@param      
      dmaCh - which DMA channel to be used for this 
@return
      void 
@{
info:

@}
 ************************************************************************* */
void DrvRoicInitCmd(u32 commandAddress, u32 commandLength, u8 dmaCh);


/* ***********************************************************************//**
   *************************************************************************
DrvRoicInitCalibration - initialise the dma channnel which will copy data to the calibration buffer in the ROIC controller
@{
----------------------------------------------------------------------------
@}
@param
      calibrationDataAddress  - address in the memory where calibration data is stored
@param      
      calibrationDataLength - length in 32bit words 
@param      
      dmaCh - which DMA channel to be used for this 
@return
      void 
@{
info:

@}
 ************************************************************************* */
void DrvRoicInitCalibration(u32 calibrationDataAddress, u32 calibrationDataLength, u8 dmaCh );


/* ***********************************************************************//**
   *************************************************************************
DrvRoicInitData - initialise the dma channnel which will copy data from the ROIC controller to the memory 
@{
----------------------------------------------------------------------------
@}
@param
      dataAddress  - address in the memory where data will be sotred 
@param      
      dataLength - length in 32bit words 
@param      
      dmaCh - which DMA channel to be used for this 
@return
      void 
@{
info:

@}
 ************************************************************************* */
void DrvRoicInitData(u32 dataAddress, u32 dataLength, u8 dmaCh);


/* ***********************************************************************//**
   *************************************************************************
DrvRoicInitTiming - set roic timings
@{
----------------------------------------------------------------------------
@}
@param
     framePeriod  - frame period in CC,
@param     
     noLines  - no of lines per frame 
@param     
     linePeriod - line period in CC
@param     
     noWords - no of words per line 
@param     
     wordPeriod - number of CC per word
@return
      void 
@{
info:

@}
 ************************************************************************* */
void DrvRoicInitTiming(u32 framePeriod, u32 noLines, u32 linePeriod, u32 noWords, u32 wordPeriod);


/* ***********************************************************************//**
   *************************************************************************
DrvRoicInitDelay - set roic timings delay
@{
----------------------------------------------------------------------------
@}
@param
         calStart - delay between command start and calibration start
@param
         delay - delay between line start (calibration start) and data start
@return
      void 
@{
info:

@}
 ************************************************************************* */
void DrvRoicInitDelay(u32 calStart, u32 delay);


/* ***********************************************************************//**
   *************************************************************************
DrvRoicStart - set enable and number of data lines 
@{
----------------------------------------------------------------------------
@}
@param
         noDataLines - DRV_ROIC_1_DATA_LINE, DRV_ROIC_2_DATA_LINE
@param
         dataTsh - wathermark in the fifo for data
@param
         calTsh - wathermark in the fifo for cal data
@return
      void 
@{
info:

@}
 ************************************************************************* */
void DrvRoicStart(u8 noDataLines, u8 dataTsh, u8 calTsh);


#endif //DRV_ROIC_DEF_H 
