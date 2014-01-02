#include <mv_types.h>
#include <isaac_registers.h>
#include "DrvGpio.h"
#include "DrvHsdio.h"
#include <swcLeonUtils.h>

/* ***********************************************************************//**
   *************************************************************************
HSDIO get response R1 and R1b (normal response), R3, R4, R5, R5b, R6
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@return
     R1 or R1b or  R3, R4, R5, R5b, R6, R7
@{
info:
    - can be used to get R1, R1b, R3, R4, R5, R5b, R6, R7
    - interrupts must be enabled from interrupt status enable register (call init hsdio)
    .

@}

 ************************************************************************* */
u32 DrvHsdioGetR1(u32 base, u32 slot, u32 *err)
{
     if (slot)
         base += 0x100;
     // wait while commad complete is not ready or timeout did not occur
     while ((GET_REG_WORD_VAL(base + SDIOH_INT_STATUS) & 0x00010001) == 0 ) NOP;
     *err = (GET_REG_WORD_VAL(base + SDIOH_INT_STATUS) >> 16) & 0xFFFF;
     SET_REG_WORD(base + SDIOH_INT_STATUS, 0x00010001);

     return GET_REG_WORD_VAL(base + SDIOH_RESPONSE_REG1);
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO get response R1b for CMD12
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@return
     R1b
@{
info:
    - can be used to get R1b for cmd12
    - interrupts must be enabled from interrupt status enable register (call init hsdio)
    .
@}

 ************************************************************************* */
u32 DrvHsdioGetR1b(u32 base, u32 slot, u32 *err)
{
     if (slot)
         base += 0x100;
     // wait while commad complete is not ready or timeout did not occur
     while ((GET_REG_WORD_VAL(base + SDIOH_INT_STATUS) & 0x00010001) == 0 ) NOP;
     *err = (GET_REG_WORD_VAL(base + SDIOH_INT_STATUS) >> 16) & 0xFFFF;
     SET_REG_WORD(base + SDIOH_INT_STATUS, 0x00010001);

     return GET_REG_WORD_VAL(base + SDIOH_RESPONSE_REG4);
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO get response R2
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured (kept for compliance)
@param
     ptrR2 - pointer to a zone with 4 free words, where the data will be stored
@{
info:
    - can be used to get R2 ( CID or CSD)
    - interrupts must be enabled from interrupt status enable register (call init hsdio)
    .
@}

 ************************************************************************* */
void DrvHsdioGetR2(u32 base, u32 slot, u32 *ptrR2, u32 *err)
{
     if (slot)
         base += 0x100;
     // wait while commad complete is not ready or timeout did not occur
     while ((GET_REG_WORD_VAL(base + SDIOH_INT_STATUS) & 0x00010001) == 0 ) NOP;
     *err = (GET_REG_WORD_VAL(base + SDIOH_INT_STATUS) >> 16) & 0xFFFF;
     // clear those  bits
     SET_REG_WORD(base + SDIOH_INT_STATUS, 0x00010001);

     ptrR2[0] = GET_REG_WORD_VAL(base + SDIOH_RESPONSE_REG1);
     ptrR2[1] = GET_REG_WORD_VAL(base + SDIOH_RESPONSE_REG2);
     ptrR2[2] = GET_REG_WORD_VAL(base + SDIOH_RESPONSE_REG3);
     ptrR2[3] = GET_REG_WORD_VAL(base + SDIOH_RESPONSE_REG4);
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO wait for a response from cmd 0
@{
----------------------------------------------------------------------------
@}
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
@{
info:
     - check status of line 0 this should be high
@}

 ************************************************************************* */
u32 DrvHsdioGetRCmd0(u32 base, u32 slot, u32 *err)
{
     if (slot)
         base += 0x100;
         
     // wait while commad complete is not ready or timeout did not occur
     while ((GET_REG_WORD_VAL(base + SDIOH_INT_STATUS) & 0x00010001) == 0 ) NOP;
     *err = (GET_REG_WORD_VAL(base + SDIOH_INT_STATUS) >> 16) & 0xFFFF;
     // clear those  bits
     SET_REG_WORD(base + SDIOH_INT_STATUS, 0x00010001);
     
     return (GET_REG_WORD_VAL(base + SDIOH_PRESENT_STATE) >> 20) & 0x0F;
}


/* ***********************************************************************//**
   *************************************************************************
HSDIO control
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     hCtrlPwrGapWup - word which will be writen in the host control, power control, block gap control and wakeup control

@{
info:
@}

 ************************************************************************* */
void DrvHsdioControl(u32 base, u32 slot, u32 hCtrlPwrGapWup)
{
     if (slot)
         base += 0x100;

     // unmask all interrupts
     SET_REG_WORD(base + SDIOH_INT_ENABLE, 0xFFFFFEFF); // card int status enable is not enabled 
     // write all the settings
     SET_REG_WORD(base + SDIOH_HOST_CTRL , hCtrlPwrGapWup & 0xFFFFFEFF); 
     // now power on
     SET_REG_WORD(base + SDIOH_HOST_CTRL , GET_REG_WORD_VAL(base + SDIOH_HOST_CTRL) | D_HSDIO_POW_BUS_PWR_ON); 
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO timing
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     clkDiv - 8bit value, only one bit should be high or none, defines in the DrvHsdioDefines.h should be used, since the value is not shifted
               D_HSDIO_CLK_DIV_XXX 
@param     
     timeout - one of the define D_HSDIO_TOUT_2_XX (DrvHsdioDefines.h) should be used , where XX is 13..27, since the value will not be shifted
@{
info:
@}
 ************************************************************************* */
void DrvHsdioClock(u32 base, u32 slot, u32 clkDiv, u32 timeout)
{
     if (slot)
         base += 0x100;
     
     // enable internal clock and set timeout 
     SET_REG_WORD(base + SDIOH_CLK_CTRL, 0x0001 | (timeout & 0xF0000));     
     
     // wait while internal clock is not stable 
     while (!(GET_REG_WORD_VAL(base + SDIOH_CLK_CTRL) & 0x0002)) NOP; 
     
     // set clock divizor
     SET_REG_WORD(base + SDIOH_CLK_CTRL, GET_REG_WORD_VAL(base + SDIOH_CLK_CTRL) | (clkDiv & 0xFF00));
     
     // enable sd clock - this bit might get clear if there is no card present
     SET_REG_WORD(base + SDIOH_CLK_CTRL, GET_REG_WORD_VAL(base + SDIOH_CLK_CTRL) | 0x0004 ); 
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO reset
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     rst -  defines in the DrvHsdioDefines.h should be used  D_HSDIO_RST_DAT, D_HSDIO_RST_CMD, D_HSDIO_RST_ALL
@{
info:
@}
 ************************************************************************* */
void DrvHsdioReset(u32 base, u32 slot, u32 rst)
{
     if (slot)
         base += 0x100;

     SET_REG_WORD(base + SDIOH_CLK_CTRL, GET_REG_WORD_VAL(base + SDIOH_CLK_CTRL) | (rst & 0x07000000));
}


/* ***********************************************************************//**
   *************************************************************************
HSDIO send command
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
#include <stdio.h>
void DrvHsdioCmd(u32 base, u32 slot, u32 sysAdr, u32 blCnt, u32 arg, u32 cmd)
{
    if (slot)
        base += 0x100;
        
    while ( GET_REG_WORD_VAL(base + SDIOH_PRESENT_STATE) & 0x03 ) NOP;  // wait while comand inhibit
    // address in teh system memory where data will be stored 
    SET_REG_WORD(base + SDIOH_SDMA      , sysAdr);
    // block size is 0x200 , boundry is 4k, 
    SET_REG_WORD(base + SDIOH_BLOCK_SIZE, D_HSDIO_BLOCK_S_BOUND_512K | 0x200 | ((blCnt & 0xFFFF) << 16)); 
    SET_REG_WORD(base + SDIOH_ARGUEMENT , arg);
    SET_REG_WORD(base + SDIOH_TRANS_MODE, cmd);
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO led on
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@{
info:
@}
 ************************************************************************* */
void DrvHsdioLedOn(u32 base, u32 slot)
{
    slot=slot; // deliberately unused

    SET_REG_WORD(base + SDIOH_HOST_CTRL , GET_REG_WORD_VAL(base + SDIOH_HOST_CTRL) | 0x0001); 
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO led off
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@{
info:
@}
 ************************************************************************* */
void DrvHsdioLedOff(u32 base, u32 slot)
{
    slot=slot; // deliberately unused
    SET_REG_WORD(base + SDIOH_HOST_CTRL , GET_REG_WORD_VAL(base + SDIOH_HOST_CTRL) & 0xFFFFFFFE); 
}


/* ***********************************************************************//**
   *************************************************************************
HSDIO test init
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@{
info:
@}
 ************************************************************************* */
void DrvHsdioInit(u32 base, u32 slot, u32 busWidth)
{  

    // reset the HSDIO
    DrvHsdioReset(base, slot, D_HSDIO_RST_ALL);
    
    // divide the clock by 2, set timeout, enable clocks 
    DrvHsdioClock(base, slot, D_HSDIO_CLK_PASS_TROUGH, D_HSDIO_TOUT_2_27); //D_HSDIO_CLK_DIV_2 D_HSDIO_CLK_PASS_TROUGH
    
    // CONTROL, POWER, BLOCK GAP, WAKEUP
    DrvHsdioControl(base, slot, D_HSDIO_CTRL_SDMA | 
                                  (busWidth & (D_HSDIO_CTRL_4BIT_MODE | D_HSDIO_CTRL_1BIT_MODE)) | 
                                  D_HSDIO_POW_BUS_VOLT_3_3);
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO send command 0
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@return 
     response of the command      
@{
info:
@}
 ************************************************************************* */
u32 DrvHsdioCmd0(u32 base, u32 slot, u32 *err)
{
  u32 lineStat;
  DrvHsdioCmd(base, slot, 0x00000000, 0x0000, 0x0000, 
                SD_CMD_0 | 
                D_HSDIO_COMM_CMD_IDX_CHK_EN |
                D_HSDIO_COMM_CMD_CRC_CHK_EN |
                D_HSDIO_COMM_R_TYPE_NONE |
                D_HSDIO_TRMODE_DIR_WR |
                D_HSDIO_TRMODE_BLOCK_COUNT_EN |
                D_HSDIO_TRMODE_DMA_EN);
                
  *err = 0x00;
  lineStat = DrvHsdioGetRCmd0(base, slot, err);
  return lineStat;
} 



/* ***********************************************************************//**
   *************************************************************************
HSDIO send command 7, select card
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
u32 DrvHsdioCmd7(u32 base, u32 slot, u32 *err, u32 rca)
{ 
   u32 resp;
   DrvHsdioCmd(base, slot, 0x00000000, 0x0000/*bl count*/, rca/*arg*/, 
                SD_CMD_7 | 
                D_HSDIO_COMM_CMD_IDX_CHK_EN |
                D_HSDIO_COMM_CMD_CRC_CHK_EN |
                D_HSDIO_COMM_R_TYPE_LEN_48 |
                D_HSDIO_TRMODE_DIR_WR);

  *err = 0x00;
  // response should be 120h ready and acmd enabled 
  resp = DrvHsdioGetR1(base, slot, err);
  return resp;
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO send command 8, SDHC
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */

u32 DrvHsdioCmd8(u32 base, u32 slot, u32 *err, u32 volt)
{ 
   u32 resp;
   DrvHsdioCmd(base, slot, 0x00000000, 0x0000/*bl count*/, volt/*arg*/, 
                SD_CMD_8 | 
                D_HSDIO_COMM_CMD_IDX_CHK_EN |
                D_HSDIO_COMM_CMD_CRC_CHK_EN |
                D_HSDIO_COMM_R_TYPE_LEN_48 |
                D_HSDIO_TRMODE_DIR_WR);

  *err = 0x00;
  // response should be 120h ready and acmd enabled 
  resp = DrvHsdioGetR1(base, slot, err);
  return resp;
}


/* ***********************************************************************//**
   *************************************************************************
HSDIO send command 55
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
u32 DrvHsdioCmd55(u32 base, u32 slot, u32 *err, u32 rca)
{ 
   u32 resp;
   DrvHsdioCmd(base, slot, 0x00000000, 0x0000/*bl count*/, rca/*arg*/, 
                SD_CMD_55 | 
                D_HSDIO_COMM_CMD_IDX_CHK_EN |
                D_HSDIO_COMM_CMD_CRC_CHK_EN |
                D_HSDIO_COMM_R_TYPE_LEN_48 |
                D_HSDIO_TRMODE_DIR_WR);

  *err = 0x00;
  // response should be 120h ready and acmd enabled 
  resp = DrvHsdioGetR1(base, slot, err);
  return resp;
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO send ACMD 41
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */

u32 DrvHsdioAcmd41(u32 base, u32 slot, u32 *err, u32 voltRng)
{
  u32 resp;
  DrvHsdioCmd(base, slot, 0x00000000, 0x0000/*bl count*/, voltRng /*OCR as arg*/, 
                SD_ACMD_41 | 
                D_HSDIO_COMM_R_TYPE_LEN_48 |
                D_HSDIO_TRMODE_DIR_WR);

  *err = 0x00;
  resp = DrvHsdioGetR1(base, slot, err);
  return resp;
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 1. This is SEND_OP_COND for MMC cards
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */

u32 DrvHsdioCmd1(u32 base, u32 slot, u32 *err, u32 voltRng)
{
  u32 resp;
  DrvHsdioCmd(base, slot, 0x00000000, 0x0000/*bl count*/, voltRng /*OCR as arg*/,
                SD_CMD_1 |
                D_HSDIO_COMM_R_TYPE_LEN_48 |
                D_HSDIO_TRMODE_DIR_WR);

  *err = 0x00;
  resp = DrvHsdioGetR1(base, slot, err);
  return resp;
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO send ACMD 6
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */

u32 DrvHsdioAcmd6(u32 base, u32 slot, u32 *err, u32 busWidth)
{
  u32 resp, bw;
  bw=0;
  if (busWidth)
      bw = 2; 
  DrvHsdioCmd(base, slot, 0x00000000, 0x0000/*bl count*/, bw /*arg*/, 
                SD_ACMD_6 | 
                D_HSDIO_COMM_R_TYPE_LEN_48 |
                D_HSDIO_TRMODE_DIR_WR);

  *err = 0x00;
  resp = DrvHsdioGetR1(base, slot, err);
  return resp;
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 2
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@param     
     cidReg - pointer to a 4 word array where the CID will be stored
@{
info:
@}
 ************************************************************************* */
void DrvHsdioCmd2(u32 base, u32 slot, u32 *err, u32 *cidReg)
{
  DrvHsdioCmd(base, slot, 0x00000000, 0x0000/*bl count*/, 0x0000/*arg*/, 
                SD_CMD_2 | 
                D_HSDIO_COMM_CMD_CRC_CHK_EN |
                D_HSDIO_COMM_R_TYPE_LEN_136 |
                D_HSDIO_TRMODE_DIR_WR);

  *err = 0x00;
  DrvHsdioGetR2(base, slot, cidReg, err);
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 3 get RCA
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
@return 
     RCA     
@{
info:
@}
 ************************************************************************* */
u32 DrvHsdioCmd3(u32 base, u32 slot, u32 *err)
{
 u32 resp;
 DrvHsdioCmd(base, slot, 0x00000000, 0x0000/*bl count*/, 0x0000/*arg*/, 
                SD_CMD_3 | 
                D_HSDIO_COMM_CMD_IDX_CHK_EN |
                D_HSDIO_COMM_CMD_CRC_CHK_EN |
                D_HSDIO_COMM_R_TYPE_LEN_48 |
                D_HSDIO_TRMODE_DIR_WR);

  *err = 0x00;
  resp = DrvHsdioGetR1(base, slot, err);
  return resp;
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 9 get CSD
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
void DrvHsdioCmd9(u32 base, u32 slot, u32 *err, u32 *csdReg, u32 rca)
{
  DrvHsdioCmd(base, slot, 0xA0000000, 0x0000/*bl count*/, rca/*arg*/, 
                SD_CMD_9 | 
                D_HSDIO_COMM_CMD_CRC_CHK_EN |
                D_HSDIO_COMM_R_TYPE_LEN_136 |
                D_HSDIO_TRMODE_DIR_WR);

  *err = 0x00;
  DrvHsdioGetR2(base, slot, csdReg, err);
}


/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 9 get CSD
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     *err - pointer to a word that will contain a value that's not null if an error occured
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
     
@{
info:
@}
 ************************************************************************* */
u32 DrvHsdioInitSdMem(u32 base, u32 slot, u32 *sdioErr,
                                   u32 *ocr, u32 *sdCidReg, u32 *sdRca, u32 *sdCsdReg)
{
  u32 sdR;
  if (slot) slot = 1;
//  CMD0
     DrvHsdioCmd0(base, slot, sdioErr);
     if (*sdioErr)
         return SD_CMD_0; // return the command number if there is an error
      
     SET_REG_WORD(base + 0x100*slot + SDIOH_INT_STATUS, 0xFFFF); // clear all interrupts   

     do {
//CMD55
          sdR = DrvHsdioCmd55(base, slot, sdioErr, 0x0000);
          if ((*sdioErr !=0 ) || ((sdR & 0x120) != 0x120)) // sd card ready and next command is accepted as acmd
                return SD_CMD_55;
  
          SET_REG_WORD(base + 0x100*slot + SDIOH_INT_STATUS, 0xFFFF); // clear all interrupts   
// -----------------------------------------------------------------------------------------------    
//ACMD41
          sdR = DrvHsdioAcmd41(base, slot, sdioErr, *ocr );
          if (*sdioErr)
               return SD_ACMD_41; // return the command number if there is an error

          SET_REG_WORD(base + 0x100*slot + SDIOH_INT_STATUS, 0xFFFF); // clear all interrupts   
        }
     while ((sdR & D_SDIO_CARD_BUSY) != D_SDIO_CARD_BUSY);
     *ocr = sdR & 0x7FFFFFFF; // retrun OCR without the busy bit
// =================================================================================================  
//CMD2
     DrvHsdioCmd2(base, slot, sdioErr, sdCidReg);
     if (*sdioErr)
         return SD_CMD_2; 

     SET_REG_WORD(base + 0x100*slot + SDIOH_INT_STATUS, 0xFFFF); // clear all interrupts   
// =================================================================================================  
//CMD3
     sdR = DrvHsdioCmd3(base, slot, sdioErr);
     *sdRca = sdR & 0xFFFF0000;
     if (*sdioErr)
         return SD_CMD_3; 
     
     SET_REG_WORD(base + 0x100*slot + SDIOH_INT_STATUS, 0xFFFF); // clear all interrupts   
// =================================================================================================  
//CMD9
     DrvHsdioCmd9(base, slot, sdioErr, sdCsdReg, *sdRca);
     if (*sdioErr)
         return SD_CMD_9; 

     SET_REG_WORD(base + 0x100*slot + SDIOH_INT_STATUS, 0xFFFF); // clear all interrupts   
// =================================================================================================  
  return 0x1;
}



/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 17 get a block
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
u32 DrvHsdioCmd17(u32 base, u32 slot, u32 *err, u32 sysAddr, u32 sdAdr)
{ 
   u32 resp;
   DrvHsdioCmd(base, slot, sysAddr, 0x0000/*bl count*/, sdAdr/*arg*/, 
                SD_CMD_17 | 
                D_HSDIO_COMM_CMD_IDX_CHK_EN |
                D_HSDIO_COMM_CMD_CRC_CHK_EN |
                D_HSDIO_COMM_DATA_PRESENT |
                D_HSDIO_COMM_R_TYPE_LEN_48 |
                D_HSDIO_TRMODE_DMA_EN |
                D_HSDIO_TRMODE_DIR_RD);

  *err = 0x00;
  // response should be 120h ready and acmd enabled 
  resp = DrvHsdioGetR1(base, slot, err);
  return resp;
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 24 write a block
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
u32 DrvHsdioCmd24(u32 base, u32 slot, u32 *err, u32 sysAddr, u32 sdAdr)
{ 
   u32 resp;
   DrvHsdioCmd(base, slot, sysAddr, 0x0000/*bl count*/, sdAdr/*arg*/, 
                SD_CMD_24 | 
                D_HSDIO_COMM_CMD_IDX_CHK_EN |
                D_HSDIO_COMM_CMD_CRC_CHK_EN |
                D_HSDIO_COMM_DATA_PRESENT |
                D_HSDIO_COMM_R_TYPE_LEN_48 |
                D_HSDIO_TRMODE_DMA_EN |
                D_HSDIO_TRMODE_DIR_WR);

  *err = 0x00;
  // response should be 120h ready and acmd enabled 
  resp = DrvHsdioGetR1(base, slot, err);
  return resp;
}


/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 18, read multiple blocks
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
u32 DrvHsdioCmd18(u32 base, u32 slot, u32 *err, u32 sysAddr, u32 sdAdr, u32 blckCnt)
{ 
   u32 resp;
   DrvHsdioCmd(base, slot, sysAddr, blckCnt, sdAdr, 
                SD_CMD_18 | 
                D_HSDIO_COMM_CMD_IDX_CHK_EN |
                D_HSDIO_COMM_CMD_CRC_CHK_EN |
                D_HSDIO_COMM_DATA_PRESENT |
                D_HSDIO_COMM_R_TYPE_LEN_48 |
                D_HSDIO_TRMODE_DMA_EN |
                D_HSDIO_TRMODE_CMD12_EN |
                D_HSDIO_TRMODE_BLOCK_COUNT_EN |
                D_HSDIO_TRMODE_MULT_BLOC |
                D_HSDIO_TRMODE_DIR_RD);

  *err = 0x00;
  // response should be 120h ready and acmd enabled 
  resp = DrvHsdioGetR1(base, slot, err);
  return resp;
}


/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 25 write multiple blocks
@{
----------------------------------------------------------------------------
@}
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
@{
info:
@}
 ************************************************************************* */
u32 DrvHsdioCmd25(u32 base, u32 slot, u32 *err, u32 sysAddr, u32 sdAdr, u32 blckCnt)
{ 
   u32 resp;
   DrvHsdioCmd(base, slot, sysAddr, blckCnt, sdAdr, 
                SD_CMD_25 | 
                D_HSDIO_COMM_CMD_IDX_CHK_EN |
                D_HSDIO_COMM_CMD_CRC_CHK_EN |
                D_HSDIO_COMM_DATA_PRESENT |
                D_HSDIO_COMM_R_TYPE_LEN_48 |
                D_HSDIO_TRMODE_DMA_EN |
                D_HSDIO_TRMODE_CMD12_EN |
                D_HSDIO_TRMODE_BLOCK_COUNT_EN |
                D_HSDIO_TRMODE_MULT_BLOC |
                D_HSDIO_TRMODE_DIR_WR);

  *err = 0x00;
  // response should be 120h ready and acmd enabled 
  resp = DrvHsdioGetR1(base, slot, err);
  return resp;
}

/* ***********************************************************************//**
   *************************************************************************
HSDIO send CMD 25 write multiple blocks
@{
----------------------------------------------------------------------------
@}
@param
     base - base address of the sdio host module : SDIOH1_BASE_ADR or SDIOH2_BASE_ADR
@param
     slot - if 0 the slot used will be slot 0, if anything else the slot used will be slot 1 (only for hsdio1)
@param
     sysAddr - data is read from this address in the system 
@param
     sdAdr - address on the card, has to be block alligned (tippical 0x200) where data will be written 
@param
     blckCnt - number of blocks to be transfered     
@param
     cont - if set (non 0) when the controller reaches a memory boundry (default 512K) the next address will 
     
@{
info:
@}
 ************************************************************************* */
u32 DrvHsdioSdMemRead(u32 base, u32 slot,u32 sysAddr, u32 sdAddr, u32 blocks, u32 cont)
{
    u32 sdioErr;
    if (slot)
        base += 0x100;
    DrvHsdioCmd18(base, slot, &sdioErr, sysAddr, sdAddr, blocks);
    while ((GET_REG_WORD_VAL(base + SDIOH_PRESENT_STATE) & 0x0200) == 0x0200)
    {    // wait while the read transfer is active
        if (GET_REG_WORD_VAL(base + SDIOH_INT_STATUS) & 0x08)
        {
            SET_REG_WORD(base + SDIOH_INT_STATUS, 0x0008); // clear the dma interrupt
            
            if (cont)
              SET_REG_WORD(base + SDIOH_SDMA, GET_REG_WORD_VAL(base + SDIOH_SDMA)); // continue    
            else
              SET_REG_WORD(base + SDIOH_SDMA, sysAddr);
        }
    }     
    return sdioErr;
}


u32 DrvHsdioSdMemWrite(u32 base, u32 slot,u32 sysAddr, u32 sdAddr, u32 blocks, u32 cont)
{
    u32 sdioErr;
    if (slot)
        base += 0x100;
        
    DrvHsdioCmd25(base, slot, &sdioErr, sysAddr, sdAddr, blocks);
    while ((GET_REG_WORD_VAL(base + SDIOH_PRESENT_STATE) & 0x0100) == 0x0100) 
    {    // wait while the read transfer is active
         // this while can be replaced with an interrupt routine, on the DMA event bit 
        if (GET_REG_WORD_VAL(base + SDIOH_INT_STATUS) & 0x08)
        {
            SET_REG_WORD(base + SDIOH_INT_STATUS, 0x0008); // clear the dma interrupt 
            if (cont)
              SET_REG_WORD(base + SDIOH_SDMA, GET_REG_WORD_VAL(base + SDIOH_SDMA)); // continue    
            else
              SET_REG_WORD(base + SDIOH_SDMA, sysAddr);
        }
    }     
    return sdioErr;
}


