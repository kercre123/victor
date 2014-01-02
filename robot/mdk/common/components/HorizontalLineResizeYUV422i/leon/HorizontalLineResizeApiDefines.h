///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by the Horizontal resize library
/// 
#ifndef _HORIZONTAL_LINE_RESIZE_API_DEFINES_H_
#define _HORIZONTAL_LINE_RESIZE_API_DEFINES_H_

// 1: Defines
// ----------------------------------------------------------------------------
#ifndef HRES_DATA_SECTION
#define HRES_DATA_SECTION(x) x
#endif

#ifndef HRES_CODE_SECTION
#define HRES_CODE_SECTION(x) x
#endif

//Just set simply at the top of Shave0 CMX Slice (128K - 0x8000 (the code size))
#define HRES_STACK 0x1C017FFC

//Number of output buffers
#define HORIZRESZ_BUFFERS 3

#define HRES_SHAVE_NO 5
// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
 
// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

// Constant value description

#endif //_HORIZONTAL_LINE_RESIZE_API_DEFINES_H_
