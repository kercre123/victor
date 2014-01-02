// usbCoreApiDefines.h


#ifndef _BI_RESIZE_API_DEFINES_H_
#define _BI_RESIZE_API_DEFINES_H_

// 1: Defines
// ----------------------------------------------------------------------------

/// @brief   error codes
#define BI_RESIZE_OK            0x00
#define BI_RESIZE_INIT          0x01
#define BI_RESIZE_BUSY          0x02
#define BI_RESIZE_INVALID       0xFF


// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

/// @brief   control data structure
typedef struct
{
    u32 wi;
    u32 hi;
    u32 wo;
    u32 ho;
    u32 offsetX;
    u32 offsetY;
    u32 inBuffer;
    u32 outBuffer;
} biResize_param_t;

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------


#endif /* _BI_RESIZE_API_DEFINES_H_ */
