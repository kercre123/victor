#include "diskio_wrapper_SpiMem.h"

#include "SpiMemApi.h"

#if ENABLEPRINTK
#include <stdio.h>
#define printk(...) printf(__VA_ARGS__)
#else
#define printk(...)
#endif

static DSTATUS SpiMem_disk_status (void *private) {

    // check the EEPROM data
    // release from powerdown / read ES
    u32 es = SpiMemRES();
    printk( "  <spiRES>     ES=0x%02X\n", es );

    // read manufacturer/device IDs
    u32 mfg_id = SpiMemREMS();
    printk( "  <spiREMS> MFGID=0x%02X DEVID=0x%02X\n", (mfg_id>>8)&0xFF, mfg_id&0xFF );

    // read device identification
    u32 rdid = SpiMemRDID();
    printk( "  <spiRDID> MFGID=0x%02X MTYPE=0x%02X DEVID=0x%02X\n", rdid>>16, (rdid>>8)&0xFF, rdid&0xFF );

    //check for expected EEPROM
    if( es != 0x14 || mfg_id != 0x3714 || rdid != 0x373015 )
    {
        printk( "ERROR: unknown chip\n"
                "(expected spiRES={0x14}, spiREMS={0x37,0x17}, spiRDID={0x37,0x30,0x15})\n" );

        return RES_ERROR;
    }
    else
    {
        printk( "SPI EEPROM is of the expected type - AMIC A25L016\n" );

        return RES_OK;
    }
}

static DSTATUS SpiMem_disk_initialize (void *private)
{
    printk("initializing spi mem ........................\n");

    spiMemInit();

    printk("initialized  spi mem ........................\n");

    return SpiMem_disk_status(private);
}

static DRESULT SpiMem_disk_read (void *private, BYTE *Buffer, DWORD SectorNumber, BYTE SectorCount, pointer_type PointerType) {
    printk("read --> args: buffer= 0x%X, sector=%d, count=%d ............\n", (u32)Buffer, SectorNumber, SectorCount);

    SpiMemReadSectors((tySpiMemState *)private, Buffer, SectorNumber, SectorCount, PointerType);
    return RES_OK;
}


#if	_READONLY == 0
static DRESULT SpiMem_disk_write (void *private, const BYTE *Buffer, DWORD SectorNumber, BYTE SectorCount, pointer_type PointerType) {
    printk("write --> args: buffer= 0x%X, sector=%d, count=%d ............\n", (u32)Buffer, SectorNumber, SectorCount);

    SpiMemWriteSectors((tySpiMemState *)private, Buffer, SectorNumber, SectorCount, PointerType);
    return RES_OK;
}
#endif


static DRESULT SpiMem_disk_ioctl (void * __restrict private, BYTE Command, void * __restrict Buffer) {
    tySpiMemState *spiMemLocal = (tySpiMemState *)private;
    switch (Command) {
    case CTRL_SYNC:
        SpiMemWaitWrite();
        break;
    case GET_SECTOR_SIZE:
        *((WORD *) Buffer) = spiMemLocal->sectorSize;
        break;
    case GET_SECTOR_COUNT:
        *((DWORD *) Buffer) = spiMemLocal->sectorCount;
        break;
    case GET_BLOCK_SIZE:
        *((DWORD *) Buffer) = spiMemLocal->sectorPerBlock;
        break;
    case CTRL_ERASE_SECTOR: {
        DWORD start_sector = ((DWORD*)Buffer)[0];
        DWORD end_sector   = ((DWORD*)Buffer)[1];
        printk("erase_sector --> args: start=%d end=%d ............\n", start_sector, end_sector);
        SpiMemEraseSectors((tySpiMemState *)private, start_sector, end_sector);
        break;
    }
    default:
        return RES_PARERR;
    }
    return RES_OK;
    // TODO: return RES_ERROR if any of the commands fails
}

const tyDiskIoFunctions SpiMemWrapperFunctions = {
        .disk_initialize = SpiMem_disk_initialize,
        .disk_status = SpiMem_disk_status,
        .disk_read = SpiMem_disk_read,
#if	_READONLY == 0
        // _READONLY is (re)defined in ffconf.h
        .disk_write = SpiMem_disk_write,
#endif
        .disk_ioctl = SpiMem_disk_ioctl,
        .private = NULL
};
