/**
 * Simple higher level L2 Cache driver
 * 
 * @File                                        
 * @Author    Cormac Brick                     
 * @Brief     Simple Shave loader and window manager    
 * @copyright All code copyright Movidius Ltd 2012, all rights reserved 
 *            For License Warranty see: common/license.txt   
 * 
*/

#include "swcL2Cache.h"

#include "isaac_registers.h" 
#include "DrvSvu.h"
#include "assert.h" 

static void L2_service( unsigned int partition, unsigned int mode ) {
  // set L2C_STOP and wait while L2C_BUSY
  *(unsigned int*)L2C_STOP_ADR = -1;
  while( *(volatile unsigned int*)L2C_BUSY_ADR )
    ;
  // invalidate the cache and wait while L2C_BUSY
  *(unsigned int*)L2C_FLUSHINV_ADR =( partition << 3 )| mode;
  while( *(volatile unsigned int*)L2C_BUSY_ADR )
    ;
  // clear L2C_STOP
  *(unsigned int*)L2C_STOP_ADR = 0;
}

void swcL2cacheSetPartitions(u32 *partition_regs)
{
  // Set L2C_MODE
  *(unsigned int*)L2C_MODE_ADR  = 0x6; //This was also set at load time

  // Set partition 0 to
  *(unsigned int*)L2C_PTRANS0_ADR = partition_regs[0]; SET_REG_WORD(  SLICE_WIN_CPC(0) , 0x0000);SET_REG_WORD(  SLICE_NWN_CPC(0) , 0x00000018 | 0x0);
  *(unsigned int*)L2C_PTRANS1_ADR = partition_regs[1]; SET_REG_WORD(  SLICE_WIN_CPC(1) , 0x0001);SET_REG_WORD(  SLICE_NWN_CPC(1) , 0x01000018 | 0x0);
  *(unsigned int*)L2C_PTRANS2_ADR = partition_regs[2]; SET_REG_WORD(  SLICE_WIN_CPC(2) , 0x0001);SET_REG_WORD(  SLICE_NWN_CPC(2) , 0x01000018 | 0x0);
  *(unsigned int*)L2C_PTRANS3_ADR = partition_regs[3]; SET_REG_WORD(  SLICE_WIN_CPC(3) , 0x0001);SET_REG_WORD(  SLICE_NWN_CPC(3) , 0x01000018 | 0x0);
  *(unsigned int*)L2C_PTRANS4_ADR = partition_regs[4]; SET_REG_WORD(  SLICE_WIN_CPC(4) , 0x0001);SET_REG_WORD(  SLICE_NWN_CPC(4) , 0x01000018 | 0x0);
  *(unsigned int*)L2C_PTRANS5_ADR = partition_regs[5]; SET_REG_WORD(  SLICE_WIN_CPC(5) , 0x0001);SET_REG_WORD(  SLICE_NWN_CPC(5) , 0x01000018 | 0x0);
  *(unsigned int*)L2C_PTRANS6_ADR = partition_regs[6]; SET_REG_WORD(  SLICE_WIN_CPC(6) , 0x0001);SET_REG_WORD(  SLICE_NWN_CPC(6) , 0x01000018 | 0x0);
  *(unsigned int*)L2C_PTRANS7_ADR = partition_regs[7]; SET_REG_WORD(  SLICE_WIN_CPC(7) , 0x0007);SET_REG_WORD(  SLICE_NWN_CPC(7) , 0x01000018 | 0x0);


 // @todo: remove this ifdef
#ifdef ENABLE_L1
  // With L1 cache activated also
  DrvSvuEnableL1Cache(0);DrvSvuEnableL1Cache(1);DrvSvuEnableL1Cache(2);DrvSvuEnableL1Cache(3);
  DrvSvuEnableL1Cache(4);DrvSvuEnableL1Cache(5);DrvSvuEnableL1Cache(6);DrvSvuEnableL1Cache(7);
#endif
  // Setup master IDs
  *(unsigned int*)L2C_MXITID_ADR  = 0x76543210;
  // Invalidate the cache
  L2_service( 7, 1 << 1 );
}
 
