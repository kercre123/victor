///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @addtogroup FatFS FatFS component
/// @{
/// @brief     This component adapts the fatfs disk-side interface to a set of easier to use callbacks.
///
/// FatFs has a disk-side interface that is made out of 5 functions, and the first parameter of all 5
/// functions is "BYTE Drive". These function only have prototypes in FatFs, and the user has to implement them.
/// If there are different disk implementations (say Spi flash memory, and SD card),
/// then the implementations of the FatFs disk-side interface should decide which device is being accessed,
/// based on the "Drive" argument, and call the per-device versions of these functions.
///
/// This is exactly what this component does, but in a flexible way.
/// It defines a structure which holds a set of device-specific callbacks for each of the disk access functions,
/// (this time without a Drive parameter), and it provides a way to assign these collections of callbacks
/// to phsycal drive numbers.
///
/// The actual implementation of the disk-side FatFs functions is not visible in this header file, because
/// those functions are actually declared by FatFs's diskio.h. The implementations of those functions
/// is hidden in <b>this</b> header file's matching .c file.
///
/// All that potential future disk devices will have to provide is a filled-in #tyDiskIoFunctions structure.

#ifndef DISKIO_WRAPPER_H
#define DISKIO_WRAPPER_H

#include "diskio.h"

/// @brief Structure that contains a set of callbacks for accessing the media.
/// See the official FatFs documentation for the signature of these functions.
/// (note that these functions are midding the "Drive" argument, because they don't need it,
/// on the other hand they have a "private" argument which is passed to them as
/// it was given to the #diskIoRegister function)
///
/// One movidius-specific change compared to original FatFs is that the disk_read and disk_wire
/// callbacks have a final pointer_type argument, that specifies the endianness of the Buffer argument.
/// pointer_type is either le_pointer or be_pointer.
typedef struct {
    DSTATUS (*disk_initialize) (void *private);
    DSTATUS (*disk_status) (void *private);
    DRESULT (*disk_read) (void *private, BYTE *Buffer, DWORD SectorNumber, BYTE SectorCount, pointer_type PointerType);
    #if	_READONLY == 0
        DRESULT (*disk_write) (void *private, const BYTE *Buffer, DWORD SectorNumber, BYTE SectorCount, pointer_type PointerType);
    #endif
    DRESULT (*disk_ioctl) (void *private, BYTE Command, void *Buffer);
    void *private;
} tyDiskIoFunctions;

/// @brief Register a mapping between a phsycal driver number and a set of callbacks.
///
/// @param[in] physicalDrive The index of the physical drive.
/// @param[in] functions The set of callback functions.
/// @param[in] private A pointer that will get passed as a first argument to all of the callbacks,
///                         when they are called because of the current drive. This can be used
///                         as an explicit "self" pointer.
void diskIoRegister(BYTE physicalDrive, const tyDiskIoFunctions *functions, void *private);

/// @brief Set a different get time callback. This callback is used when creating new files, to set the timestamp of the files.
///
/// @param[in] get_fattime The callback
void diskIoSetFatTime(DWORD (*get_fattime) (void));

/// @brief A dummy initial get_fattime callback, which returns a constant value.
///
/// This will be used if you don't set a different callback.
DWORD get_fattime_dummy(void); // default get_fattime, returns constant value
/// @}
#endif

