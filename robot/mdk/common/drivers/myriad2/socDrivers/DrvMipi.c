/**
 * @file DrvMIPI.c
 *                Contains the driver funtions to manage MIPI controller 
 * @brief MIPI Driver Implementation
 * @author Lucian Vancea 
 */

#include "registersMyriad.h"
#include "mv_types.h"
#include "assert.h" 
#include "stdio.h"


#include "DrvMipi.h"
#include "DrvMipiDefines.h"

#define DRV_MIPI_FREQ_RANGE_NO (40)

static u16 dDrvMipiFreqRangeMHz[DRV_MIPI_FREQ_RANGE_NO] = {
80, 90, 100, 110, 130, 140, 150, 170, 180, 200, 220, 240, 250, 270, 
300, 330, 360, 400, 450, 500, 550, 600, 650, 700, 750, 800, 850, 900, 
950, 1000, 1050, 1100, 1150, 1200, 1250, 1300, 1350, 1400, 1450, 1500};

static u8 dDrvMipihsfreqrange[DRV_MIPI_FREQ_RANGE_NO - 1] = {
0,16,32,1,17,33,2,18,34,3,19,35,4,20,5,21,37,6,22,7,23,8,24,9,25,41,57,10,26,42,58,11,27,43,59,12,28,44,60};

//#define DRV_MIPI_DEBUG

#ifdef DRV_MIPI_DEBUG
#define DPRINTF1(...) printf(__VA_ARGS__)
#else
#define DPRINTF1(...)
#endif

// if for some reason, only one PHY has to be cleared
void DrvMipiTestClear(u8 MipiCtrlNo)
{
      // test clear
      SET_REG_WORD(MIPI_DPHY_TEST_CLR_ADDR, 1<<MipiCtrlNo);
      SET_REG_WORD(MIPI_DPHY_TEST_CLR_ADDR, 0);            
}

void DrvMipiTestClearAll()
{
      // test clear all
      SET_REG_WORD(MIPI_DPHY_TEST_CLR_ADDR, 0x3F);         
      SET_REG_WORD(MIPI_DPHY_TEST_CLR_ADDR, 0);            
}


 u8 DrvMipiTestModeSendCmd(u8 MipiCtrlNo, u8 Command, u8 *DataIn, u8 *DataOut, u8 Length)
 {
      int i;
      u32 RetVal, DOut;
      
      // test input data 
      assert(MipiCtrlNo <= 5);
      assert((u32)DataIn);
/*
                 1____2 
      CLR      __|    |________________________________________________________ 
                              3_________________7
      EN       _______________|                 |_____________________________
                                  4_________6                   9_____10 
      CLK      ___________________|         |___________________|     |_______
                                       5                     8 
      DIN      ------------------------<COMMAND>------------<  DATA  >---------
*/      
      
      
      // send test command 
      SET_REG_WORD(MIPI_DPHY_TEST_EN_ADDR,  1<<MipiCtrlNo); //3    
      SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, 1<<MipiCtrlNo); //4
      SET_REG_WORD(MIPI_DPHY_TEST_DIN_ADDR, Command);       //5
      SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, 0);             //6      
      SET_REG_WORD(MIPI_DPHY_TEST_EN_ADDR,  0);             //7
      
      
      // because DOUT FROM phys 0&1 share a register and so do 2&3 and 4&5
      RetVal = GET_REG_WORD_VAL(MIPI_DPHY_TEST_DOUT_DPHY01_ADDR + 4*(MipiCtrlNo>>1));
      // Higher numbered dphy's output is placed on bites 8->15
      RetVal >>= 8*(MipiCtrlNo & 1);
      RetVal &= 0xFF;
      
      if (DataIn != NULL)
      // send specific data , repeat steps 8, 9 and 19   
          for (i=0 ; i<Length ; ++i)
          {
                    SET_REG_WORD(MIPI_DPHY_TEST_DIN_ADDR, (*DataIn) & 0xFF);//8
                    ++DataIn;
                    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, 1<<MipiCtrlNo); //9                
                    SET_REG_WORD(MIPI_DPHY_TEST_CLK_ADDR, 0);             //10                       
 
                    DOut = GET_REG_WORD_VAL(MIPI_DPHY_TEST_DOUT_DPHY01_ADDR + 4*(MipiCtrlNo>>1));
                    // Higher numbered dphy's output is placed on bites 8->15
                    DOut >>= 8*(MipiCtrlNo & 1);
                    if (DataOut != NULL)
                    {
                       *DataOut = DOut & 0xFF;
                       ++DataOut;
                    }
          }
      
      return RetVal;
 } 
 
 u32 DrvMipiPllProg(u32 mipiCtrlNo, u32 refClkKHz, u32 desiredClkKHz)
 {
       int m, n, i; // pll params 
       int MemM, MemN;
       u32 MemFreqDifference, TempFreqDifference, MemPllFreq;
       float DivFreq; 
       float PllFreq;
       
       u8 dataIn, dataOut;
       
       int Solutions;
       MemM = 0;
       MemN = 0;
       MemPllFreq = 0;
       
       //      Fout = (M/ N) * RefClk;
       //      40MHz >= RefClk / n >= 5      
      
       // m = 1, 3, 5, ... , 297, 299 => M = 2, 4, 6, ..., 298, 300
       // n = 0, 1, 2, ... , 98, 99   => N = 1, 2, 3, ..., 99, 100
       MemFreqDifference = 0xFFFFFFFF; // init with the worst case 
       Solutions = 0;

       DPRINTF1("mipi ctrl no %d\n", mipiCtrlNo);
       DPRINTF1("refClkKHz %d\n", refClkKHz);       
       DPRINTF1("desiredClkKHz %d\n", desiredClkKHz);
       
        for (n = 0; n<64; n++)
        {
             DivFreq = (float)refClkKHz  / (float)(n+1);
             DPRINTF1("n %d\n", n);
             if (DivFreq >= 5000)
                     if (DivFreq <= 40000)
                     {
                             // so N is good, lets find M
                             for (m = 1 ; m < 300 ; m+=2)
                             {
                                 // DPRINTF1("m %d\n", m);
                                  PllFreq = DivFreq * (m+1);
                                  if ((u32)PllFreq > desiredClkKHz)
                                      TempFreqDifference = (u32)PllFreq - desiredClkKHz;
                                  else     
                                      TempFreqDifference = desiredClkKHz - (u32)PllFreq ;
                                      
                                  if (TempFreqDifference < MemFreqDifference)    
                                  {
                                      // if there is a better match for M & N, memorise 
                                      MemM = m;
                                      MemN = n;
                                      MemFreqDifference = TempFreqDifference;
                                      MemPllFreq = (u32)PllFreq;
                                      ++Solutions;
                                  }
                             }
                     }
                     
        }
        
        if (Solutions == 0)
              return 0; // no matching freq found 
        
        //find the range 
        for (i=0 ; i<= DRV_MIPI_FREQ_RANGE_NO ; ++i )
        {
                if (MemPllFreq <= (dDrvMipiFreqRangeMHz[i] * 1000))
                      break;
        }

        // if smaller then min freq or bigger then max freq, not possible
        if (i==0)
            return 0; 
            
        if ((i == DRV_MIPI_FREQ_RANGE_NO) && (MemPllFreq > dDrvMipiFreqRangeMHz[i])) 
            return 0;
        
        // with M and N found that have the closest match to the desired frequency, programm the PLL
        DPRINTF1("programming params\n");
        
        // programm the range 
        dataIn = (dDrvMipihsfreqrange[i-1] << 1) & 0x7E;
        DrvMipiTestModeSendCmd(mipiCtrlNo, D_MIPI_TCMD_HS_RX_CTRL_L0, &dataIn, &dataOut, 1);
        
        // program N
        dataIn = MemN;
        DrvMipiTestModeSendCmd(mipiCtrlNo, D_MIPI_TCMD_PLL_IN_DIV, &dataIn, &dataOut, 1);

        // use N & M  - this has to be done before M is programmed 
        dataIn = 0x30;
        DrvMipiTestModeSendCmd(mipiCtrlNo, D_MIPI_TCMD_PLL_IN_LOOP_DIV, &dataIn, &dataOut, 1);
        
        
        // program M low part
        dataIn = MemM & 0x1F;
        DrvMipiTestModeSendCmd(mipiCtrlNo, D_MIPI_TCMD_PLL_LOOP_DIV, &dataIn, &dataOut, 1);

        // program M high part        
        dataIn = ((MemM >> 5) & 0xF) | 0x80;
        DrvMipiTestModeSendCmd(mipiCtrlNo, D_MIPI_TCMD_PLL_LOOP_DIV, &dataIn, &dataOut, 1);

        return MemPllFreq;
 }
 
 // if timeout is 0 waits forever for the PLL to lock
 // otherwise it will wait for timeout cicles 
 
int DrvMipiWaitPllLock(u32 mipiCtrlNo, u32 timeout)
{
     if (timeout == 0)
         while (1)
         {
              if ((GET_REG_WORD_VAL(MIPI_DPHY_PLL_LOCK_ADDR) & (1<<mipiCtrlNo)) == (u32)(1<<mipiCtrlNo))
                  return 1;
         }
     else
         do 
         {  
              if ((GET_REG_WORD_VAL(MIPI_DPHY_PLL_LOCK_ADDR) & (1<<mipiCtrlNo)) == (u32)(1<<mipiCtrlNo))
                  return 1;
         }
         while (timeout-- > 0);

    return 0;
}

u32 DrvMipiInitDphy(u32 mipiDphyNo)
{
   u32 RegOldVal;
   
   RegOldVal = GET_REG_WORD_VAL(MIPI_DPHY_INIT_CTRL_ADDR);
   
   SET_REG_WORD(MIPI_DPHY_INIT_CTRL_ADDR, RegOldVal | 
                                          (D_MIPI_DPHY_INIT_CTRL_EN_0 << mipiDphyNo));
   
   SET_REG_WORD(MIPI_DPHY_INIT_CTRL_ADDR,  RegOldVal | 
                                          (D_MIPI_DPHY_INIT_CTRL_SHUTDOWNZ_0 << mipiDphyNo) | // according to the spec 5.2.1 SHUTDOWNZ is first then RSTZ
                                          (D_MIPI_DPHY_INIT_CTRL_EN_0 << mipiDphyNo));  
                                           
  
   SET_REG_WORD(MIPI_DPHY_INIT_CTRL_ADDR, RegOldVal | 
                                          (D_MIPI_DPHY_INIT_CTRL_CLK_CONT_0 << mipiDphyNo) | 
                                          (D_MIPI_DPHY_INIT_CTRL_SHUTDOWNZ_0 << mipiDphyNo)|
                                          (D_MIPI_DPHY_INIT_CTRL_EN_0 << mipiDphyNo) |
                                          (D_MIPI_DPHY_INIT_CTRL_RSTZ_0 << mipiDphyNo) );                                         
                                          
   return RegOldVal;
}



u32 DrvMipiTxLPData(u32 mipiLaneNo, u32 data)
{
         SET_REG_WORD(MIPI_TX_LPDT_DATA_ADDR, data);
         SET_REG_WORD(MIPI_TX_LPDT_REQ_ADDR,     1 <<  mipiLaneNo);     // request LP tx on lane 
}




