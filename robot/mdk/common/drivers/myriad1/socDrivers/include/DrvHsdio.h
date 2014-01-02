///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Low-level driver for SD card I/O
/// 
/// 
/// 
#ifndef _BRINGUP_SABRE_HSDIO_H_
#define _BRINGUP_SABRE_HSDIO_H_

#include "mv_types.h"
#include "DrvHsdioDefines.h"

/* ***********************************************************************//**
   *************************************************************************
HSDIO get response R1 and R1b (normal response), R3, R4, R5, R5b, R6



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@return
     R1 or R1b or  R3, R4, R5, R5b, R6, R7

info:
    - can be used to get R1, R1b, R3, R4, R5, R5b, R6, R7
    - interrupts must be enabled from interrupt status enable register (call init hsdio)
    .



 ************************************************************************* */
u32 DrvHsdioGetR1(u32 base, u32 slot, u32 *err);

/* ***********************************************************************//**
   *************************************************************************
HSDIO get response R1b for CMD12



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@return
     R1b

info:
    - can be used to get R1b for cmd12
    - interrupts must be enabled from interrupt status enable register (call init hsdio)
    .


 ************************************************************************* */
u32 DrvHsdioGetR1b(u32 base, u32 slot, u32 *err);

/* ***********************************************************************//**
   *************************************************************************
HSDIO get response R2



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured (kept for compliance)
@param
     ptrR2 - pointer to a zone with 4 free words, where the data will be stored

info:
    - can be used to get R2 ( CID or CSD)
    - interrupts must be enabled from interrupt status enable register (call init hsdio)
    .


 ************************************************************************* */
void DrvHsdioGetR2(u32 base, u32 slot, u32 *ptrR2, u32 *err);

/* ***********************************************************************//**
   *************************************************************************
HSDIO wait for a response from cmd 0



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured (kept for compliance)
@return 
     - line status     
     - bit 0 = data line 0 
     - bit 1 = data line 1
     - bit 2 = data line 2      
     - bit 3 = data line 3      
     .

info:
     - check status of line 0 this should be high


 ************************************************************************* */
u32 DrvHsdioGetRCmd0(u32 base, u32 slot, u32 *err);

/* ***********************************************************************//**
   *************************************************************************
HSDIO control



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     hCtrlPwrGapWup - word which will be writen in the host control, power control, block gap control and wakeup control


info:


 ************************************************************************* */
void DrvHsdioControl(u32 base, u32 slot, u32 hCtrlPwrGapWup);

/* ***********************************************************************//**
   *************************************************************************
HSDIO timing



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     clkDiv - 8bit value, only one bit should be high or none, defines in the DrvHsdioDefines.h should be used, since the value is not shifted
               D_HSDIO_CLK_DIV_XXX 
@param     
     timeout - one of the define D_HSDIO_TOUT_2_XX (DrvHsdioDefines.h) should be used , where XX is 13..27, since the value will not be shifted

info:

 ************************************************************************* */
void DrvHsdioClock(u32 base, u32 slot, u32 clkDiv, u32 timeout);

/* ***********************************************************************//**
   *************************************************************************
HSDIO reset



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     rst -  defines in the DrvHsdioDefines.h should be used  D_HSDIO_RST_DAT, D_HSDIO_RST_CMD, D_HSDIO_RST_ALL

info:

 ************************************************************************* */
void DrvHsdioReset(u32 base, u32 slot, u32 rst);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send command



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     sysAdr - address in the system where data will be stored or read from
@param     
     blCnt - number of blocks to transfer
@param     
     arg - argument of the command
@param     
     cmd - value to be written in the transfer mode and command registers, defines in the DrvHsdioDefines.h

info:

 ************************************************************************* */
void DrvHsdioCmd(u32 base, u32 slot, u32 sysAdr, u32 blCnt, u32 arg, u32 cmd);

/* ***********************************************************************//**
   *************************************************************************
HSDIO led on



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)

info:

 ************************************************************************* */
void DrvHsdioLedOn(u32 base, u32 slot);

/* ***********************************************************************//**
   *************************************************************************
HSDIO led off



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)

info:

 ************************************************************************* */
void DrvHsdioLedOff(u32 base, u32 slot);

/* ***********************************************************************//**
   *************************************************************************
HSDIO test init



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param     
     busWidth -    0 - 1 bit ; !0 - 4 bits

info:

 ************************************************************************* */
void DrvHsdioInit(u32 base, u32 slot, u32 busWidth);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send command 0



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@return 
     response of the command      

info:

 ************************************************************************* */
u32 DrvHsdioCmd0(u32 base, u32 slot, u32 *err);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send command 7, select card



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param
     rca - relative card address, rca is on the upper 16 bits
@return 
     response of the command      

info:

 ************************************************************************* */
u32 DrvHsdioCmd7(u32 base, u32 slot, u32 *err, u32 rca);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send command 8, SDHC



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param
     volt - voltage argument and check pattern , should be 0x1AA
@return 
     response of the command  R7 

info:

 ************************************************************************* */
u32 DrvHsdioCmd8(u32 base, u32 slot, u32 *err, u32 volt);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send command 55



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param
     rca - relative card address of the card (on bits [31:16] ) , the others are blank
@return 
     response of the command      

info:

 ************************************************************************* */
u32 DrvHsdioCmd55(u32 base, u32 slot, u32 *err, u32 rca);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send ACMD 41



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param     
     voltRng - voltage range of the host
@return 
     response of the command, OCR 

info:

 ************************************************************************* */
u32 DrvHsdioAcmd41(u32 base, u32 slot, u32 *err, u32 voltRng);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 1. This is SEND_OP_COND for MMC cards



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param
     voltRng - voltage range of the host
@return
     response of the command, OCR

info:

 ************************************************************************* */
 
u32 DrvHsdioCmd1(u32 base, u32 slot, u32 *err, u32 voltRng);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send ACMD 6



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param     
     busWidth -    0 - 1 bit ; !0 - 4 bits
@return 
     response of the command, OCR 

info:

 ************************************************************************* */

u32 DrvHsdioAcmd6(u32 base, u32 slot, u32 *err, u32 busWidth);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 2



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param     
     cidReg - pointer to a 4 word array where the CID will be stored

info:

 ************************************************************************* */
 
void DrvHsdioCmd2(u32 base, u32 slot, u32 *err, u32 *cidReg);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 3 get RCA



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@return 
     RCA     

info:

 ************************************************************************* */
 
u32 DrvHsdioCmd3(u32 base, u32 slot, u32 *err);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 9 get CSD



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param     
     *csdReg - pointer to a 4 word array where the CID will be stored 
@param     
     rca - relative card address , found with cmd3  
         - rca is on bits [31:16] , rest is don't care
         .

info:

 ************************************************************************* */
 
void DrvHsdioCmd9(u32 base, u32 slot, u32 *err, u32 *csdReg, u32 rca);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 9 get CSD



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *sdioErr - pointer to a word that will contain a value that's not null if an error occured
@param     
     *ocr - pointer where the ocr value will be returned, at the time of call it should contain the ocr of the host 
@param          
     *sdCidReg - pointer to a 4 word array, where the CID will be returned
@param          
     *sdRca - pointer to a word where the RCA will be stored, on the upper 16 bits
@param          
     *sdCsdReg - pointer to a 4 word array, where the CSD will be returned
@return
     1 if init was ok, or the CMD code if there was an error      
     

info:

 ************************************************************************* */
 
u32 DrvHsdioInitSdMem(u32 base, u32 slot, u32 *sdioErr,
                                   u32 *ocr, u32 *sdCidReg, u32 *sdRca, u32 *sdCsdReg);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 17 get a block



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param
     sysAddr - address in the system where the data will be stored
@param
     sdAdr - address on the card, has to be block alligned (tippical 0x200)

info:

 ************************************************************************* */   
 
u32 DrvHsdioCmd17(u32 base, u32 slot, u32 *err, u32 sysAddr, u32 sdAdr);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 24 write a block



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param
     sysAddr - data is read from this address in the system 
@param
     sdAdr - address on the card, has to be block alligned (tippical 0x200) where data will be written 

info:

 ************************************************************************* */
 
u32 DrvHsdioCmd24(u32 base, u32 slot, u32 *err, u32 sysAddr, u32 sdAdr);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 18, read multiple blocks



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param
     sysAddr - data is written to this address in the system 
@param
     sdAdr - address on the card, has to be block alligned (tippical 0x200) , data is read from here 
@param
     blckCnt - number of blocks to be transfered     

info:

 ************************************************************************* */
 
u32 DrvHsdioCmd18(u32 base, u32 slot, u32 *err, u32 sysAddr, u32 sdAdr, u32 blckCnt);

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 25 write multiple blocks



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param
     sysAddr - data is read from this address in the system 
@param
     sdAdr - address on the card, has to be block alligned (tippical 0x200) where data will be written 
@param
     blckCnt - number of blocks to be transfered     

info:

 ************************************************************************* */
u32 DrvHsdioCmd25(u32 base, u32 slot, u32 *err, u32 sysAddr, u32 sdAdr, u32 blckCnt);

u32 DrvHsdioSdMemRead(u32 base, u32 slot,u32 sysAddr, u32 sdAddr, u32 blocks, u32 cont);
/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 25 read/write multiple blocks



@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     sysAddr - data is read from this address in the system 
@param
     sdAddr - address on the card, has to be block alligned (tippical 0x200) where data will be written 
@param
     blocks - number of blocks to be transfered     
@param
     cont - if set (non 0) when the controller reaches a memory boundry (default 512K) the next address will 
     

info:

 ************************************************************************* */
u32 DrvHsdioSdMemWrite(u32 base, u32 slot,u32 sysAddr, u32 sdAddr, u32 blocks, u32 cont);
#endif // _BRINGUP_SABRE_HSDIO_H_

