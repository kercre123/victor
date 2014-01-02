///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by the Math operations library
/// 
#ifndef _YUV_SUBSAMPLE_API_DEFINES_H_
#define _YUV_SUBSAMPLE_API_DEFINES_H_


#ifndef YUVSUBSAMPLE_DATA_SECTION
#define YUVSUBSAMPLE_DATA_SECTION(x) x
#endif

#ifndef YUVSUBSAMPLE_CODE_SECTION
#define YUVSUBSAMPLE_CODE_SECTION(x) x
#endif

//run the code on shave 0
#ifndef YUVSUBSAMPLE_SHAVE
#define YUVSUBSAMPLE_SHAVE 0
#endif

#define YUVSUBSAMPLE_sym_aux(svu_number,symbolname) SVE##svu_number##_##symbolname
#define YUVSUBSAMPLE_sym(svu_number,symbolname) YUVSUBSAMPLE_sym_aux(svu_number,symbolname)

//Just set simply at the top of Shave0 CMX Slice (128K - 0x8000 (the code size))
#define YUVSUBSAMPLE_STACK 0x1C017FFC


#endif //_YUV_SUBSAMPLE_DEFINES_H_