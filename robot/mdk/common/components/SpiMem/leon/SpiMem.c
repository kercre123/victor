///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
///
///

#include "SpiMemLowLevelApi.h"
#include "SpiMemApi.h"

#define SPIMEM_WIP 0x01

#if ENABLEPRINTK
#include <stdio.h>
#define printk(...) printf(__VA_ARGS__)
#else
#define printk(...)
#endif


void SpiMemWaitWrite()
{
    u32 _srr;
    do
    {
        _srr = SpiMemRDSR();
        //printk(" <spiRDSR> = 0x%X\n", _srr);
    }
    while( _srr & SPIMEM_WIP );
}


void SpiMemEraseSectors(tySpiMemState * spiMemPtr, u32 start_sector, u32 end_sector)
{
    u32 pos;
    for(pos = spiMemPtr->base + start_sector * (spiMemPtr->sectorSize);
            pos < spiMemPtr->base + (end_sector+1) * (spiMemPtr->sectorSize);
            pos += (spiMemPtr->sectorSize))
    {
        printk("deleting sector = 0x%X \n", pos);

        //wait if chip is still writing
        SpiMemWaitWrite();
        // erase a sector
        SpiMemWREN();
        SpiMemSE(pos);
    }
}


void SpiMemWriteSectors(
        tySpiMemState * spiMemPtr,
        const u8* buffer,
        u32 sector,
        u32 sector_count,
        pointer_type PointerType)
{
    u32 pos, bpos;

    for(pos = spiMemPtr->base + sector * (spiMemPtr->sectorSize), bpos = 0;
            pos < spiMemPtr->base + (sector+sector_count) * (spiMemPtr->sectorSize);
            pos+=(spiMemPtr->pageSize), bpos+=(spiMemPtr->pageSize))
    {
        if(pos % (spiMemPtr->sectorSize) == 0)  /// if start of a sector, clear it
        {
            //wait if chip is still writing
            SpiMemWaitWrite();
            //erase first (flash memory requirement)
            SpiMemWREN();
            SpiMemSE(pos);
        }

        printk ("writing page from : 0x%X    ---->>> 0x%X  \n", buffer + bpos, pos);

        //wait if chip is still writing
        SpiMemWaitWrite();
        // write page
        SpiMemWREN();
        SpiMemPP(pos, (spiMemPtr->pageSize), (u8 *)(buffer + bpos), PointerType);

    }
}


void SpiMemReadSectors(
        tySpiMemState * spiMemPtr,
        u8* buffer,
        u32 sector,
        u32 sector_count,
        pointer_type PointerType)
{
    //wait if chip is still writing
    SpiMemWaitWrite();
    //do read
    SpiMemFASTREAD(spiMemPtr->base + sector*(spiMemPtr->sectorSize),
            sector_count*(spiMemPtr->sectorSize),
            (u8 *)(buffer), PointerType);
}


int spiMemSetup(tySpiMemState * spiMemPtr, u32 base_addr, u32 mem_size)
{
    //enter generic data
    spiMemPtr->pageSize    = 256;
    spiMemPtr->sectorSize  = 4096;
    spiMemPtr->sectorPerBlock = 1;

    if (mem_size==0)
        mem_size = 2*1024*1024 - base_addr;

    if(base_addr + mem_size > 2*1024*1024)
    {
        spiMemPtr->sectorCount = 0;
        spiMemPtr->size        = 0;
        return 0;
    }
    else
    {
        spiMemPtr->base = (base_addr/spiMemPtr->sectorSize)*spiMemPtr->sectorSize;
        spiMemPtr->size = (mem_size/spiMemPtr->sectorSize)*spiMemPtr->sectorSize;

        spiMemPtr->sectorCount = spiMemPtr->size / 4096;
        return 1;
    }
}


