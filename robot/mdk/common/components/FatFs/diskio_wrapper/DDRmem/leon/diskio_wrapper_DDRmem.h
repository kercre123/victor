#ifndef DISKIO_WRAPPER_DDRMEM_H
#define DISKIO_WRAPPER_DDRMEM_H

#include "diskio_wrapper.h"
#include "mv_types.h"


extern const tyDiskIoFunctions DDRMemWrapperFunctions;

typedef struct {

    u32 start;
    u32 size;

} tyDDRmem;




#endif

