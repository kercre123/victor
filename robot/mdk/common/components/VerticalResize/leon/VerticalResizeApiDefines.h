///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by the Vertical resize library
/// 
#ifndef _VERTICAL_RESIZE_API_DEFINES_H_
#define _VERTICAL_RESIZE_API_DEFINES_H_

// 1: Defines
// ----------------------------------------------------------------------------
#ifndef VRES_DATA_SECTION
#define VRES_DATA_SECTION(x) x
#endif

#ifndef VRES_CODE_SECTION
#define VRES_CODE_SECTION(x) x
#endif

#define VRES_SHAVES 2

#ifndef VRES_SH1
#define VRES_SH1 4
#endif

#ifndef VRES_SH2
#define VRES_SH2 5
#endif

#define VRES_sym_aux(svu_number,symbolname) vert##svu_number##_##symbolname
#define VRES_sym(svu_number,symbolname) VRES_sym_aux(svu_number,symbolname)

#define VRES_SHAVE 4
//Just set simply at the top of Shave0 CMX Slice (128K - 0x8000 (the code size))
#define VRES_STACK 0x1C017FFC

//Number of output buffers
#define VERTRESZ_BUFFERS 3
// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
 
// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

// Constant value description

#endif //_VERTICAL_RESIZE_API_DEFINES_H_
