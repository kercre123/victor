/**
 * L2 Cache helper
 * 
 * @file                                        
 * @author    Cormac Brick                     
 * @brief     L2 Cache helper    
 * @copyright All code copyright Movidius Ltd 2012, all rights reserved 
 *            For License Warranty see: common/license.txt   
 * 
*/

#ifndef SWC_L2_CACHE_H
#define SWC_L2_CACHE_H

#include "mv_types.h"


/// Configure L2 cache partitions
///@param partition_regs an array containing the register configuration for each of the 8 possible partitions
void swcL2cacheSetPartitions(u32 *partition_regs);

#endif
