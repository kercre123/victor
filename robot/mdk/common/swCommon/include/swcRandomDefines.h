///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by swcRandom
/// 
#ifndef SWC_RANDI_DEF_H
#define SWC_RANDI_DEF_H 

// 1: Defines
// ----------------------------------------------------------------------------

#define RAND_MAX ((u64)(-1))

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------
typedef enum
{
    RAND_WRITE,       // This is a byte by byte based operation
    RAND_VERIFY,      // This is a byte by byte based operation 
    RAND_WRITE_32,    // 32 bit operation
    RAND_VERIFY_32    // 32 bit operation 
} tyRandOperation;

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // SWC_RANDI_DEF_H
