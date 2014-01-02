#include "diskio_wrapper.h"
#include "ffconf.h"

static DWORD (*get_fattime_func) (void) = get_fattime_dummy;
static tyDiskIoFunctions disk_io_functions [_VOLUMES];
// _VOLUMES is defined in ffconf.h, and it is the number of logical drivers
// there can't be more physical drives then logical drives, so this is safe

void diskIoSetFatTime(DWORD (*get_fattime) (void)) {
    get_fattime_func = get_fattime;
}

DWORD get_fattime(void) {
    return get_fattime_func();
}

DWORD get_fattime_dummy(void)
{
	return	  ((DWORD)(2010 - 1980) << 25)	/* Fixed to Jan. 1, 2010 */
			| ((DWORD)1 << 21)
			| ((DWORD)1 << 16)
			| ((DWORD)0 << 11)
			| ((DWORD)0 << 5)
			| ((DWORD)0 >> 1);
}

void diskIoRegister(BYTE physicalDrive, const tyDiskIoFunctions *functions, void *private) {
    disk_io_functions[physicalDrive] = *functions;
    disk_io_functions[physicalDrive].private = private;
}

DRESULT disk_ioctl (
  BYTE Drive,      /* Drive number */
  BYTE Command,    /* Control command code */
  void* Buffer     /* Parameter and data buffer */
) {
    return disk_io_functions[Drive].disk_ioctl(disk_io_functions[Drive].private, Command, Buffer);
}

DSTATUS disk_initialize (
  BYTE Drive           /* Physical drive number */
) {
    return disk_io_functions[Drive].disk_initialize(disk_io_functions[Drive].private);
}

DSTATUS disk_status (
  BYTE Drive     /* Physical drive number */
) {
    return disk_io_functions[Drive].disk_status(disk_io_functions[Drive].private);
}

DRESULT disk_read (
  BYTE Drive,          /* Physical drive number */
  BYTE* Buffer,        /* Pointer to the read data buffer */
  DWORD SectorNumber,  /* Start sector number */
  BYTE SectorCount,    /* Number of sectros to read */
  pointer_type pt      /* endianness of the Buffer */
) {
    return disk_io_functions[Drive].disk_read(disk_io_functions[Drive].private, Buffer, SectorNumber, SectorCount, pt);
}



DRESULT disk_write (
  BYTE Drive,          /* Physical drive number */
  const BYTE* Buffer,  /* Pointer to the write data (may be non aligned) */
  DWORD SectorNumber,  /* Sector number to write */
  BYTE SectorCount,    /* Number of sectors to write */
  pointer_type pt      /* endianness of the Buffer */
) {
    return disk_io_functions[Drive].disk_write(disk_io_functions[Drive].private, Buffer, SectorNumber, SectorCount, pt);
}

