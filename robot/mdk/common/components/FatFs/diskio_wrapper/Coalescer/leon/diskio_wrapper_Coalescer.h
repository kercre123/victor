#ifndef DISKIO_WRAPPER_COALESCER_H
#define DISKIO_WRAPPER_COALESCER_H

#include "diskio_wrapper.h"
#include "mv_types.h"
#include "swcLeonUtils.h"

extern const tyDiskIoFunctions CoalescerWrapperFunctions;

typedef struct coalescer {
    const tyDiskIoFunctions *wrappedFunctions;
    void *private;
    void *buffer;
    u32 buffer_size; // this is the maximum size of the buffer
    int buffer_dirty;
    u32 buffer_dirty_from;
    u32 underlying_sector_size; // may be left zero, then ioctl will be called
    void *underlying_sector_buf; // only needed if underlying_sector_size != presented_sector_size
    u32 presented_sector_size; // 0 means 512 bytes
    u32 pos; // position (in bytes) of the start of the buffer
    s32 count; // number of bytes in the buffer
    pointer_type preferred_endianness; // choose it to maximize performance;
} coalescer_t;

#endif

