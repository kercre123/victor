#include "diskio_wrapper_SdMem.h"
#include "ffconf.h"
#include "sdMem.h"
#include "stdlib.h"

static DSTATUS SdMem_disk_status (void *private);

static DSTATUS SdMem_disk_initialize (void *private) {
    sdMemCardInit((tySdMemSlotState *)private);
    return SdMem_disk_status(private);
}

static DSTATUS SdMem_disk_status (void *private) {
	tySdMemSlotState *slotState = (tySdMemSlotState *)private;
	DSTATUS flags = 0;
    if (!slotState->cardInitialized) flags |= STA_NOINIT;
    if (slotState->writeProtected) flags |= STA_PROTECT;
    if (sdMemNoCard(slotState)) flags |= STA_NODISK;
    return flags;
}

static DRESULT SdMem_disk_read (void *private, BYTE *Buffer, DWORD SectorNumber, BYTE SectorCount, pointer_type PointerType) {
    int val = sdMemRead((tySdMemSlotState *)private, Buffer, SectorNumber, SectorCount, PointerType);
    return val; /* TODO return something based on val */
}

#if	_READONLY == 0
static DRESULT SdMem_disk_write (void *private, const BYTE *Buffer, DWORD SectorNumber, BYTE SectorCount, pointer_type PointerType) {
    int val = sdMemWrite((tySdMemSlotState *)private, Buffer, SectorNumber, SectorCount, PointerType);
    return val; /* TODO return something based on val */
}
#endif

static DRESULT SdMem_disk_ioctl (void * __restrict private, BYTE Command, void * __restrict Buffer) {
	tySdMemSlotState *slotState = (tySdMemSlotState *)private;
	if (!slotState->cardInitialized) return RES_NOTRDY;
    switch (Command) {
        case CTRL_SYNC:
            sdMemSync(slotState);
            break;
        case GET_SECTOR_SIZE:
            *((WORD *) Buffer) = sdMemGetSectorSize(slotState);
            break;
        case GET_SECTOR_COUNT: {
            u32 sectorCount = sdMemGetSectorCount(slotState);
            *((DWORD *) Buffer) = sectorCount;
            if (sectorCount == 0) return RES_ERROR;
            break;
        }
        case GET_BLOCK_SIZE:
            *((DWORD *) Buffer) = sdMemGetEraseBlockSize(slotState)/sdMemGetSectorSize(slotState);
            break;
        case CTRL_ERASE_SECTOR: {
        	if (slotState->writeProtected) return RES_ERROR;
            DWORD StartSector = ((DWORD *) Buffer)[0];
            DWORD EndSector = ((DWORD *) Buffer)[1];
            sdMemEraseSector(slotState, StartSector, EndSector);
            break;
        }
        default:
        	return RES_PARERR;
    }
    return RES_OK;
    // TODO: return RES_ERROR if any of the commands fails, once drvSdMem interface is clearer
}


const tyDiskIoFunctions SdMemWrapperFunctions = {
    .disk_initialize = SdMem_disk_initialize,
    .disk_status = SdMem_disk_status,
    .disk_read = SdMem_disk_read,
    #if	_READONLY == 0
         // _READONLY is (re)defined in ffconf.h
        .disk_write = SdMem_disk_write,
    #endif
    .disk_ioctl = SdMem_disk_ioctl,
    .private = NULL
};
