#include "diskio_wrapper_DDRmem.h"
#include "ffconf.h"
#include "mv_types.h"
#include "stdlib.h"
#include "isaac_registers.h"
#include "stdio.h"
#include "string.h"
#include "diskio.h"

#include "swcLeonUtils.h"
#include "DrvDdr.h"
#include <DrvCpr.h>
#include "DrvSvu.h" // this must be replaced
#include <swcLeonUtils.h>

static DSTATUS DDRMem_disk_initialize (void *private) {

    u32 *addr;

    addr= (u32 *)CPR_AUX_CLK_EN_ADR;

    if (!(((*addr)&(1<<4))>>4)) DrvDdrInitialise(DrvCprGetClockFreqKhz(AUX_CLK_DDR,NULL));

    return 0;
}



static DSTATUS DDRMem_disk_status (void *private) {

    u32 *addr;

    addr= (u32 *)CPR_AUX_CLK_EN_ADR;

    if (!(((*addr)&(1<<4))>>4)) return STA_NOINIT;
    return 0;
}

static DRESULT DDRMem_disk_read (void *private, BYTE *Buffer, DWORD SectorNumber, BYTE SectorCount, pointer_type PointerType) {

    tyDDRmem *DDRmem;
    u32 *addr;
    int i;

    DDRmem = (tyDDRmem *)private;

    addr = (u32 *)(DDRmem->start + SectorNumber * 512);

    swcLeonMemCpy(Buffer, PointerType, addr, le_pointer, SectorCount*512);

    return 0;
}

#if    _READONLY == 0
static DRESULT DDRMem_disk_write (void *private, const BYTE *Buffer, DWORD SectorNumber, BYTE SectorCount, pointer_type PointerType) {


    tyDDRmem *DDRmem;
    u32 *addr, offset, *aligned_buffer;
    int i;



    DDRmem = (tyDDRmem *)private;

    addr = (u32 *)(DDRmem->start + SectorNumber * 512);


    swcLeonMemCpy(addr, le_pointer, Buffer, PointerType, SectorCount*512);

    return 0;
}
#endif

static DRESULT DDRMem_disk_ioctl (void * __restrict private, BYTE Command, void * __restrict Buffer) {

     tyDDRmem *DDRmem;
     DDRmem = (tyDDRmem *)private;


    switch (Command) {
            case CTRL_SYNC:
                return RES_OK;
                break;
            case GET_SECTOR_SIZE:
                *((WORD *) Buffer) = 512;
                break;
            case GET_SECTOR_COUNT: { // seek to the end of the file, and determine how many sectors fit into it.

                *((DWORD *) Buffer) = (DDRmem->size)/512;
                break;
            }
            case GET_BLOCK_SIZE:
                *((DWORD *) Buffer) = 1;
                break;
            case CTRL_ERASE_SECTOR:
                return RES_OK;
                break;
            default:
                return RES_PARERR;
        }

        return RES_OK;

}


const tyDiskIoFunctions DDRMemWrapperFunctions = {
    .disk_initialize = DDRMem_disk_initialize,
    .disk_status = DDRMem_disk_status,
    .disk_read = DDRMem_disk_read,
    #if    _READONLY == 0
         // _READONLY is (re)defined in ffconf.h
        .disk_write = DDRMem_disk_write,
    #endif
    .disk_ioctl = DDRMem_disk_ioctl,
    .private = NULL
};







