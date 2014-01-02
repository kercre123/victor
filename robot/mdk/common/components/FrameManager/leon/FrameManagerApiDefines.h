///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by the Math operations library
/// 
#ifndef _FRAMEMANAGER_API_DEFINES_H_
#define _FRAMEMANAGER_API_DEFINES_H_

// 1: Defines
// ----------------------------------------------------------------------------
#ifndef FRAMEMANAGER_DATA_SECTION
#define FRAMEMANAGER_DATA_SECTION(x) x
#endif

#ifndef FRAMEMANAGER_CODE_SECTION
#define FRAMEMANAGER_CODE_SECTION(x) x
#endif

#ifndef MAX_FRAME_CREATORS
#define MAX_FRAME_CREATORS 4
#endif

#ifndef MAX_FRAMES_PER_CREATOR
#define MAX_FRAMES_PER_CREATOR 8
#endif
// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

typedef enum FM_results{
	FM_FAIL,
	FM_SUCCESS
}FMRes_type;

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

// Constant value description

#endif //_FRAMEMANAGER_API_DEFINES_H_
