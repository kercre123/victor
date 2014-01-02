///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @defgroup SpiMem SpiMem component
/// @{
/// @brief FatFS back-end component for use on SPI chip
///

#ifndef SPIMEMAPI_H_
#define SPIMEMAPI_H_

#include <swcLeonUtils.h>
#include <mv_types.h>
#include "SpiMemApiDefines.h"
#include "SpiMemLowLevelApi.h"

/// Initialize SpiMem handler to be used by FatFs
/// @param spiMemPtr   SpiMem handler to be initialized
/// @param base_addr   spi address to be used as base of FatFs (multiple of 4096 preffered)
/// @param mem_size    the size of flash spi to be used as FatFs ( use 0 to autodetect from base_addr to end of flash memory)
/// @return            function returns 1 on success and 0 on fail
int spiMemSetup(tySpiMemState * spiMemPtr, u32 base_addr, u32 mem_size);


/// Erase sectors from SpiMem flash
/// @param spiMemPtr    SpiMem handler address (must be initialized)
/// @param start_sector first sector to be erased
/// @param end_sector   last sector to be erased
/// @return       void
void SpiMemEraseSectors(tySpiMemState * spiMemPtr, u32 start_sector, u32 end_sector);


/// Write a number of sectors to SpiMem from a buffer
/// @param spiMemPtr    SpiMem handler address (must be initialized)
/// @param buffer        buffer pointer to the data to be written
/// @param sector       sector number where the data should be written
/// @param sector_count how many sectors should be written
/// @param PointerType  endianness of the buffer parameter
/// @return  void
void SpiMemWriteSectors(tySpiMemState * spiMemPtr, const u8* buffer, u32 sector, u32 sector_count, pointer_type PointerType);


/// Read a number of sectors to a buffer from SpiMem
/// @param spiMemPtr    SpiMem handler address (must be initialized)
/// @param buffer        buffer pointer where the data should be read
/// @param sector       sector number from where the data should be read
/// @param sector_count how many sectors should be read
/// @param PointerType  endianness of the buffer parameter
/// @return   void
void SpiMemReadSectors(tySpiMemState * spiMemPtr, u8* buffer, u32 sector, u32 sector_count, pointer_type PointerType);


/// Block until flash chip is finished writing from it's internal buffers
/// @return  void
void SpiMemWaitWrite();

/// @}
#endif /* SPIMEMAPI_H_ */
